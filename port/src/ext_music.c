#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <PR/ultratypes.h>
#include <ultra64.h>
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <AudioToolbox/AudioToolbox.h>
#endif
#include "constants.h"
#include "data.h"
#include "audio.h"
#include "fs.h"
#include "system.h"
#include "ext_music.h"
#include "md/audio_aliases.h"
#include "game/mplayer/mplayer.h"
#include "bss.h"
#include "external/minimp3.h"
#include "lib/rng.h"

#define EXT_MUSIC_DIR "ext_music"
#define EXT_MUSIC_TARGET_HZ 22020
#define EXT_MUSIC_PHASE_BITS 16
#define EXT_MUSIC_PHASE_ONE (1u << EXT_MUSIC_PHASE_BITS)
#define EXT_MUSIC_HEADROOM_SHIFT 1
#define EXT_MUSIC_MAX_CACHE 128
#define EXT_MUSIC_MAX_FILE_BYTES (32u * 1024u * 1024u)
#define EXT_MUSIC_MAX_DECODED_FRAMES (15u * 60u * EXT_MUSIC_TARGET_HZ)

static char g_ExtMusicPath[FS_MAXPATH + 1];

static s32 g_ExtMusicDirExists = 0;
static s32 g_ExtMusicInitDone = 0;
static void *g_ExtMusicMenuData = NULL;
static u32 g_ExtMusicMenuSize = 0;
static s16 *g_ExtMusicMenuPcm = NULL;
static u32 g_ExtMusicMenuPcmFrames = 0;
static s32 g_ExtMusicMenuPcmOwned = 0;
static u32 g_ExtMusicPlayhead = 0;
static s32 g_ExtMusicVolume = 0;
static s32 g_ExtMusicActiveTracktype = TRACKTYPE_NONE;
static s32 g_ExtMusicLoopActiveTrack = 1;
static char g_ExtMusicCurrentKey[128];
static char g_ExtMusicCurrentFile[FS_MAXPATH + 1];
static s32 g_ExtMusicCurrentVariant = -1;
static s16 *g_ExtMusicDeathPcm = NULL;
static u32 g_ExtMusicDeathPcmFrames = 0;
static s32 g_ExtMusicDeathPcmOwned = 0;
static u32 g_ExtMusicDeathPlayhead = 0;
static s32 g_ExtMusicDeathVolume = 0;
static char g_ExtMusicDeathFile[FS_MAXPATH + 1];

struct extmusiccacheentry {
	char filepath[FS_MAXPATH + 1];
	s16 *pcm;
	u32 frames;
};

struct extmusicmatch {
	char filepath[FS_MAXPATH + 1];
	s32 variant;
};

static struct extmusiccacheentry g_ExtMusicCache[EXT_MUSIC_MAX_CACHE];
static s32 g_ExtMusicCacheCount = 0;

static const char *g_ExtMusicKnownKeys[] = {
	MD_MUSIC_KEY_OPENING_COMPANY_LOGOS,
	MD_MUSIC_KEY_MAIN_TITLE_LOGO,
	MD_MUSIC_KEY_MAIN_MENU,
	MD_MUSIC_KEY_COMBAT_SIM_MENU,
	MD_MUSIC_KEY_TERMINATION_MENU,
	MD_MUSIC_KEY_COMBAT_SIM_COMPLETE,
	MD_MUSIC_KEY_MULTIPLAYER_DEATH,
	MD_MUSIC_KEY_LMS_WIN,
	MD_MUSIC_KEY_LMS_LOSE,
	MD_MUSIC_KEY_TERMINATION_WIN,
	MD_MUSIC_KEY_TERMINATION_LOSE,
	MD_MUSIC_KEY_MISSION_SUCCESS,
	MD_MUSIC_KEY_MISSION_UNKNOWN,
	MD_MUSIC_KEY_MISSION_FAILED,
	MD_MUSIC_KEY_PAUSE_MENU,
	MD_MUSIC_KEY_SOLO_DEATH,
	MD_MUSIC_KEY_CI_TRAINING,
	MD_MUSIC_KEY_MP_DARK_COMBAT,
	MD_MUSIC_KEY_MP_SKEDAR_MYSTERY,
	MD_MUSIC_KEY_MP_CI_OPERATIVE,
	MD_MUSIC_KEY_MP_DATADYNE_ACTION,
	MD_MUSIC_KEY_MP_MAIAN_TEARS,
	MD_MUSIC_KEY_MP_ALIEN_CONFLICT,
	MD_MUSIC_KEY_MP_CI,
	MD_MUSIC_KEY_MP_DD_CENTRAL,
	MD_MUSIC_KEY_MP_DD_CENTRAL_X,
	MD_MUSIC_KEY_MP_DD_RESEARCH,
	MD_MUSIC_KEY_MP_DD_RESEARCH_X,
	MD_MUSIC_KEY_MP_DD_EXTRACTION,
	MD_MUSIC_KEY_MP_DD_EXTRACTION_X,
	MD_MUSIC_KEY_MP_VILLA,
	MD_MUSIC_KEY_MP_VILLA_X,
	MD_MUSIC_KEY_MP_CHICAGO,
	MD_MUSIC_KEY_MP_CHICAGO_X,
	MD_MUSIC_KEY_MP_G5,
	MD_MUSIC_KEY_MP_G5_X,
	MD_MUSIC_KEY_MP_A51_INFILTRATION,
	MD_MUSIC_KEY_MP_A51_INFILTRATION_X,
	MD_MUSIC_KEY_MP_A51_RESCUE,
	MD_MUSIC_KEY_MP_A51_RESCUE_X,
	MD_MUSIC_KEY_MP_A51_ESCAPE,
	MD_MUSIC_KEY_MP_A51_ESCAPE_X,
	MD_MUSIC_KEY_MP_AIR_BASE,
	MD_MUSIC_KEY_MP_AIR_BASE_X,
	MD_MUSIC_KEY_MP_AIR_FORCE_ONE,
	MD_MUSIC_KEY_MP_AIR_FORCE_ONE_X,
	MD_MUSIC_KEY_MP_CRASH_SITE,
	MD_MUSIC_KEY_MP_CRASH_SITE_X,
	MD_MUSIC_KEY_MP_PELAGIC_II,
	MD_MUSIC_KEY_MP_PELAGIC_II_X,
	MD_MUSIC_KEY_MP_DEEP_SEA,
	MD_MUSIC_KEY_MP_DEEP_SEA_X,
	MD_MUSIC_KEY_MP_INSTITUTE_DEFENSE,
	MD_MUSIC_KEY_MP_INSTITUTE_DEFENSE_X,
	MD_MUSIC_KEY_MP_ATTACK_SHIP,
	MD_MUSIC_KEY_MP_ATTACK_SHIP_X,
	MD_MUSIC_KEY_MP_SKEDAR_RUINS,
	MD_MUSIC_KEY_MP_SKEDAR_RUINS_X,
	MD_MUSIC_KEY_MP_END_CREDITS,
	MD_MUSIC_KEY_MP_SKEDAR_WARRIOR,
};

