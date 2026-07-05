#ifndef _IN_EXT_MUSIC_H
#define _IN_EXT_MUSIC_H

#include <PR/ultratypes.h>

void extMusicInit(void);
void extMusicFree(void);
s32 extMusicDirExists(void);
s32 extMusicPreloadAll(void);
s32 extMusicStartForEvent(s32 tracktype, s32 tracknum, s32 volume);
s32 extMusicStartForTrack(s32 tracktype, s32 volume);
s32 extMusicStartForKey(const char *key, s32 tracktype, s32 volume);
void extMusicStopForTrack(s32 tracktype);
void extMusicStopAll(void);
s32 extMusicIsAnyTrackActive(void);
s32 extMusicIsTrackActive(s32 tracktype);
s32 extMusicMixIntoBuffer(s16 *samples, u32 frames);

#endif
