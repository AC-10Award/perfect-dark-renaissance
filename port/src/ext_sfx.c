#include <dirent.h>
#include <string.h>
#include <strings.h>
#include <SDL.h>
#include <PR/ultratypes.h>
#include <PR/libaudio.h>
#include "constants.h"
#include "audio.h"
#include "ext_sfx.h"
#include "fs.h"
#include "system.h"

#define EXT_SFX_DIR "ext_sfx"
#define EXT_SFX_TARGET_HZ 22020
#define EXT_SFX_HEADROOM_SHIFT 1
#define EXT_SFX_FULL_VOLUME 0x7fff
#define EXT_SFX_MAX_CACHE 128
#define EXT_SFX_MAX_FILE_BYTES (16u * 1024u * 1024u)
#define EXT_SFX_MAX_DECODED_FRAMES (60u * EXT_SFX_TARGET_HZ)
#define EXT_SFX_MAX_VOICES 24

static char g_ExtSfxPath[FS_MAXPATH + 1];
static s32 g_ExtSfxInitDone = 0;
static s32 g_ExtSfxDirExists = 0;

struct extsfxvoice {
	char filepath[FS_MAXPATH + 1];
	s16 *pcm;
	u32 frames;
	s32 pcmowned;
	u32 playhead;
	s32 volume;
	s32 pan;
	s32 priority;
	u32 startedtick;
};

struct extsfxcacheentry {
	char filepath[FS_MAXPATH + 1];
	s16 *pcm;
	u32 frames;
};

static struct extsfxvoice g_ExtSfxVoices[EXT_SFX_MAX_VOICES];
static struct extsfxcacheentry g_ExtSfxCache[EXT_SFX_MAX_CACHE];
static s32 g_ExtSfxCacheCount = 0;
static u32 g_ExtSfxNextVoiceTick = 1;

static s32 extSfxCacheLookup(const char *filepath, s16 **outPcm, u32 *outFrames);
static s32 extSfxCacheStore(const char *filepath, s16 *pcm, u32 frames);
extern u16 g_SfxVolume;

static inline s32 extSfxGetEffectiveVolume(s32 volume)
{
	return (volume * VOLUME(g_SfxVolume)) / EXT_SFX_FULL_VOLUME;
}

static inline s16 extSfxClamp16(s32 value)
{
	if (value < -0x8000) {
		return -0x8000;
	}

	if (value > 0x7fff) {
		return 0x7fff;
	}

	return (s16)value;
}

static void extSfxResetVoice(struct extsfxvoice *voice)
{
	if (voice->pcm && voice->pcmowned) {
		SDL_free(voice->pcm);
	}

	voice->filepath[0] = '\0';
	voice->pcm = NULL;
	voice->frames = 0;
	voice->pcmowned = 0;
	voice->playhead = 0;
	voice->volume = 0;
	voice->pan = AL_PAN_CENTER;
	voice->priority = EXT_SFX_PRIORITY_NORMAL;
	voice->startedtick = 0;
}

static void extSfxResetPlayback(void)
{
	for (s32 i = 0; i < EXT_SFX_MAX_VOICES; i++) {
		extSfxResetVoice(&g_ExtSfxVoices[i]);
	}
}

static struct extsfxvoice *extSfxChooseVoice(s32 priority)
{
	struct extsfxvoice *best = &g_ExtSfxVoices[0];

	for (s32 i = 0; i < EXT_SFX_MAX_VOICES; i++) {
		if (!g_ExtSfxVoices[i].pcm || g_ExtSfxVoices[i].frames == 0) {
			return &g_ExtSfxVoices[i];
		}

		if (g_ExtSfxVoices[i].priority < best->priority
				|| (g_ExtSfxVoices[i].priority == best->priority
					&& g_ExtSfxVoices[i].startedtick < best->startedtick)) {
			best = &g_ExtSfxVoices[i];
		}
	}

	if (best->priority > priority) {
		return NULL;
	}

	return best;
}

static s32 extSfxClampPan(s32 pan)
{
	if (pan < AL_PAN_LEFT) {
		return AL_PAN_LEFT;
	}

	if (pan > AL_PAN_RIGHT) {
		return AL_PAN_RIGHT;
	}

	return pan;
}

