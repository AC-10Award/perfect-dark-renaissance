#include <PR/ultratypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include "platform.h"
#include "config.h"
#include "audio.h"
#include "system.h"
#include "ext_music.h"
#include "ext_sfx.h"
#include "constants.h"

static SDL_AudioDeviceID dev;
static const s16 *nextBuf;
static u32 nextSize = 0;
static u8 *queuedBuf;
static u32 queuedBufSize = 0;
static u8 *ringBuf;
static u32 ringBufSize = 0;
static u32 ringReadPos = 0;
static u32 ringWritePos = 0;
static u32 ringBytesUsed = 0;
static u32 staleAudioDiscardWarnings = 0;

static s32 bufferSize = 2048;
static s32 queueLimit = 8192;

static void audioDiscardOldest(u32 bytes)
{
	if (!ringBuf || ringBufSize == 0 || bytes == 0) {
		return;
	}

	bytes = (bytes + 3) & ~3;

	if (bytes > ringBytesUsed) {
		bytes = ringBytesUsed;
	}

	ringReadPos = (ringReadPos + bytes) % ringBufSize;
	ringBytesUsed -= bytes;

	if (ringBytesUsed == 0) {
		ringWritePos = ringReadPos;
	}
}

static void audioCallback(void *userdata, Uint8 *stream, int len)
{
	u32 copied = 0;

	(void)userdata;

	memset(stream, 0, len);

	while (copied < (u32)len && ringBytesUsed > 0) {
		u32 srcChunk = ringBufSize - ringReadPos;
		u32 todo = ringBytesUsed;

		if (todo > (u32)len - copied) {
			todo = (u32)len - copied;
		}

		if (todo > srcChunk) {
			todo = srcChunk;
		}

		memcpy(stream + copied, ringBuf + ringReadPos, todo);
		ringReadPos = (ringReadPos + todo) % ringBufSize;
		ringBytesUsed -= todo;
		copied += todo;
	}

	extMusicMixIntoBuffer((s16 *)stream, (u32)len / 4);
	extSfxMixIntoBuffer((s16 *)stream, (u32)len / 4);
}

s32 audioInit(void)
{
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
		sysLogPrintf(LOG_ERROR, "SDL audio init error: %s", SDL_GetError());
		return -1;
	}

	SDL_AudioSpec want, have;
	SDL_zero(want);
	want.freq = 22020; // TODO: this might cause trouble for some platforms
	want.format = AUDIO_S16SYS;
	want.channels = 2;
	want.samples = bufferSize;
	want.callback = audioCallback;
	want.userdata = NULL;

	nextBuf = NULL;
	queuedBuf = NULL;
	queuedBufSize = 0;
	ringBuf = NULL;
	ringBufSize = queueLimit;
	ringReadPos = 0;
	ringWritePos = 0;
	ringBytesUsed = 0;
	staleAudioDiscardWarnings = 0;

	ringBuf = malloc(ringBufSize);
	if (!ringBuf) {
		sysLogPrintf(LOG_ERROR, "audio: failed to allocate ring buffer (%u bytes)", ringBufSize);
		return -1;
	}

	dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
	if (dev == 0) {
		sysLogPrintf(LOG_ERROR, "SDL_OpenAudio error: %s", SDL_GetError());
		free(ringBuf);
		ringBuf = NULL;
		ringBufSize = 0;
		return -1;
	}

	sysLogPrintf(LOG_NOTE, "audio: opened SDL device freq=%d channels=%d samples=%d", have.freq, have.channels, have.samples);

	SDL_PauseAudioDevice(dev, 0);

	return 0;
}

s32 audioGetBytesBuffered(void)
{
	return ringBytesUsed;
}

s32 audioGetSamplesBuffered(void)
{
	return audioGetBytesBuffered() / 4;
}

void audioSetNextBuffer(const s16 *buf, u32 len)
{
	if (len == 0 || buf == NULL) {
		nextBuf = NULL;
		nextSize = 0;
		return;
	}

	if (queuedBufSize < len) {
		u8 *newbuf = realloc(queuedBuf, len);

		if (!newbuf) {
			sysLogPrintf(LOG_ERROR, "audio: failed to grow queue buffer to %u bytes", len);
			nextBuf = NULL;
			nextSize = 0;
			return;
		}

		queuedBuf = newbuf;
		queuedBufSize = len;
	}

	memcpy(queuedBuf, buf, len);
	nextBuf = (const s16 *)queuedBuf;
	nextSize = len;
}

void audioClearBuffered(void)
{
	SDL_LockAudioDevice(dev);
	ringReadPos = 0;
	ringWritePos = 0;
	ringBytesUsed = 0;
	staleAudioDiscardWarnings = 0;
	SDL_UnlockAudioDevice(dev);
	nextBuf = NULL;
	nextSize = 0;
}

void audioEndFrame(void)
{
	if (nextBuf && nextSize) {
		SDL_LockAudioDevice(dev);
		if (ringBuf && nextSize <= ringBufSize) {
			if (nextSize > ringBufSize - ringBytesUsed) {
				u32 discard = nextSize - (ringBufSize - ringBytesUsed);

				audioDiscardOldest(discard);

				if (staleAudioDiscardWarnings < 8) {
					sysLogPrintf(LOG_WARNING,
							"audio: discarded %u old queued byte(s) to keep latency low (%u/%u)",
							discard, ringBytesUsed, ringBufSize);
					staleAudioDiscardWarnings++;
				}
			}

			u32 firstChunk = ringBufSize - ringWritePos;

			if (firstChunk > nextSize) {
				firstChunk = nextSize;
			}

			memcpy(ringBuf + ringWritePos, nextBuf, firstChunk);
			ringWritePos = (ringWritePos + firstChunk) % ringBufSize;
			ringBytesUsed += firstChunk;

			if (firstChunk < nextSize) {
				const u32 secondChunk = nextSize - firstChunk;
				memcpy(ringBuf + ringWritePos, (const u8 *)nextBuf + firstChunk, secondChunk);
				ringWritePos = (ringWritePos + secondChunk) % ringBufSize;
				ringBytesUsed += secondChunk;
			}
		} else if (ringBuf) {
			sysLogPrintf(LOG_WARNING, "audio: dropping %u-byte frame, larger than ring (%u)", nextSize, ringBufSize);
		}
		SDL_UnlockAudioDevice(dev);
		nextBuf = NULL;
		nextSize = 0;
	}
}

void audioLock(void)
{
	if (dev != 0) {
		SDL_LockAudioDevice(dev);
	}
}

void audioUnlock(void)
{
	if (dev != 0) {
		SDL_UnlockAudioDevice(dev);
	}
}

PD_CONSTRUCTOR static void audioConfigInit(void)
{
	configRegisterInt("Audio.BufferSize", &bufferSize, 0, 1 * 1024 * 1024);
	configRegisterInt("Audio.QueueLimit", &queueLimit, 0, 1 * 1024 * 1024);
}