static s32 extMusicEnsureTrackLoaded(const char *key, const char *filepath);
static s32 extMusicCacheLookup(const char *filepath, s16 **outPcm, u32 *outFrames);
static s32 extMusicCacheStore(const char *filepath, s16 *pcm, u32 frames);
static s32 extMusicCollectMatchesRecursive(const char *rootpath, const char *subpath, const char *key, struct extmusicmatch *matches, s32 maxmatches, s32 *count);

static s32 extMusicFileWithinLimit(const char *filepath)
{
	const s32 filesize = fsFileSize(filepath);

	if (filesize < 0 || (u32)filesize > EXT_MUSIC_MAX_FILE_BYTES) {
		sysLogPrintf(LOG_WARNING, "ext_music: skipping oversized MP3 %s", filepath);
		return 0;
	}

	return 1;
}

static inline s16 extMusicClamp16(s32 value)
{
	if (value < -0x8000) {
		return -0x8000;
	}

	if (value > 0x7fff) {
		return 0x7fff;
	}

	return (s16)value;
}

static s32 extMusicIsSuffixDelimiter(char c)
{
	return c == '_' || c == '-';
}

static s32 extMusicStemBelongsToLongerKnownKey(const char *filename, size_t stemlen, const char *key)
{
	size_t keylen = strlen(key);

	for (s32 i = 0; i < ARRAYCOUNT(g_ExtMusicKnownKeys); i++) {
		const char *knownkey = g_ExtMusicKnownKeys[i];
		size_t knownlen = strlen(knownkey);

		if (knownlen <= keylen) {
			continue;
		}

		if (knownlen > stemlen) {
			continue;
		}

		if (strncasecmp(filename, knownkey, knownlen) != 0) {
			continue;
		}

		if (stemlen == knownlen || extMusicIsSuffixDelimiter(filename[knownlen])) {
			return 1;
		}
	}

	return 0;
}

static void extMusicFreeDecoded(void)
{
	if (g_ExtMusicMenuPcm && g_ExtMusicMenuPcmOwned) {
		sysMemFree(g_ExtMusicMenuPcm);
	}

	g_ExtMusicMenuPcm = NULL;
	g_ExtMusicMenuPcmFrames = 0;
	g_ExtMusicMenuPcmOwned = 0;
	g_ExtMusicPlayhead = 0;
	g_ExtMusicLoopActiveTrack = 1;
}

static void extMusicResetLoadedTrack(void)
{
	if (g_ExtMusicMenuData) {
		sysMemFree(g_ExtMusicMenuData);
		g_ExtMusicMenuData = NULL;
		g_ExtMusicMenuSize = 0;
	}

	extMusicFreeDecoded();
	g_ExtMusicCurrentKey[0] = '\0';
	g_ExtMusicCurrentFile[0] = '\0';
	g_ExtMusicCurrentVariant = -1;
}

static void extMusicFreeDeathDecoded(void)
{
	if (g_ExtMusicDeathPcm && g_ExtMusicDeathPcmOwned) {
		sysMemFree(g_ExtMusicDeathPcm);
	}

	g_ExtMusicDeathPcm = NULL;
	g_ExtMusicDeathPcmFrames = 0;
	g_ExtMusicDeathPcmOwned = 0;
	g_ExtMusicDeathPlayhead = 0;
	g_ExtMusicDeathVolume = 0;
	g_ExtMusicDeathFile[0] = '\0';
}

static void extMusicFreeCache(void)
{
	for (s32 i = 0; i < g_ExtMusicCacheCount; i++) {
		if (g_ExtMusicCache[i].pcm) {
			sysMemFree(g_ExtMusicCache[i].pcm);
			g_ExtMusicCache[i].pcm = NULL;
		}

		g_ExtMusicCache[i].filepath[0] = '\0';
		g_ExtMusicCache[i].frames = 0;
	}

	g_ExtMusicCacheCount = 0;
}