static s32 extSfxStartVoice(const char *filepath, s16 *pcm, u32 frames, s32 pcmowned, s32 volume, s32 pan, s32 priority)
{
	struct extsfxvoice *voice = extSfxChooseVoice(priority);

	if (!voice) {
		return 0;
	}

	extSfxResetVoice(voice);
	strncpy(voice->filepath, filepath, FS_MAXPATH);
	voice->filepath[FS_MAXPATH] = '\0';
	voice->pcm = pcm;
	voice->frames = frames;
	voice->pcmowned = pcmowned;
	voice->playhead = 0;
	voice->volume = volume > 0 ? volume : EXT_SFX_FULL_VOLUME;
	voice->pan = extSfxClampPan(pan);
	voice->priority = priority;
	voice->startedtick = g_ExtSfxNextVoiceTick++;

	if (g_ExtSfxNextVoiceTick == 0) {
		g_ExtSfxNextVoiceTick = 1;
	}

	return 1;
}

static void extSfxFreeCache(void)
{
	for (s32 i = 0; i < g_ExtSfxCacheCount; i++) {
		if (g_ExtSfxCache[i].pcm) {
			SDL_free(g_ExtSfxCache[i].pcm);
			g_ExtSfxCache[i].pcm = NULL;
		}

		g_ExtSfxCache[i].filepath[0] = '\0';
		g_ExtSfxCache[i].frames = 0;
	}

	g_ExtSfxCacheCount = 0;
}

void extSfxInit(void)
{
	const char *resolvedPath;

	if (g_ExtSfxInitDone) {
		return;
	}

	resolvedPath = fsFullPath(EXT_SFX_DIR);

	if (resolvedPath && fsFileSize(resolvedPath) >= 0) {
		strncpy(g_ExtSfxPath, resolvedPath, sizeof(g_ExtSfxPath) - 1);
		g_ExtSfxDirExists = 1;
	} else if (fsFileSize("./" EXT_SFX_DIR) >= 0) {
		strncpy(g_ExtSfxPath, "./" EXT_SFX_DIR, sizeof(g_ExtSfxPath) - 1);
		g_ExtSfxDirExists = 1;
	} else {
		g_ExtSfxPath[0] = '\0';
		g_ExtSfxDirExists = 0;
	}

	g_ExtSfxInitDone = 1;

	if (g_ExtSfxDirExists) {
		sysLogPrintf(LOG_NOTE, "ext_sfx: directory found at %s", g_ExtSfxPath);
	} else {
		sysLogPrintf(LOG_NOTE, "ext_sfx: directory not found");
	}
}

void extSfxFree(void)
{
	audioLock();
	extSfxResetPlayback();
	audioUnlock();
	extSfxFreeCache();
	g_ExtSfxPath[0] = '\0';
	g_ExtSfxDirExists = 0;
	g_ExtSfxInitDone = 0;
}

s32 extSfxDirExists(void)
{
	return g_ExtSfxDirExists;
}

static s32 extSfxCacheLookup(const char *filepath, s16 **outPcm, u32 *outFrames)
{
	for (s32 i = 0; i < g_ExtSfxCacheCount; i++) {
		if (strcmp(g_ExtSfxCache[i].filepath, filepath) == 0) {
			if (outPcm) {
				*outPcm = g_ExtSfxCache[i].pcm;
			}

			if (outFrames) {
				*outFrames = g_ExtSfxCache[i].frames;
			}

			return 1;
		}
	}

	return 0;
}

static s32 extSfxCacheStore(const char *filepath, s16 *pcm, u32 frames)
{
	if (!filepath || !filepath[0] || !pcm || frames == 0 || g_ExtSfxCacheCount >= EXT_SFX_MAX_CACHE) {
		return 0;
	}

	strncpy(g_ExtSfxCache[g_ExtSfxCacheCount].filepath, filepath, FS_MAXPATH);
	g_ExtSfxCache[g_ExtSfxCacheCount].filepath[FS_MAXPATH] = '\0';
	g_ExtSfxCache[g_ExtSfxCacheCount].pcm = pcm;
	g_ExtSfxCache[g_ExtSfxCacheCount].frames = frames;
	g_ExtSfxCacheCount++;
	return 1;
}

