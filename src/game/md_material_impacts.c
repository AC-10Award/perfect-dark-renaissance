#include <ultra64.h>
#include "bss.h"
#include "constants.h"
#include "md/audio_aliases.h"
#include "md/material_impacts.h"
#include "lib/rng.h"
#include "types.h"

#ifndef PLATFORM_N64
#include "ext_sfx.h"

#define MD_MATERIAL_IMPACT_PENDING_DUSTS 16
#define MD_MATERIAL_IMPACT_DUST_DELAY_FRAMES 3
#define MD_MATERIAL_IMPACT_WALL_HIT_COOLDOWN_FRAMES 3
#define MD_MATERIAL_IMPACT_DUST_COOLDOWN_FRAMES 3
#define MD_MATERIAL_IMPACT_WALL_HIT_VOLUME (AL_VOL_FULL * 2)

struct mdmaterialimpactpendingdust {
	const char *sound;
	s32 playframe;
};

static struct mdmaterialimpactpendingdust g_MdMaterialImpactPendingDusts[MD_MATERIAL_IMPACT_PENDING_DUSTS];
static s32 g_MdMaterialImpactNextPendingDust = 0;
static s32 g_MdMaterialImpactLastWallHitFrame = -MD_MATERIAL_IMPACT_WALL_HIT_COOLDOWN_FRAMES;
static s32 g_MdMaterialImpactLastDustFrame = -MD_MATERIAL_IMPACT_DUST_COOLDOWN_FRAMES;

static void mdMaterialImpactPlayRandom(const char *const *sounds, s32 numsounds)
{
	if (numsounds > 0) {
		extSfxPlayPriority(sounds[rngRandom() % numsounds], AL_VOL_FULL, EXT_SFX_PRIORITY_LOW);
	}
}

static void mdMaterialImpactPlayRandomAtVolume(const char *const *sounds, s32 numsounds, s32 volume)
{
	if (numsounds > 0) {
		extSfxPlayPriority(sounds[rngRandom() % numsounds], volume, EXT_SFX_PRIORITY_LOW);
	}
}

static void mdMaterialImpactScheduleRandomDust(const char *const *sounds, s32 numsounds)
{
	if (numsounds > 0) {
		struct mdmaterialimpactpendingdust *pending = &g_MdMaterialImpactPendingDusts[g_MdMaterialImpactNextPendingDust];

		pending->sound = sounds[rngRandom() % numsounds];
		pending->playframe = g_Vars.lvframenum + MD_MATERIAL_IMPACT_DUST_DELAY_FRAMES;
		g_MdMaterialImpactNextPendingDust = (g_MdMaterialImpactNextPendingDust + 1) % MD_MATERIAL_IMPACT_PENDING_DUSTS;
	}
}

static void mdMaterialImpactPlayMetalHit(void)
{
	static const char *const sounds[] = {
		MD_SFX_METAL_HIT_01,
		MD_SFX_METAL_HIT_02,
		MD_SFX_METAL_HIT_03,
		MD_SFX_METAL_HIT_04,
		MD_SFX_METAL_HIT_05,
		MD_SFX_METAL_HIT_06,
	};

	mdMaterialImpactPlayRandom(sounds, ARRAYCOUNT(sounds));
}
#endif

void mdMaterialImpactsTick(void)
{
#ifndef PLATFORM_N64
	s32 i;

	for (i = 0; i < MD_MATERIAL_IMPACT_PENDING_DUSTS; i++) {
		struct mdmaterialimpactpendingdust *pending = &g_MdMaterialImpactPendingDusts[i];

		if (pending->playframe != 0 && g_Vars.lvframenum >= pending->playframe) {
			extSfxPlayPriority(pending->sound, AL_VOL_FULL, EXT_SFX_PRIORITY_LOW);
			pending->sound = NULL;
			pending->playframe = 0;
		}
	}
#endif
}

void mdMaterialImpactPlayBodyHit(bool enabled)
{
#ifndef PLATFORM_N64
	static const char *const sounds[] = {
		MD_SFX_BODY_HIT_01,
		MD_SFX_BODY_HIT_02,
		MD_SFX_BODY_HIT_03,
		MD_SFX_BODY_HIT_04,
		MD_SFX_BODY_HIT_05,
		MD_SFX_BODY_HIT_06,
	};

	if (enabled) {
		mdMaterialImpactPlayRandom(sounds, ARRAYCOUNT(sounds));
	}
#endif
}

void mdMaterialImpactPlayTerminationMetalHit(void)
{
#ifndef PLATFORM_N64
	mdMaterialImpactPlayMetalHit();
#endif
}

bool mdMaterialImpactTryPlayCustomBgMetalHit(s32 stagenum, s32 texturenum)
{
#ifndef PLATFORM_N64
	if (stagenum == STAGE_EXTRA15 && (texturenum == 0x007f || texturenum == 0x007d)) {
		mdMaterialImpactPlayMetalHit();
		return true;
	}
#endif

	return false;
}

bool mdMaterialImpactTryPlayCustomBgWallHit(s32 stagenum, s32 texturenum)
{
#ifndef PLATFORM_N64
	static const char *const wallhits[] = {
		MD_SFX_WALL_HIT_01,
		MD_SFX_WALL_HIT_02,
		MD_SFX_WALL_HIT_03,
		MD_SFX_WALL_HIT_04,
		MD_SFX_WALL_HIT_05,
		MD_SFX_WALL_HIT_06,
		MD_SFX_WALL_HIT_07,
		MD_SFX_WALL_HIT_08,
		MD_SFX_WALL_HIT_09,
		MD_SFX_WALL_HIT_10,
		MD_SFX_WALL_HIT_11,
		MD_SFX_WALL_HIT_12,
		MD_SFX_WALL_HIT_13,
		MD_SFX_WALL_HIT_14,
	};
	static const char *const dusts[] = {
		MD_SFX_DUST_01,
		MD_SFX_DUST_02,
		MD_SFX_DUST_03,
		MD_SFX_DUST_04,
	};

	if (stagenum == STAGE_EXTRA15 && texturenum == 0x0088) {
		if (g_Vars.lvframenum - g_MdMaterialImpactLastWallHitFrame >= MD_MATERIAL_IMPACT_WALL_HIT_COOLDOWN_FRAMES) {
			mdMaterialImpactPlayRandomAtVolume(wallhits, ARRAYCOUNT(wallhits), MD_MATERIAL_IMPACT_WALL_HIT_VOLUME);
			g_MdMaterialImpactLastWallHitFrame = g_Vars.lvframenum;
		}

		if (g_Vars.lvframenum - g_MdMaterialImpactLastDustFrame >= MD_MATERIAL_IMPACT_DUST_COOLDOWN_FRAMES) {
			mdMaterialImpactScheduleRandomDust(dusts, ARRAYCOUNT(dusts));
			g_MdMaterialImpactLastDustFrame = g_Vars.lvframenum;
		}

		return true;
	}
#endif

	return false;
}

void mdMaterialImpactPlayTileHit(void)
{
#ifndef PLATFORM_N64
	extSfxPlayPriority(MD_SFX_TILE_HIT_01, AL_VOL_FULL, EXT_SFX_PRIORITY_LOW);
#endif
}

void mdMaterialImpactPlayDoorDebris(void)
{
#ifndef PLATFORM_N64
	extSfxPlayPriority(MD_SFX_DOOR_EXPLOSION_DEBRIS, AL_VOL_FULL, EXT_SFX_PRIORITY_LOW);
#endif
}
