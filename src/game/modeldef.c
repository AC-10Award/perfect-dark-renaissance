#include <ultra64.h>
#include "constants.h"
#include "game/chraction.h"
#include "game/ceil.h"
#include "game/bondgun.h"
#include "game/game_0b0fd0.h"
#include "game/tex.h"
#include "game/menugfx.h"
#include "game/menu.h"
#include "game/mainmenu.h"
#include "game/inv.h"
#include "game/game_1531a0.h"
#include "game/file.h"
#include "game/texdecompress.h"
#include "game/tex.h"
#include "game/modeldef.h"
#include "game/lang.h"
#include "game/mplayer/mplayer.h"
#include "game/options.h"
#include "bss.h"
#include "lib/vi.h"
#include "lib/main.h"
#include "lib/model.h"
#include "data.h"
#include "types.h"
#ifndef PLATFORM_N64
#include "system.h"
#endif

#ifndef PLATFORM_N64
static void logHeadModelTexconfigs(u16 fileid, struct modeldef *modeldef)
{
	s32 i;

	if (modeldef == NULL) {
		return;
	}

	if (fileid != 0x019f && fileid != 0x055b && fileid != 0x0564) {
		return;
	}

	sysLogPrintf(
			LOG_NOTE,
			"headtex: model %04x (%s) numtexconfigs=%d",
			fileid,
			romdataFileGetName(fileid),
			modeldef->numtexconfigs);

	for (i = 0; i < modeldef->numtexconfigs; i++) {
		sysLogPrintf(
				LOG_NOTE,
				"headtex: model %04x texconfig[%d] texturenum=%04x size=%dx%d format=%d depth=%d",
				fileid,
				i,
				(u32) modeldef->texconfigs[i].texturenum & 0xffff,
				modeldef->texconfigs[i].width,
				modeldef->texconfigs[i].height,
				modeldef->texconfigs[i].format,
				modeldef->texconfigs[i].depth);
	}
}

static bool modeldefIsTerminationBodyHandTexture(texnum_t texturenum)
{
	texturenum &= 0xffff;

	return texturenum >= 0x030a && texturenum <= 0x030e;
}

static texnum_t modeldefGetCharacterTextureRedirect(u16 fileid, texnum_t texturenum)
{
	texturenum &= 0xffff;

	if (fileid == FILE_CBOILERTREVZ) {
		if (texturenum == 0x030a) {
			return 0x0c7a;
		}

		if (texturenum == 0x030d) {
			return 0x0c5a;
		}

		if (texturenum == 0x030c || texturenum == 0x030b) {
			return 0x0c7e;
		}
	}

	if (fileid == FILE_CEVILCYBORGZ && modeldefIsTerminationBodyHandTexture(texturenum)) {
		return 0x0c78;
	}

	return texturenum;
}

static void modeldefApplyCharacterTextureRedirects(u16 fileid, struct modeldef *modeldef)
{
	s32 i;

	if (modeldef == NULL || modeldef->texconfigs == NULL) {
		return;
	}

	for (i = 0; i < modeldef->numtexconfigs; i++) {
		texnum_t texturenum = modeldef->texconfigs[i].texturenum & 0xffff;
		texnum_t redirected = modeldefGetCharacterTextureRedirect(fileid, texturenum);

		if (redirected != texturenum) {
			sysLogPrintf(
					LOG_NOTE,
					"character-tex: remap model=%04x texconfig[%d] %04x -> %04x",
					fileid,
					i,
					(u32)texturenum,
					(u32)redirected);
			modeldef->texconfigs[i].texturenum = redirected;
		}
	}
}
#endif

struct skeleton *g_Skeletons[] = {
	&g_SkelChr,
	&g_SkelClassicGun,
	&g_Skel06,
	&g_SkelUzi,
	&g_SkelBasic,
	&g_SkelCctv,
	&g_SkelWindowedDoor,
	&g_Skel11,
	&g_Skel12,
	&g_Skel13,
	&g_SkelTerminal,
	&g_SkelCiHub,
	&g_SkelAutogun,
	&g_Skel17,
	&g_Skel18,
	&g_Skel19,
	&g_Skel0A,
	&g_Skel0B,
	&g_SkelCasing,
	&g_SkelChrGun,
	&g_Skel0C,
	&g_SkelJoypad,
	&g_SkelLift,
	&g_SkelSkedar,
	&g_SkelLogo,
	&g_SkelPdLogo,
	&g_SkelHoverbike,
	&g_SkelJumpship,
	&g_Skel20,
	&g_Skel21,
	&g_Skel22,
	&g_SkelLaptopGun,
	&g_SkelK7Avenger,
	&g_SkelChopper,
	&g_SkelFalcon2,
	&g_SkelKnife,
	&g_SkelDrCaroll,
	&g_SkelRope,
	&g_SkelCmp150,
	&g_SkelBanner,
	&g_SkelDragon,
	&g_SkelSuperDragon,
	&g_SkelRocket,
	&g_Skel4A,
	&g_SkelShotgun,
	&g_SkelFarsight,
	&g_Skel4D,
	&g_SkelReaper,
	&g_SkelDropship,
	&g_SkelMauler,
	&g_SkelDevastator,
	&g_SkelRobot,
	&g_SkelPistol,
	&g_SkelAr34,
	&g_SkelMagnum,
	&g_SkelSlayerRocket,
	&g_SkelCyclone,
	&g_SkelSniperRifle,
	&g_SkelTranquilizer,
	&g_SkelCrossbow,
	&g_SkelHudPiece,
	&g_SkelTimedProxyMine,
	&g_SkelPhoenix,
	&g_SkelCallisto,
	&g_SkelHand,
	&g_SkelRcp120,
	&g_SkelSkShuttle,
	&g_SkelLaser,
	&g_SkelMaianUfo,
	&g_SkelGrenade,
	&g_SkelCableCar,
	&g_SkelSubmarine,
	&g_SkelTarget,
	&g_SkelEcmMine,
	&g_SkelUplink,
	&g_SkelRareLogo,
	&g_SkelWireFence,
	&g_SkelRemoteMine,
	&g_SkelBB,
#ifdef AVOID_UB
	NULL // terminate list for sure
#endif
};