s32 extSfxPreloadAll(void)
{
	DIR *dr;
	struct dirent *de;
	s32 loaded = 0;

	if (!g_ExtSfxDirExists || !g_ExtSfxPath[0]) {
		return 0;
	}

	dr = opendir(g_ExtSfxPath);

	if (!dr) {
		return 0;
	}

	while ((de = readdir(dr)) != NULL) {
		char filepath[FS_MAXPATH + 1];
		SDL_AudioSpec wavSpec;
		SDL_AudioCVT cvt;
		Uint8 *wavBuf = NULL;
		Uint32 wavLen = 0;
		size_t len;

		if (de->d_name[0] == '.') {
			continue;
		}

		len = strlen(de->d_name);

		if (len < 4 || strcasecmp(&de->d_name[len - 4], ".wav") != 0) {
			continue;
		}

			snprintf(filepath, sizeof(filepath), "%s/%s", g_ExtSfxPath, de->d_name);

			if (extSfxCacheLookup(filepath, NULL, NULL)) {
				continue;
			}

			const s32 filesize = fsFileSize(filepath);
			if (filesize < 0 || (u32)filesize > EXT_SFX_MAX_FILE_BYTES) {
				sysLogPrintf(LOG_WARNING, "ext_sfx: skipping oversized WAV %s", filepath);
				continue;
			}

			if (!SDL_LoadWAV(filepath, &wavSpec, &wavBuf, &wavLen)) {
				continue;
			}

		if (SDL_BuildAudioCVT(&cvt, wavSpec.format, wavSpec.channels, wavSpec.freq,
				AUDIO_S16SYS, 1, EXT_SFX_TARGET_HZ) < 0) {
			SDL_FreeWAV(wavBuf);
			continue;
		}

		cvt.len = (s32)wavLen;
		cvt.buf = SDL_malloc((size_t)cvt.len * (size_t)cvt.len_mult);

		if (!cvt.buf) {
			SDL_FreeWAV(wavBuf);
			continue;
		}

		memcpy(cvt.buf, wavBuf, wavLen);
		SDL_FreeWAV(wavBuf);

			if (SDL_ConvertAudio(&cvt) < 0) {
				SDL_free(cvt.buf);
				continue;
			}

			if ((u32)cvt.len_cvt / sizeof(s16) > EXT_SFX_MAX_DECODED_FRAMES) {
				sysLogPrintf(LOG_WARNING, "ext_sfx: skipping overlong WAV %s", filepath);
				SDL_free(cvt.buf);
				continue;
			}

			if (!extSfxCacheStore(filepath, (s16 *)cvt.buf, (u32)cvt.len_cvt / sizeof(s16))) {
				SDL_free(cvt.buf);
				break;
			}

		loaded++;
		sysLogPrintf(LOG_NOTE, "ext_sfx: preloaded %s (%u mono frames)", filepath, (u32)cvt.len_cvt / sizeof(s16));
	}

	closedir(dr);
	sysLogPrintf(LOG_NOTE, "ext_sfx: preload complete, cached %d file(s)", loaded);
	return loaded;
}

s32 extSfxPlay(const char *filename, s32 volume)
{
	return extSfxPlayPanPriority(filename, volume, AL_PAN_CENTER, EXT_SFX_PRIORITY_NORMAL);
}

s32 extSfxPlayPan(const char *filename, s32 volume, s32 pan)
{
	return extSfxPlayPanPriority(filename, volume, pan, EXT_SFX_PRIORITY_NORMAL);
}

s32 extSfxPlayPriority(const char *filename, s32 volume, s32 priority)
{
	return extSfxPlayPanPriority(filename, volume, AL_PAN_CENTER, priority);
}