static const char *extMusicKeyForTrack(s32 tracktype, s32 tracknum)
{
	(void)tracktype;

	switch (tracknum) {
	case MUSIC_TITLE1:
		return MD_MUSIC_KEY_OPENING_COMPANY_LOGOS;
	case MUSIC_TITLE2:
		return MD_MUSIC_KEY_MAIN_TITLE_LOGO;
	case MUSIC_MAINMENU:
		return MD_MUSIC_KEY_MAIN_MENU;
	case MUSIC_COMBATSIM_MENU:
		return MD_MUSIC_KEY_COMBAT_SIM_MENU;
	case MUSIC_TERMINATION_MENU:
		return MD_MUSIC_KEY_TERMINATION_MENU;
	case MUSIC_COMBATSIM_COMPLETE:
		return MD_MUSIC_KEY_COMBAT_SIM_COMPLETE;
	case MUSIC_DEATH_MP:
		return MD_MUSIC_KEY_MULTIPLAYER_DEATH;
	case MUSIC_MISSION_SUCCESS:
		return MD_MUSIC_KEY_MISSION_SUCCESS;
	case MUSIC_MISSION_UNKNOWN:
		return MD_MUSIC_KEY_MISSION_UNKNOWN;
	case MUSIC_MISSION_FAILED:
		return MD_MUSIC_KEY_MISSION_FAILED;
	case MUSIC_PAUSEMENU:
		return MD_MUSIC_KEY_PAUSE_MENU;
	case MUSIC_DEATH_SOLO:
		return MD_MUSIC_KEY_SOLO_DEATH;
	case MUSIC_CI_TRAINING:
		return MD_MUSIC_KEY_CI_TRAINING;
	case MUSIC_DARK_COMBAT:
		return MD_MUSIC_KEY_MP_DARK_COMBAT;
	case MUSIC_SKEDAR_MYSTERY:
		return MD_MUSIC_KEY_MP_SKEDAR_MYSTERY;
	case MUSIC_CI_OPERATIVE:
		return MD_MUSIC_KEY_MP_CI_OPERATIVE;
	case MUSIC_DATADYNE_ACTION:
		return MD_MUSIC_KEY_MP_DATADYNE_ACTION;
	case MUSIC_MAIAN_TEARS:
		return MD_MUSIC_KEY_MP_MAIAN_TEARS;
	case MUSIC_ALIEN_CONFLICT:
		return MD_MUSIC_KEY_MP_ALIEN_CONFLICT;
	case MUSIC_CI:
		return MD_MUSIC_KEY_MP_CI;
	case MUSIC_DEFECTION:
		return MD_MUSIC_KEY_MP_DD_CENTRAL;
	case MUSIC_DEFECTION_X:
		return MD_MUSIC_KEY_MP_DD_CENTRAL_X;
	case MUSIC_INVESTIGATION:
		return MD_MUSIC_KEY_MP_DD_RESEARCH;
	case MUSIC_INVESTIGATION_X:
		return MD_MUSIC_KEY_MP_DD_RESEARCH_X;
	case MUSIC_EXTRACTION:
		return MD_MUSIC_KEY_MP_DD_EXTRACTION;
	case MUSIC_EXTRACTION_X:
		return MD_MUSIC_KEY_MP_DD_EXTRACTION_X;
	case MUSIC_VILLA:
		return MD_MUSIC_KEY_MP_VILLA;
	case MUSIC_VILLA_X:
		return MD_MUSIC_KEY_MP_VILLA_X;
	case MUSIC_CHICAGO:
		return MD_MUSIC_KEY_MP_CHICAGO;
	case MUSIC_CHICAGO_X:
		return MD_MUSIC_KEY_MP_CHICAGO_X;
	case MUSIC_G5:
		return MD_MUSIC_KEY_MP_G5;
	case MUSIC_G5_X:
		return MD_MUSIC_KEY_MP_G5_X;
	case MUSIC_INFILTRATION:
		return MD_MUSIC_KEY_MP_A51_INFILTRATION;
	case MUSIC_INFILTRATION_X:
		return MD_MUSIC_KEY_MP_A51_INFILTRATION_X;
	case MUSIC_RESCUE:
		return MD_MUSIC_KEY_MP_A51_RESCUE;
	case MUSIC_RESCUE_X:
		return MD_MUSIC_KEY_MP_A51_RESCUE_X;
	case MUSIC_ESCAPE:
		return MD_MUSIC_KEY_MP_A51_ESCAPE;
	case MUSIC_ESCAPE_X:
		return MD_MUSIC_KEY_MP_A51_ESCAPE_X;
	case MUSIC_AIRBASE:
		return MD_MUSIC_KEY_MP_AIR_BASE;
	case MUSIC_AIRBASE_X:
		return MD_MUSIC_KEY_MP_AIR_BASE_X;
	case MUSIC_AIRFORCEONE:
		return MD_MUSIC_KEY_MP_AIR_FORCE_ONE;
	case MUSIC_AIRFORCEONE_X:
		return MD_MUSIC_KEY_MP_AIR_FORCE_ONE_X;
	case MUSIC_CRASHSITE:
		return MD_MUSIC_KEY_MP_CRASH_SITE;
	case MUSIC_CRASHSITE_X:
		return MD_MUSIC_KEY_MP_CRASH_SITE_X;
	case MUSIC_PELAGIC:
		return MD_MUSIC_KEY_MP_PELAGIC_II;
	case MUSIC_PELAGIC_X:
		return MD_MUSIC_KEY_MP_PELAGIC_II_X;
	case MUSIC_DEEPSEA:
		return MD_MUSIC_KEY_MP_DEEP_SEA;
	case MUSIC_DEEPSEA_X:
		return MD_MUSIC_KEY_MP_DEEP_SEA_X;
	case MUSIC_DEFENSE:
		return MD_MUSIC_KEY_MP_INSTITUTE_DEFENSE;
	case MUSIC_DEFENSE_X:
		return MD_MUSIC_KEY_MP_INSTITUTE_DEFENSE_X;
	case MUSIC_ATTACKSHIP:
		return MD_MUSIC_KEY_MP_ATTACK_SHIP;
	case MUSIC_ATTACKSHIP_X:
		return MD_MUSIC_KEY_MP_ATTACK_SHIP_X;
	case MUSIC_SKEDARRUINS:
		return MD_MUSIC_KEY_MP_SKEDAR_RUINS;
	case MUSIC_SKEDARRUINS_X:
		return MD_MUSIC_KEY_MP_SKEDAR_RUINS_X;
	case MUSIC_CREDITS:
		return MD_MUSIC_KEY_MP_END_CREDITS;
	case MUSIC_SKEDARRUINS_KING:
		return MD_MUSIC_KEY_MP_SKEDAR_WARRIOR;
	default:
		return NULL;
	}
}

static s32 extMusicParseVariant(const char *filename, const char *key)
{
	size_t keylen;
	size_t extpos;
	const char *suffix;
	char *endptr;
	long value;
	u32 hash = 2166136261u;
	size_t i;

	if (!filename || !key) {
		return -2;
	}

	keylen = strlen(key);
	extpos = strlen(filename);

	if (extpos < 4 || strcasecmp(&filename[extpos - 4], ".mp3") != 0) {
		return -2;
	}

	extpos -= 4;

	if (strlen(filename) == keylen + 4 && strncasecmp(filename, key, keylen) == 0) {
		return -1;
	}

	if (extpos <= keylen + 1 || strncasecmp(filename, key, keylen) != 0 || !extMusicIsSuffixDelimiter(filename[keylen])) {
		return -2;
	}

	if (extMusicStemBelongsToLongerKnownKey(filename, extpos, key)) {
		return -2;
	}

	suffix = &filename[keylen + 1];
	value = strtol(suffix, &endptr, 10);

	if (*suffix == '\0') {
		return -2;
	}

	if (endptr == &filename[extpos] && value > 0 && value <= 9999) {
		return (s32)value;
	}

	for (i = keylen + 1; i < extpos; i++) {
		unsigned char c = (unsigned char)tolower((unsigned char)filename[i]);

		hash ^= c;
		hash *= 16777619u;
	}

	hash &= 0x7fffffff;

	if (hash == 0) {
		hash = 1;
	}

	return (s32)hash;
}

