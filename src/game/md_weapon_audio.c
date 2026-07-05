#include <ultra64.h>
#include "constants.h"
#include "types.h"
#include "game/propsnd.h"
#include "md/audio_aliases.h"
#include "md/weapon_aliases.h"
#include "md/weapon_audio.h"

#ifndef PLATFORM_N64
#include "ext_sfx.h"
#endif

static const char *mdWeaponAudioGetFireSfx(s32 weaponnum)
{
	switch (weaponnum) {
	case MD_WEAPON_MATRIX85:
		return MD_SFX_MATRIX85_FIRE;
	}

	return NULL;
}

s32 mdWeaponAudioPlayFire(const struct gset *gset, s32 volume)
{
	const char *filename;

	if (!gset) {
		return 0;
	}

	filename = mdWeaponAudioGetFireSfx(gset->weaponnum);

	if (!filename) {
		return 0;
	}

#ifndef PLATFORM_N64
	return extSfxPlayPriority(filename, volume, EXT_SFX_PRIORITY_WEAPON);
#else
	return 0;
#endif
}

s32 mdWeaponAudioPlayFireAtProp(const struct gset *gset, struct prop *prop, s16 stocksound, s32 volume)
{
	const char *filename;
	s32 calculatedvol = volume;
	s32 pan = AL_PAN_CENTER;

	if (!gset) {
		return 0;
	}

	filename = mdWeaponAudioGetFireSfx(gset->weaponnum);

	if (!filename) {
		return 0;
	}

	if (prop && stocksound) {
		psGetTheoreticalVolPan(&prop->pos, prop->rooms, stocksound, &calculatedvol, &pan);
		calculatedvol = (calculatedvol * volume) / AL_VOL_FULL;
	}

	if (calculatedvol <= 0) {
		return 1;
	}

#ifndef PLATFORM_N64
	return extSfxPlayPanPriority(filename, calculatedvol, pan, EXT_SFX_PRIORITY_WEAPON);
#else
	return 0;
#endif
}