s32 extSfxPlayPanPriority(const char *filename, s32 volume, s32 pan, s32 priority)
{
	char filepath[FS_MAXPATH + 1];
	SDL_AudioSpec wavSpec;
	SDL_AudioCVT cvt;
	Uint8 *wavBuf = NULL;
	Uint32 wavLen = 0;

	if (!g_ExtSfxDirExists || !filename || !filename[0]) {
		return 0;
	}

	snprintf(filepath, sizeof(filepath), "%s/%s", g_ExtSfxPath, filename);

	const s32 filesize = fsFileSize(filepath);
	if (filesize < 0 || (u32)filesize > EXT_SFX_MAX_FILE_BYTES) {
		sysLogPrintf(LOG_WARNING, "ext_sfx: skipping oversized WAV %s", filepath);
		return 0;
	}

	{
		s16 *pcm = NULL;
		u32 frames = 0;

			if (extSfxCacheLookup(filepath, &pcm, &frames)) {
				s32 started;

				audioLock();
				started = extSfxStartVoice(filepath, pcm, frames, 0, volume, pan, priority);
				audioUnlock();
				return started;
			}
	}

	if (!SDL_LoadWAV(filepath, &wavSpec, &wavBuf, &wavLen)) {
		sysLogPrintf(LOG_WARNING, "ext_sfx: SDL_LoadWAV failed for %s: %s", filepath, SDL_GetError());
		return 0;
	}

	if (SDL_BuildAudioCVT(&cvt, wavSpec.format, wavSpec.channels, wavSpec.freq,
			AUDIO_S16SYS, 1, EXT_SFX_TARGET_HZ) < 0) {
		sysLogPrintf(LOG_WARNING, "ext_sfx: SDL_BuildAudioCVT failed for %s: %s", filepath, SDL_GetError());
		SDL_FreeWAV(wavBuf);
		return 0;
	}

	cvt.len = (s32)wavLen;
	cvt.buf = SDL_malloc((size_t)cvt.len * (size_t)cvt.len_mult);

	if (!cvt.buf) {
		sysLogPrintf(LOG_WARNING, "ext_sfx: failed to allocate conversion buffer for %s", filepath);
		SDL_FreeWAV(wavBuf);
		return 0;
	}

	memcpy(cvt.buf, wavBuf, wavLen);
	SDL_FreeWAV(wavBuf);

	if (SDL_ConvertAudio(&cvt) < 0) {
		sysLogPrintf(LOG_WARNING, "ext_sfx: SDL_ConvertAudio failed for %s: %s", filepath, SDL_GetError());
		SDL_free(cvt.buf);
		return 0;
	}

	if ((u32)cvt.len_cvt / sizeof(s16) > EXT_SFX_MAX_DECODED_FRAMES) {
		sysLogPrintf(LOG_WARNING, "ext_sfx: skipping overlong WAV %s", filepath);
		SDL_free(cvt.buf);
		return 0;
	}

	audioLock();
	if (!extSfxStartVoice(filepath, (s16 *)cvt.buf, (u32)cvt.len_cvt / sizeof(s16), 1, volume, pan, priority)) {
		audioUnlock();
		SDL_free(cvt.buf);
		return 0;
	}
	audioUnlock();

	sysLogPrintf(LOG_NOTE, "ext_sfx: started %s (%u mono frames)", filepath, (u32)cvt.len_cvt / sizeof(s16));
	return 1;
}

s32 extSfxMixIntoBuffer(s16 *samples, u32 frames)
{
	s32 mixed = 0;

	if (!samples || frames == 0) {
		return 0;
	}

	for (s32 voiceindex = 0; voiceindex < EXT_SFX_MAX_VOICES; voiceindex++) {
		struct extsfxvoice *voice = &g_ExtSfxVoices[voiceindex];
		const s32 effectivevolume = extSfxGetEffectiveVolume(voice->volume);
		const s32 pan = extSfxClampPan(voice->pan);
		const s32 leftscale = pan <= AL_PAN_CENTER
			? EXT_SFX_FULL_VOLUME
			: ((AL_PAN_RIGHT - pan) * EXT_SFX_FULL_VOLUME) / (AL_PAN_RIGHT - AL_PAN_CENTER);
		const s32 rightscale = pan >= AL_PAN_CENTER
			? EXT_SFX_FULL_VOLUME
			: (pan * EXT_SFX_FULL_VOLUME) / AL_PAN_CENTER;

		if (!voice->pcm || voice->frames == 0 || effectivevolume <= 0) {
			continue;
		}

		for (u32 i = 0; i < frames; i++) {
			const s32 sfx = ((voice->pcm[voice->playhead] * effectivevolume) / EXT_SFX_FULL_VOLUME) >> EXT_SFX_HEADROOM_SHIFT;
			const s32 left = (sfx * leftscale) / EXT_SFX_FULL_VOLUME;
			const s32 right = (sfx * rightscale) / EXT_SFX_FULL_VOLUME;
			const u32 base = i * 2;

			samples[base + 0] = extSfxClamp16((s32)samples[base + 0] + left);
			samples[base + 1] = extSfxClamp16((s32)samples[base + 1] + right);

			voice->playhead++;

			if (voice->playhead >= voice->frames) {
				extSfxResetVoice(voice);
				break;
			}
		}

		mixed = 1;
	}

	return mixed;
}