static s32 extMusicCollectMatchesRecursive(const char *rootpath, const char *subpath, const char *key, struct extmusicmatch *matches, s32 maxmatches, s32 *count)
{
	char scanpath[FS_MAXPATH + 1];
	DIR *dr;
	struct dirent *de;

	if (subpath != NULL && subpath[0] != '\0') {
		snprintf(scanpath, sizeof(scanpath), "%s/%s", rootpath, subpath);
	} else {
		snprintf(scanpath, sizeof(scanpath), "%s", rootpath);
	}

	dr = opendir(scanpath);

	if (!dr) {
		return 0;
	}

	while ((de = readdir(dr)) != NULL) {
		const char *name = de->d_name;
		char relpath[FS_MAXPATH + 1];
		char filepath[FS_MAXPATH + 1];
		struct stat stbuf;

		if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
			continue;
		}

		if (subpath != NULL && subpath[0] != '\0') {
			snprintf(relpath, sizeof(relpath), "%s/%s", subpath, name);
		} else {
			snprintf(relpath, sizeof(relpath), "%s", name);
		}

		snprintf(filepath, sizeof(filepath), "%s/%s", rootpath, relpath);

		if (stat(filepath, &stbuf) == -1) {
			continue;
		}

		if (S_ISDIR(stbuf.st_mode)) {
			extMusicCollectMatchesRecursive(rootpath, relpath, key, matches, maxmatches, count);
			continue;
		}

		if (count && *count < maxmatches) {
			s32 variant;

			if (key == NULL || key[0] == '\0') {
				size_t len = strlen(name);

				if (len < 4 || strcasecmp(&name[len - 4], ".mp3") != 0) {
					continue;
				}

				variant = -1;
			} else {
				variant = extMusicParseVariant(name, key);
			}

			if (variant >= -1) {
				strncpy(matches[*count].filepath, filepath, FS_MAXPATH);
				matches[*count].filepath[FS_MAXPATH] = '\0';
				matches[*count].variant = variant;
				(*count)++;
			}
		}
	}

	closedir(dr);
	return 1;
}

static s32 extMusicResolveFileForKey(const char *key, char *filepath, size_t filepathlen)
{
	char candidate[FS_MAXPATH + 1];
	struct extmusicmatch matches[64];
	s32 count = 0;
	s32 directindex = -1;
	s32 variantindexes[64];
	s32 variantcount = 0;
	s32 chosenindex;

	extMusicCollectMatchesRecursive(g_ExtMusicPath, "", key, matches, ARRAYCOUNT(matches), &count);

	for (s32 i = 0; i < count; i++) {
		if (matches[i].variant == -1 && directindex < 0) {
			directindex = i;
		} else if (matches[i].variant > 0) {
			variantindexes[variantcount++] = i;
		}
	}

	if (variantcount > 0) {
		chosenindex = variantindexes[rngRandom() % variantcount];

		if (variantcount > 1 && matches[chosenindex].variant == g_ExtMusicCurrentVariant) {
			s32 currentpos = -1;

			for (s32 i = 0; i < variantcount; i++) {
				if (matches[variantindexes[i]].variant == g_ExtMusicCurrentVariant) {
					currentpos = i;
					break;
				}
			}

			if (currentpos >= 0) {
				chosenindex = variantindexes[(currentpos + 1 + rngRandom() % (variantcount - 1)) % variantcount];
			}
		}

		strncpy(filepath, matches[chosenindex].filepath, filepathlen - 1);
		filepath[filepathlen - 1] = '\0';
		g_ExtMusicCurrentVariant = matches[chosenindex].variant;
		return 1;
	}

	if (directindex >= 0) {
		strncpy(filepath, matches[directindex].filepath, filepathlen - 1);
		filepath[filepathlen - 1] = '\0';
		g_ExtMusicCurrentVariant = -1;
		return 1;
	}

	if (strcmp(key, MD_MUSIC_KEY_MAIN_MENU) == 0) {
		count = 0;
		extMusicCollectMatchesRecursive(g_ExtMusicPath, "", "menu", matches, ARRAYCOUNT(matches), &count);

		for (s32 i = 0; i < count; i++) {
			if (matches[i].variant == -1) {
				strncpy(filepath, matches[i].filepath, filepathlen - 1);
				filepath[filepathlen - 1] = '\0';
				g_ExtMusicCurrentVariant = -1;
				return 1;
			}
		}
	}

	return 0;
}

static s32 extMusicCacheLookup(const char *filepath, s16 **outPcm, u32 *outFrames)
{
	for (s32 i = 0; i < g_ExtMusicCacheCount; i++) {
		if (strcmp(g_ExtMusicCache[i].filepath, filepath) == 0) {
			if (outPcm) {
				*outPcm = g_ExtMusicCache[i].pcm;
			}

			if (outFrames) {
				*outFrames = g_ExtMusicCache[i].frames;
			}

			return 1;
		}
	}

	return 0;
}

static s32 extMusicCacheStore(const char *filepath, s16 *pcm, u32 frames)
{
	if (!filepath || !filepath[0] || !pcm || frames == 0 || g_ExtMusicCacheCount >= EXT_MUSIC_MAX_CACHE) {
		return 0;
	}

	strncpy(g_ExtMusicCache[g_ExtMusicCacheCount].filepath, filepath, FS_MAXPATH);
	g_ExtMusicCache[g_ExtMusicCacheCount].filepath[FS_MAXPATH] = '\0';
	g_ExtMusicCache[g_ExtMusicCacheCount].pcm = pcm;
	g_ExtMusicCache[g_ExtMusicCacheCount].frames = frames;
	g_ExtMusicCacheCount++;
	return 1;
}