void modeldef0f1a7560(struct modeldef *modeldef, u16 filenum, u32 arg2, struct modeldef *modeldef2, struct texpool *texpool, bool arg5)
{
	s32 allocsize;
	s32 loadedsize;
	s32 sp84;
	u32 s0;
	u32 s4;
	uintptr_t s5;
	struct modelnode *node;
	struct modelnode *prevnode;
	uintptr_t gdl;
	Vtx *vertices;

	allocsize = fileGetAllocationSize(filenum);
	loadedsize = fileGetLoadedSize(filenum);
	node = NULL;

	modelIterateDisplayLists(modeldef, &node, (Gfx **)&gdl);

	s5 = gdl;

	if (gdl) {
		s32 v1 = allocsize - (loadedsize - (uintptr_t)(((uintptr_t)modeldef + (UNSEGADDR(gdl) & 0xffffff)) - (uintptr_t)modeldef));
		sp84 = (uintptr_t)v1 + (uintptr_t)((uintptr_t)modeldef - ((uintptr_t)modeldef + (UNSEGADDR(gdl) & 0xffffff)));

		texCopyGdls((Gfx *)((uintptr_t)modeldef + (UNSEGADDR(gdl) & 0xffffff)),
				(Gfx *)(v1 + (uintptr_t)modeldef),
				loadedsize - (uintptr_t)(((uintptr_t)modeldef + (UNSEGADDR(gdl) & 0xffffff)) - (uintptr_t)modeldef));
#ifndef PLATFORM_N64
		logHeadModelTexconfigs(filenum, modeldef);
		modeldefApplyCharacterTextureRedirects(filenum, modeldef);
#endif
		texLoadFromConfigs(modeldef->texconfigs, modeldef->numtexconfigs, texpool, (uintptr_t)modeldef2 - (uintptr_t)arg2);

		while (node) {
			prevnode = node;
			s0 = gdl;

			modelIterateDisplayLists(modeldef, &node, (Gfx **) &gdl);

			if (gdl) {
				s4 = UNSEGADDR(gdl) - UNSEGADDR(s0);
			} else {
				s4 = loadedsize + (uintptr_t)modeldef - (uintptr_t)modeldef - (UNSEGADDR(s0) & 0xffffff);
			}

			modelNodeReplaceGdl(modeldef, prevnode, (Gfx *) s0, (Gfx *) s5);

			if (prevnode->type == MODELNODETYPE_DL) {
				struct modelrodata_dl *rodata = &prevnode->rodata->dl;
				vertices = rodata->vertices;
			} else {
				vertices = NULL;
			}

			s5 += texLoadFromGdl((Gfx *)((uintptr_t)modeldef + (UNSEGADDR(s0) & 0xffffff) + sp84), s4, (Gfx *)((uintptr_t)modeldef + (UNSEGADDR(s5) & 0xffffff)), texpool, (u8 *) vertices, filenum);
		}

		fileSetSize(filenum, modeldef, (((uintptr_t)modeldef + (UNSEGADDR(s5) & 0xffffff)) - (uintptr_t)modeldef + 0xf) & ~0xf, arg5);
	}
}

void modelPromoteTypeToPointer(struct modeldef *modeldef)
{
	s32 i;

	if ((u32)modeldef->skel < 0x10000) {
		for (i = 0; g_Skeletons[i] != NULL; i++) {
			if ((s16)modeldef->skel == g_Skeletons[i]->skel) {
				modeldef->skel = g_Skeletons[i];
				return;
			}
		}
	}
}

struct modeldef *modeldefLoad(u16 fileid, u8 *dst, s32 size, struct texpool *arg3)
{
	struct modeldef *modeldef;

	g_LoadType = LOADTYPE_MODEL;

	if (dst) {
		modeldef = fileLoadToAddr(fileid, FILELOADMETHOD_EXTRAMEM, dst, size);
	} else {
		modeldef = fileLoadToNew(fileid, FILELOADMETHOD_EXTRAMEM, LOADTYPE_MODEL);
	}

	modelPromoteTypeToPointer(modeldef);
	modelPromoteOffsetsToPointers(modeldef, 0x5000000, (uintptr_t) modeldef);
	modeldef0f1a7560(modeldef, fileid, 0x5000000, modeldef, arg3, dst == NULL);

	return modeldef;
}

struct modeldef *modeldefLoadToNew(u16 fileid)
{
	return modeldefLoad(fileid, NULL, 0, NULL);
}

struct modeldef *modeldefLoadToAddr(u16 fileid, u8 *dst, s32 size)
{
	return modeldefLoad(fileid, dst, size, NULL);
}
