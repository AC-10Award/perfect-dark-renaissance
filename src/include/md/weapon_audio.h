#ifndef _IN_MD_WEAPON_AUDIO_H
#define _IN_MD_WEAPON_AUDIO_H

#include <ultra64.h>

struct gset;
struct prop;

s32 mdWeaponAudioPlayFire(const struct gset *gset, s32 volume);
s32 mdWeaponAudioPlayFireAtProp(const struct gset *gset, struct prop *prop, s16 stocksound, s32 volume);

#endif