#ifdef __APPLE__
static s32 extMusicDecodePcmApple(const char *filepath, s16 **outPcm, u32 *outFrames)
{
	CFURLRef url = NULL;
	ExtAudioFileRef file = NULL;
	AudioStreamBasicDescription input = {0};
	AudioStreamBasicDescription output = {0};
	UInt32 size;
	SInt64 sourceFrames = 0;
	s16 *decoded = NULL;
	u32 decodedFrames = 0;
	u32 capacityFrames = 0;
	OSStatus status;

	url = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8 *)filepath, strlen(filepath), false);
	if (!url) {
		sysLogPrintf(LOG_ERROR, "ext_music: failed to create CFURL for %s", filepath);
		return 0;
	}

	status = ExtAudioFileOpenURL(url, &file);
	CFRelease(url);

	if (status != noErr || !file) {
		sysLogPrintf(LOG_ERROR, "ext_music: ExtAudioFileOpenURL failed (%d)", (s32)status);
		return 0;
	}

	size = sizeof(input);
	status = ExtAudioFileGetProperty(file, kExtAudioFileProperty_FileDataFormat, &size, &input);
	if (status != noErr) {
		sysLogPrintf(LOG_ERROR, "ext_music: failed to read source format (%d)", (s32)status);
		ExtAudioFileDispose(file);
		return 0;
	}

	size = sizeof(sourceFrames);
	status = ExtAudioFileGetProperty(file, kExtAudioFileProperty_FileLengthFrames, &size, &sourceFrames);
	if (status != noErr || sourceFrames <= 0) {
		sourceFrames = 0;
	}

	output.mSampleRate = EXT_MUSIC_TARGET_HZ;
	output.mFormatID = kAudioFormatLinearPCM;
	output.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
	output.mBytesPerPacket = sizeof(s16);
	output.mFramesPerPacket = 1;
	output.mBytesPerFrame = sizeof(s16);
	output.mChannelsPerFrame = 1;
	output.mBitsPerChannel = 16;

	status = ExtAudioFileSetProperty(file, kExtAudioFileProperty_ClientDataFormat, sizeof(output), &output);
	if (status != noErr) {
		sysLogPrintf(LOG_ERROR, "ext_music: failed to set output format (%d)", (s32)status);
		ExtAudioFileDispose(file);
		return 0;
	}

	if (sourceFrames > 0 && input.mSampleRate > 0) {
		capacityFrames = (u32)(((u64)sourceFrames * EXT_MUSIC_TARGET_HZ) / (u32)input.mSampleRate) + 4096;
	} else {
		capacityFrames = 65536;
	}

	if (capacityFrames > EXT_MUSIC_MAX_DECODED_FRAMES) {
		capacityFrames = EXT_MUSIC_MAX_DECODED_FRAMES;
	}

	decoded = sysMemAlloc(capacityFrames * sizeof(*decoded));
	if (!decoded) {
		sysLogPrintf(LOG_ERROR, "ext_music: failed to allocate Apple decode buffer");
		ExtAudioFileDispose(file);
		return 0;
	}

	for (;;) {
		AudioBufferList buffers;
		UInt32 chunkFrames = 4096;

		if (decodedFrames + chunkFrames > capacityFrames) {
			u32 newCapacity = capacityFrames * 2;
			s16 *newDecoded;

			if (capacityFrames >= EXT_MUSIC_MAX_DECODED_FRAMES
					|| decodedFrames + chunkFrames > EXT_MUSIC_MAX_DECODED_FRAMES) {
				sysLogPrintf(LOG_ERROR, "ext_music: decoded MP3 exceeds %u frames", EXT_MUSIC_MAX_DECODED_FRAMES);
				sysMemFree(decoded);
				ExtAudioFileDispose(file);
				return 0;
			}

			while (decodedFrames + chunkFrames > newCapacity) {
				newCapacity *= 2;
			}

			if (newCapacity > EXT_MUSIC_MAX_DECODED_FRAMES) {
				newCapacity = EXT_MUSIC_MAX_DECODED_FRAMES;
			}

			newDecoded = sysMemRealloc(decoded, newCapacity * sizeof(*decoded));
			if (!newDecoded) {
				sysLogPrintf(LOG_ERROR, "ext_music: failed to grow Apple decode buffer");
				sysMemFree(decoded);
				ExtAudioFileDispose(file);
				return 0;
			}

			decoded = newDecoded;
			capacityFrames = newCapacity;
		}

		buffers.mNumberBuffers = 1;
		buffers.mBuffers[0].mNumberChannels = 1;
		buffers.mBuffers[0].mDataByteSize = chunkFrames * sizeof(*decoded);
		buffers.mBuffers[0].mData = decoded + decodedFrames;

		status = ExtAudioFileRead(file, &chunkFrames, &buffers);
		if (status != noErr) {
			sysLogPrintf(LOG_ERROR, "ext_music: ExtAudioFileRead failed (%d)", (s32)status);
			sysMemFree(decoded);
			ExtAudioFileDispose(file);
			return 0;
		}

		if (chunkFrames == 0) {
			break;
		}

		decodedFrames += chunkFrames;
	}

	ExtAudioFileDispose(file);

	if (decodedFrames == 0) {
		sysLogPrintf(LOG_ERROR, "ext_music: Apple decoder produced no frames");
		sysMemFree(decoded);
		return 0;
	}

	*outPcm = decoded;
	*outFrames = decodedFrames;
	sysLogPrintf(
		LOG_NOTE,
		"ext_music: Apple decoder produced %u mono frames at %d Hz from source rate %.0f",
		decodedFrames,
		EXT_MUSIC_TARGET_HZ,
		input.mSampleRate);
	return 1;
}

static s32 extMusicDecodeMenuPcmApple(const char *filepath)
{
	if (!extMusicDecodePcmApple(filepath, &g_ExtMusicMenuPcm, &g_ExtMusicMenuPcmFrames)) {
		return 0;
	}

	g_ExtMusicMenuPcmOwned = 1;
	g_ExtMusicPlayhead = 0;
	return 1;
}
#endif

