#ifndef _IN_EXT_SFX_H
#define _IN_EXT_SFX_H

#include <PR/ultratypes.h>

void extSfxInit(void);
void extSfxFree(void);
s32 extSfxDirExists(void);
s32 extSfxPreloadAll(void);

#define EXT_SFX_PRIORITY_LOW      10
#define EXT_SFX_PRIORITY_NORMAL   50
#define EXT_SFX_PRIORITY_WEAPON   80
#define EXT_SFX_PRIORITY_RELOAD   90
#define EXT_SFX_PRIORITY_VOICE    90
#define EXT_SFX_PRIORITY_CRITICAL 100

s32 extSfxPlay(const char *filename, s32 volume);
s32 extSfxPlayPan(const char *filename, s32 volume, s32 pan);
s32 extSfxPlayPriority(const char *filename, s32 volume, s32 priority);
s32 extSfxPlayPanPriority(const char *filename, s32 volume, s32 pan, s32 priority);
s32 extSfxMixIntoBuffer(s16 *samples, u32 frames);

#endif