static s32 extMusicDecodeMenuPcm(void)
{
	mp3dec_t dec;
	mp3d_sample_t decoded[MINIMP3_MAX_SAMPLES_PER_FRAME];
	mp3dec_frame_info_t info;
	s16 *sourceMono = NULL;
	s16 *resampled = NULL;
	u32 sourceCapacity = 0;
	u32 sourceFrames = 0;
	u32 dataptr = 0;
	s32 sourceHz = 0;

	if (g_ExtMusicMenuPcm) {
		return 1;
	}

	if (!g_ExtMusicMenuData || g_ExtMusicMenuSize == 0) {
		return false;
	}

	mp3dec_init(&dec);

	while (dataptr < g_ExtMusicMenuSize) {
		const s32 samples = mp3dec_decode_frame(
			&dec,
			(const u8 *)g_ExtMusicMenuData + dataptr,
			g_ExtMusicMenuSize - dataptr,
			decoded,
			&info);

		if (info.frame_bytes <= 0) {
			break;
		}

		dataptr += info.frame_bytes;

		if (samples <= 0 || info.channels <= 0) {
			continue;
		}

		if (info.hz > 0) {
			sourceHz = info.hz;
		}

		if (sourceFrames + (u32)samples > sourceCapacity) {
			u32 newCapacity = sourceCapacity ? sourceCapacity * 2 : 8192;
			u32 maxSourceFrames = EXT_MUSIC_MAX_DECODED_FRAMES;

			if (sourceHz > 0) {
				maxSourceFrames = (u32)(((u64)EXT_MUSIC_MAX_DECODED_FRAMES * (u32)sourceHz) / EXT_MUSIC_TARGET_HZ);
			}

			if (sourceFrames + (u32)samples > maxSourceFrames) {
				sysMemFree(sourceMono);
				sysLogPrintf(LOG_ERROR, "ext_music: decoded MP3 exceeds %u source frames", maxSourceFrames);
				return 0;
			}

			while (newCapacity < sourceFrames + (u32)samples) {
				newCapacity *= 2;
			}

			sourceMono = sysMemRealloc(sourceMono, newCapacity * sizeof(*sourceMono));

			if (!sourceMono) {
				sysLogPrintf(LOG_ERROR, "ext_music: failed to grow decode buffer");
				return 0;
			}

			sourceCapacity = newCapacity;
		}

		for (s32 i = 0; i < samples; i++) {
			if (info.channels >= 2) {
				const s32 base = i * info.channels;
				sourceMono[sourceFrames + i] = (s16)(((s32)decoded[base] + (s32)decoded[base + 1]) / 2);
			} else {
				sourceMono[sourceFrames + i] = decoded[i];
			}
		}

		sourceFrames += samples;
	}

	if (!sourceMono || sourceFrames == 0 || sourceHz <= 0) {
		sysMemFree(sourceMono);
		sysLogPrintf(LOG_ERROR, "ext_music: failed to decode any usable PCM from menu override");
		return 0;
	}

	{
		const u32 targetFrames = (u32)(((u64)sourceFrames * EXT_MUSIC_TARGET_HZ) / (u32)sourceHz);
		u32 phase = 0;
		const u32 phaseStep = (u32)(((u64)sourceHz << EXT_MUSIC_PHASE_BITS) / EXT_MUSIC_TARGET_HZ);

		if (targetFrames > EXT_MUSIC_MAX_DECODED_FRAMES) {
			sysMemFree(sourceMono);
			sysLogPrintf(LOG_ERROR, "ext_music: resampled MP3 exceeds %u frames", EXT_MUSIC_MAX_DECODED_FRAMES);
			return 0;
		}

		resampled = sysMemAlloc((targetFrames + 1) * sizeof(*resampled));

		if (!resampled) {
			sysMemFree(sourceMono);
			sysLogPrintf(LOG_ERROR, "ext_music: failed to allocate decoded PCM buffer");
			return 0;
		}

		for (u32 i = 0; i < targetFrames; i++) {
			const u32 idx = phase >> EXT_MUSIC_PHASE_BITS;
			const u32 frac = phase & (EXT_MUSIC_PHASE_ONE - 1);
			const s32 s0 = sourceMono[idx < sourceFrames ? idx : sourceFrames - 1];
			const s32 s1 = sourceMono[idx + 1 < sourceFrames ? idx + 1 : sourceFrames - 1];
			resampled[i] = (s16)(s0 + (((s1 - s0) * (s32)frac) >> EXT_MUSIC_PHASE_BITS));
			phase += phaseStep;
		}

		g_ExtMusicMenuPcm = resampled;
		g_ExtMusicMenuPcmFrames = targetFrames;
		g_ExtMusicMenuPcmOwned = 1;
		g_ExtMusicPlayhead = 0;

		sysLogPrintf(
			LOG_NOTE,
			"ext_music: decoded menu override to %u mono frames at %d Hz from %u source frames at %d Hz",
			g_ExtMusicMenuPcmFrames,
			EXT_MUSIC_TARGET_HZ,
			sourceFrames,
			sourceHz);
	}

	sysMemFree(sourceMono);
	return 1;
}

void extMusicInit(void)
{
	const char *resolvedPath;

	if (g_ExtMusicInitDone) {
		return;
	}

	resolvedPath = fsFullPath(EXT_MUSIC_DIR);

	if (resolvedPath && fsFileSize(resolvedPath) >= 0) {
		strncpy(g_ExtMusicPath, resolvedPath, sizeof(g_ExtMusicPath) - 1);
		g_ExtMusicDirExists = 1;
	} else if (fsFileSize("./" EXT_MUSIC_DIR) >= 0) {
		strncpy(g_ExtMusicPath, "./" EXT_MUSIC_DIR, sizeof(g_ExtMusicPath) - 1);
		g_ExtMusicDirExists = 1;
	} else {
		g_ExtMusicPath[0] = '\0';
		g_ExtMusicDirExists = 0;
	}

	g_ExtMusicInitDone = 1;

	if (g_ExtMusicDirExists) {
		sysLogPrintf(LOG_NOTE, "ext_music: directory found at %s", g_ExtMusicPath);
	} else {
		sysLogPrintf(LOG_NOTE, "ext_music: directory not found");
	}
}

void extMusicFree(void)
{
	extMusicStopAll();

	if (g_ExtMusicMenuData) {
		sysMemFree(g_ExtMusicMenuData);
		g_ExtMusicMenuData = NULL;
		g_ExtMusicMenuSize = 0;
	}

	extMusicFreeDecoded();
	extMusicFreeDeathDecoded();
	extMusicFreeCache();

	g_ExtMusicPath[0] = '\0';
	g_ExtMusicDirExists = 0;
	g_ExtMusicInitDone = 0;
}

s32 extMusicDirExists(void)
{
	return g_ExtMusicDirExists;
}

s32 extMusicPreloadAll(void)
{
	struct extmusicmatch matches[256];
	s32 loaded = 0;
	s32 count = 0;

	if (!g_ExtMusicDirExists || !g_ExtMusicPath[0]) {
		return 0;
	}

	extMusicCollectMatchesRecursive(g_ExtMusicPath, "", "", matches, ARRAYCOUNT(matches), &count);

	for (s32 i = 0; i < count; i++) {
		s16 *pcm = NULL;
		u32 frames = 0;

		if (extMusicCacheLookup(matches[i].filepath, NULL, NULL)) {
			continue;
		}

		if (!extMusicFileWithinLimit(matches[i].filepath)) {
			continue;
		}

#ifdef __APPLE__
		if (!extMusicDecodePcmApple(matches[i].filepath, &pcm, &frames)) {
			continue;
		}
#else
		continue;
#endif

		if (!extMusicCacheStore(matches[i].filepath, pcm, frames)) {
			sysMemFree(pcm);
			break;
		}

		loaded++;
		sysLogPrintf(LOG_NOTE, "ext_music: preloaded %s (%u frames)", matches[i].filepath, frames);
	}

	sysLogPrintf(LOG_NOTE, "ext_music: preload complete, cached %d file(s)", loaded);
	return loaded;
}

static s32 extMusicEnsureTrackLoaded(const char *key, const char *filepath)
{
	if (g_ExtMusicMenuPcm != NULL && strcmp(g_ExtMusicCurrentFile, filepath) == 0) {
		sysLogPrintf(LOG_NOTE, "ext_music: track override already resident key=%s file=%s frames=%u", key, filepath, g_ExtMusicMenuPcmFrames);
		return 1;
	}

	{
		s16 *cachedPcm = NULL;
		u32 cachedFrames = 0;

		if (extMusicCacheLookup(filepath, &cachedPcm, &cachedFrames)) {
			extMusicResetLoadedTrack();
			strncpy(g_ExtMusicCurrentKey, key, sizeof(g_ExtMusicCurrentKey) - 1);
			g_ExtMusicCurrentKey[sizeof(g_ExtMusicCurrentKey) - 1] = '\0';
			strncpy(g_ExtMusicCurrentFile, filepath, sizeof(g_ExtMusicCurrentFile) - 1);
			g_ExtMusicCurrentFile[sizeof(g_ExtMusicCurrentFile) - 1] = '\0';
			g_ExtMusicMenuPcm = cachedPcm;
			g_ExtMusicMenuPcmFrames = cachedFrames;
			g_ExtMusicMenuPcmOwned = 0;
			g_ExtMusicPlayhead = 0;
			sysLogPrintf(LOG_NOTE, "ext_music: using preloaded track key=%s file=%s frames=%u", key, filepath, g_ExtMusicMenuPcmFrames);
			return 1;
		}
	}

	if (!g_ExtMusicPath[0]) {
		sysLogPrintf(LOG_NOTE, "ext_music: no resolved path while loading track override");
		return 0;
	}

	if (!extMusicFileWithinLimit(filepath)) {
		return 0;
	}

	extMusicResetLoadedTrack();
	strncpy(g_ExtMusicCurrentKey, key, sizeof(g_ExtMusicCurrentKey) - 1);
	g_ExtMusicCurrentKey[sizeof(g_ExtMusicCurrentKey) - 1] = '\0';
	strncpy(g_ExtMusicCurrentFile, filepath, sizeof(g_ExtMusicCurrentFile) - 1);
	g_ExtMusicCurrentFile[sizeof(g_ExtMusicCurrentFile) - 1] = '\0';
	sysLogPrintf(LOG_NOTE, "ext_music: attempting to load track override key=%s file=%s", key, filepath);

#ifdef __APPLE__
	if (extMusicDecodeMenuPcmApple(filepath)) {
		return 1;
	}
#endif

	g_ExtMusicMenuData = fsFileLoad(filepath, &g_ExtMusicMenuSize);

	if (!g_ExtMusicMenuData) {
		sysLogPrintf(LOG_NOTE, "ext_music: track override missing: %s", filepath);
		extMusicResetLoadedTrack();
		return 0;
	}

	sysLogPrintf(LOG_NOTE, "ext_music: loaded track override %s (%u bytes)", filepath, g_ExtMusicMenuSize);

	if (!extMusicDecodeMenuPcm()) {
		extMusicResetLoadedTrack();
		return 0;
	}

	return 1;
}

s32 extMusicStartForKey(const char *key, s32 tracktype, s32 volume)
{
	char filepath[FS_MAXPATH + 1];

	if (!g_ExtMusicDirExists || !key || key[0] == '\0') {
		return 0;
	}

	if (!extMusicResolveFileForKey(key, filepath, sizeof(filepath))) {
		sysLogPrintf(LOG_NOTE, "ext_music: no file for custom key=%s", key);
		return 0;
	}

	if (!extMusicEnsureTrackLoaded(key, filepath)) {
		sysLogPrintf(LOG_NOTE, "ext_music: custom key could not be loaded: %s", key);
		return 0;
	}

	g_ExtMusicPlayhead = 0;
	g_ExtMusicVolume = volume;
	g_ExtMusicActiveTracktype = tracktype;
	g_ExtMusicLoopActiveTrack = 1;
	audioClearBuffered();

	sysLogPrintf(LOG_NOTE, "ext_music: started custom track key=%s file=%s tracktype=%d volume=%d frames=%u",
			key, filepath, tracktype, volume, g_ExtMusicMenuPcmFrames);
	return 1;
}

s32 extMusicStartForEvent(s32 tracktype, s32 tracknum, s32 volume)
{
	const char *key;
	char filepath[FS_MAXPATH + 1];
	s32 shouldloop = 1;

	sysLogPrintf(
		LOG_NOTE,
		"ext_music: start request tracktype=%d tracknum=%d volume=%d active=%d dir=%d",
		tracktype,
		tracknum,
		volume,
		g_ExtMusicActiveTracktype,
		g_ExtMusicDirExists ? 1 : 0);

	if (!g_ExtMusicDirExists) {
		sysLogPrintf(LOG_NOTE, "ext_music: start denied because directory is unavailable");
		return 0;
	}

	key = extMusicKeyForTrack(tracktype, tracknum);

	if (!key) {
		sysLogPrintf(LOG_NOTE, "ext_music: no override key for tracktype=%d tracknum=%d", tracktype, tracknum);
		return 0;
	}

	if (!extMusicResolveFileForKey(key, filepath, sizeof(filepath))) {
		sysLogPrintf(LOG_NOTE, "ext_music: no file for key=%s", key);
		return 0;
	}

	if (tracktype == TRACKTYPE_DEATH) {
#ifdef __APPLE__
		if (!extMusicFileWithinLimit(filepath)) {
			return 0;
		}

		if (strcmp(g_ExtMusicDeathFile, filepath) != 0 || !g_ExtMusicDeathPcm || g_ExtMusicDeathPcmFrames == 0) {
			extMusicFreeDeathDecoded();

			if (!extMusicCacheLookup(filepath, &g_ExtMusicDeathPcm, &g_ExtMusicDeathPcmFrames)) {
				if (!extMusicDecodePcmApple(filepath, &g_ExtMusicDeathPcm, &g_ExtMusicDeathPcmFrames)) {
					sysLogPrintf(LOG_NOTE, "ext_music: death overlay could not be decoded for key=%s", key);
					return 0;
				}

				g_ExtMusicDeathPcmOwned = 1;
			}

			strncpy(g_ExtMusicDeathFile, filepath, sizeof(g_ExtMusicDeathFile) - 1);
			g_ExtMusicDeathFile[sizeof(g_ExtMusicDeathFile) - 1] = '\0';
		}

		g_ExtMusicDeathPlayhead = 0;
		g_ExtMusicDeathVolume = volume;
		sysLogPrintf(LOG_NOTE, "ext_music: started external death overlay key=%s file=%s volume=%d frames=%u", key, filepath, volume, g_ExtMusicDeathPcmFrames);
		return 1;
#else
		sysLogPrintf(LOG_NOTE, "ext_music: death overlay requires Apple decoder path");
		return 0;
#endif
	}

	if (!extMusicEnsureTrackLoaded(key, filepath)) {
		sysLogPrintf(LOG_NOTE, "ext_music: track override could not be loaded for key=%s", key);
		return 0;
	}

	if (g_ExtMusicActiveTracktype == tracktype
			&& strcmp(g_ExtMusicCurrentFile, filepath) == 0
			&& tracktype != TRACKTYPE_DEATH) {
		g_ExtMusicVolume = volume;

		if (tracktype == TRACKTYPE_PRIMARY && g_Vars.normmplayerisrunning && g_ExtMusicMenuPcmFrames > 0) {
			g_MusicLife60 = (s32)(((u64)g_ExtMusicMenuPcmFrames * TICKS(60) + EXT_MUSIC_TARGET_HZ - 1) / EXT_MUSIC_TARGET_HZ);
		}

		sysLogPrintf(LOG_NOTE, "ext_music: track key=%s already active, keeping playhead=%u", key, g_ExtMusicPlayhead);
		return 1;
	}

	g_ExtMusicPlayhead = 0;
	g_ExtMusicVolume = volume;
	g_ExtMusicActiveTracktype = tracktype;

	if (tracktype == TRACKTYPE_PRIMARY && g_Vars.normmplayerisrunning) {
		/*
		 * MP music has two different policies:
		 * - single selected tracks and Random should loop the chosen song
		 * - active multi-track rotation should end naturally so the MP music
		 *   scheduler can switch tracks
		 */
		shouldloop = !g_MpEnableMusicSwitching;
	}

	g_ExtMusicLoopActiveTrack = shouldloop;
	audioClearBuffered();

	if (tracktype == TRACKTYPE_PRIMARY && g_Vars.normmplayerisrunning && g_ExtMusicMenuPcmFrames > 0) {
		g_MusicLife60 = (s32)(((u64)g_ExtMusicMenuPcmFrames * TICKS(60) + EXT_MUSIC_TARGET_HZ - 1) / EXT_MUSIC_TARGET_HZ);
	}

	sysLogPrintf(LOG_NOTE, "ext_music: started external track key=%s file=%s tracktype=%d volume=%d frames=%u", key, filepath, tracktype, volume, g_ExtMusicMenuPcmFrames);
	return 1;
}

s32 extMusicStartForTrack(s32 tracktype, s32 volume)
{
	return 0;
}

void extMusicStopForTrack(s32 tracktype)
{
	if (tracktype == TRACKTYPE_DEATH) {
		g_ExtMusicDeathPlayhead = 0;
		g_ExtMusicDeathVolume = 0;
		sysLogPrintf(LOG_NOTE, "ext_music: stopped external death overlay");
		return;
	}

	if (g_ExtMusicActiveTracktype != tracktype) {
		return;
	}

	audioClearBuffered();
	g_ExtMusicActiveTracktype = TRACKTYPE_NONE;
	g_ExtMusicPlayhead = 0;
	g_ExtMusicVolume = 0;
	g_ExtMusicLoopActiveTrack = 1;
	sysLogPrintf(LOG_NOTE, "ext_music: stopped external track for type %d", tracktype);
}

void extMusicStopAll(void)
{
	if (g_ExtMusicActiveTracktype == TRACKTYPE_NONE) {
		return;
	}

	audioClearBuffered();
	g_ExtMusicActiveTracktype = TRACKTYPE_NONE;
	g_ExtMusicPlayhead = 0;
	g_ExtMusicVolume = 0;
	g_ExtMusicLoopActiveTrack = 1;
	g_ExtMusicDeathPlayhead = 0;
	g_ExtMusicDeathVolume = 0;
	sysLogPrintf(LOG_NOTE, "ext_music: stopped all external music");
}

s32 extMusicIsAnyTrackActive(void)
{
	return g_ExtMusicActiveTracktype != TRACKTYPE_NONE || g_ExtMusicDeathVolume > 0;
}

s32 extMusicIsTrackActive(s32 tracktype)
{
	if (tracktype == TRACKTYPE_DEATH) {
		return g_ExtMusicDeathVolume > 0;
	}

	return g_ExtMusicActiveTracktype == tracktype;
}

s32 extMusicMixIntoBuffer(s16 *samples, u32 frames)
{
	if (!samples || frames == 0) {
		return 0;
	}

	if (g_ExtMusicActiveTracktype != TRACKTYPE_NONE && g_ExtMusicMenuPcm && g_ExtMusicMenuPcmFrames > 0) {
		for (u32 i = 0; i < frames; i++) {
			const s32 music = ((g_ExtMusicMenuPcm[g_ExtMusicPlayhead] * g_ExtMusicVolume) / AL_VOL_FULL) >> EXT_MUSIC_HEADROOM_SHIFT;
			const u32 base = i * 2;

			samples[base + 0] = extMusicClamp16((s32)samples[base + 0] + music);
			samples[base + 1] = extMusicClamp16((s32)samples[base + 1] + music);

			g_ExtMusicPlayhead++;

			if (g_ExtMusicPlayhead >= g_ExtMusicMenuPcmFrames) {
				if (g_ExtMusicLoopActiveTrack) {
					g_ExtMusicPlayhead = 0;
				} else {
					g_ExtMusicActiveTracktype = TRACKTYPE_NONE;
					g_ExtMusicPlayhead = 0;
					g_ExtMusicVolume = 0;
					g_ExtMusicLoopActiveTrack = 1;
					sysLogPrintf(LOG_NOTE, "ext_music: non-looping track finished");
					break;
				}
			}
		}
	}

	if (g_ExtMusicDeathVolume > 0 && g_ExtMusicDeathPcm && g_ExtMusicDeathPcmFrames > 0) {
		for (u32 i = 0; i < frames; i++) {
			const s32 music = ((g_ExtMusicDeathPcm[g_ExtMusicDeathPlayhead] * g_ExtMusicDeathVolume) / AL_VOL_FULL) >> EXT_MUSIC_HEADROOM_SHIFT;
			const u32 base = i * 2;

			samples[base + 0] = extMusicClamp16((s32)samples[base + 0] + music);
			samples[base + 1] = extMusicClamp16((s32)samples[base + 1] + music);

			g_ExtMusicDeathPlayhead++;

			if (g_ExtMusicDeathPlayhead >= g_ExtMusicDeathPcmFrames) {
				g_ExtMusicDeathPlayhead = 0;
				g_ExtMusicDeathVolume = 0;
				break;
			}
		}
	}

	return 1;
}
