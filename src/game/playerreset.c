#include <ultra64.h>
#include "constants.h"
#include "constants.h"
#include "game/bondmove.h"
#include "game/cheats.h"
#include "game/inv.h"
#include "game/playerreset.h"
#include "game/chr.h"
#include "game/body.h"
#include "game/prop.h"
#include "game/setuputils.h"
#include "game/bondgun.h"
#include "game/player.h"
#include "game/inv.h"
#include "game/stagetable.h"
#include "game/mplayer/scenarios.h"
#include "game/mplayer/mplayer.h"
#include "game/pad.h"
#include "bss.h"
#include "lib/collision.h"
#include "lib/memp.h"
#include "lib/mtx.h"
#include "lib/anim.h"
#include "data.h"
#include "types.h"

#ifndef PLATFORM_N64
#include <stdarg.h>
#include <stdio.h>
#endif

#ifndef PLATFORM_N64
static void playerResetCrashAuditLog(const char *format, ...)
{
	FILE *fp = fopen("/tmp/pd-map-crash.log", "a");

	if (fp) {
		va_list ap;
		va_start(ap, format);
		vfprintf(fp, format, ap);
		va_end(ap);
		fclose(fp);
	}
}
#else
static void playerResetCrashAuditLog(const char *format, ...)
{
	if (format) {
	}
}
#endif

void playerInitEyespy(void)
{
	struct prop *prop;
	struct pad pad;
	struct chrdata *propchr;
	struct chrdata *playerchr;
	static u8 nextpad = 0;

	if (g_Vars.currentplayer->eyespy == NULL) {
		/**
		 * To create the eyespy's prop, a pad must be passed to bodyAllocateEyespy.
		 * However the eyespy doesn't have a pad because it's held by the
		 * player, so it needs to choose one from the stage. The method used
		 * will increment the chosen pad number each time the stage is loaded
		 * and wrap at 256.
		 *
		 * @bug: This method means if you play G5 Building enough times then
		 * the camspy will start in a trigger point for the mid cutscene,
		 * causing the mid cutscene to play instead of the intro.
		 */
		padUnpack(nextpad++, PADFIELD_ROOM | PADFIELD_POS, &pad);
		prop = bodyAllocateEyespy(&pad, pad.room);

		if (prop) {
			g_Vars.currentplayer->eyespy = mempAlloc(sizeof(struct eyespy), MEMPOOL_STAGE);

			if (g_Vars.currentplayer->eyespy) {
				g_Vars.currentplayer->eyespy->prop = prop;
				g_Vars.currentplayer->eyespy->look.x = 0;
				g_Vars.currentplayer->eyespy->look.y = 0;
				g_Vars.currentplayer->eyespy->look.z = 1;
				g_Vars.currentplayer->eyespy->up.x = 0;
				g_Vars.currentplayer->eyespy->up.y = 1;
				g_Vars.currentplayer->eyespy->up.z = 0;
				g_Vars.currentplayer->eyespy->theta = 0;
				g_Vars.currentplayer->eyespy->costheta = 1;
				g_Vars.currentplayer->eyespy->sintheta = 0;
				g_Vars.currentplayer->eyespy->verta = 0;
				g_Vars.currentplayer->eyespy->cosverta = 1;
				g_Vars.currentplayer->eyespy->sinverta = 0;
				g_Vars.currentplayer->eyespy->held = true;
				g_Vars.currentplayer->eyespy->deployed = false;
				g_Vars.currentplayer->eyespy->active = false;
				g_Vars.currentplayer->eyespy->buttonheld = false;
				g_Vars.currentplayer->eyespy->camerabuttonheld = false;
				g_Vars.currentplayer->eyespy->bobdir = 1;
				g_Vars.currentplayer->eyespy->bobtimer = 0;
				g_Vars.currentplayer->eyespy->bobactive = true;
				g_Vars.currentplayer->eyespy->vel.x = 0;
				g_Vars.currentplayer->eyespy->vel.y = 0;
				g_Vars.currentplayer->eyespy->vel.z = 0;
				g_Vars.currentplayer->eyespy->speed = 0;
				g_Vars.currentplayer->eyespy->oldground = 0;
				g_Vars.currentplayer->eyespy->height = 0;
				g_Vars.currentplayer->eyespy->gravity = 0;
				g_Vars.currentplayer->eyespy->hit = EYESPYHIT_NONE;
				g_Vars.currentplayer->eyespy->opendoor = false;
				g_Vars.currentplayer->eyespy->mode = EYESPYMODE_CAMSPY;
				propchr = prop->chr;
				playerchr = g_Vars.currentplayer->prop->chr;
				propchr->team = playerchr->team;

				if (stageGetIndex(g_Vars.stagenum) == STAGEINDEX_AIRBASE) {
					g_Vars.currentplayer->eyespy->mode = EYESPYMODE_DRUGSPY;
					g_Weapons[WEAPON_EYESPY]->name = L_GUN_061; // "DrugSpy"
					g_Weapons[WEAPON_EYESPY]->shortname = L_GUN_061; // "DrugSpy"
				} else if (stageGetIndex(g_Vars.stagenum) == STAGEINDEX_MBR || stageGetIndex(g_Vars.stagenum) == STAGEINDEX_CHICAGO) {
					g_Vars.currentplayer->eyespy->mode = EYESPYMODE_BOMBSPY;
				} else {
					g_Vars.currentplayer->eyespy->mode = EYESPYMODE_CAMSPY;
				}
			}
		}
	}
}

struct cmd32 {
	s32 type;
	s32 param1;
	s32 param2;
	s32 param3;
};

void playerReset(void)
{
	struct coord pos = {0, 0, 0};
	RoomNum rooms[8];
	f32 turnanglerad = 0;
	f32 groundy;
	bool hasdefaultweapon = false;
	struct cmd32 *cmd = (struct cmd32 *)g_StageSetup.intro;
	u8 haseyespy = false;
	s32 stack[7];
	s32 i;
	s32 numchrs;
	struct gecreditsdata *thing;
	struct chrdata *chr;
	s32 bodynum;
	s32 headnum;

	playerResetLoResIf4Mb();
	func0f18e558();

	if (g_Vars.normmplayerisrunning && (g_Vars.stagenum == STAGE_VILLA || g_Vars.stagenum == STAGE_CRASHSITE || g_Vars.stagenum == STAGE_PELAGIC || g_Vars.stagenum == STAGE_MP_AREA52)) {
		playerResetCrashAuditLog("map-crash: playerReset begin stagenum=%02x\n", g_Vars.stagenum);
	}

	g_InCutscene = false;

#if PAL
	var8009e388pf = 0;
#else
	g_CutsceneFrameOverrun240 = 0;
#endif

	var8007072c = 1;
	var80070738 = 0;
	var8007073c = 0;

	g_CurrentGeCreditsData = NULL;
	g_Vars.currentplayer->bondexploding = false;
	g_NumSpawnPoints = 0;
	g_Vars.currentplayer->bondtankexplode = false;
	g_Vars.currentplayer->gunmem2 = NULL;
	g_PlayersWithControl[0] = true;
	g_PlayersWithControl[1] = true;
	g_PlayersWithControl[2] = true;
	g_PlayersWithControl[3] = true;
	g_PlayerInvincible = false;

	playerSetTickMode(TICKMODE_GE_FADEIN);

	g_PlayerTriggerGeFadeIn = 0;
	var80070748 = 0;
	var8007074c = 0;

	g_Vars.currentplayer->bondviewlevtime60 = 0;
	g_Vars.currentplayer->bondwatchtime60 = 0;
	g_Vars.currentplayer->introanimnum = 0;

	g_DefaultWeapons[HAND_LEFT] = 0;
	g_DefaultWeapons[HAND_RIGHT] = 0;

	if (cmd) {
		while (cmd->type != INTROCMD_END) {
			switch (cmd->type) {
			case INTROCMD_SPAWN:
				if (cmd->param2 == 0) {
					if (g_NumSpawnPoints < 24) {
						g_SpawnPoints[g_NumSpawnPoints++] = cmd->param1;
					}
				}
				cmd = (struct cmd32 *)((uintptr_t)cmd + 12);
				break;
			case INTROCMD_CASE:
			case INTROCMD_CASERESPAWN:
				cmd = (struct cmd32 *)((uintptr_t)cmd + 12);
				break;
			case INTROCMD_HILL:
				cmd = (struct cmd32 *)((uintptr_t)cmd + 8);
				break;
			case INTROCMD_WEAPON:
				if (cmd->param3 == 0 && g_Vars.currentplayer != g_Vars.anti) {
					modelmgrLoadProjectileModeldefs(cmd->param1);

					if (cmd->param2 >= 0) {
						modelmgrLoadProjectileModeldefs(cmd->param2);
						invGiveDoubleWeapon(cmd->param1, cmd->param2);
					} else {
						invGiveSingleWeapon(cmd->param1);
					}

					if (!hasdefaultweapon) {
						g_DefaultWeapons[HAND_RIGHT] = cmd->param1;

						if (cmd->param2 >= 0) {
							g_DefaultWeapons[HAND_LEFT] = cmd->param2;
						}

						hasdefaultweapon = true;
					}

					if (cmd->param1 == WEAPON_EYESPY) {
						haseyespy = true;
					}
				}
				cmd = (struct cmd32 *)((uintptr_t)cmd + 16);
				break;
			case INTROCMD_AMMO:
				if (cmd->param3 == 0 && g_Vars.currentplayer != g_Vars.anti) {
					bgunSetAmmoQuantity(cmd->param1, cmd->param2);
				}
				cmd = (struct cmd32 *)((uintptr_t)cmd + 16);
				break;
			case INTROCMD_3:
				cmd = (struct cmd32 *)((uintptr_t)cmd + 32);
				break;
			case INTROCMD_4:
				cmd = (struct cmd32 *)((uintptr_t)cmd + 8);
				break;
			case INTROCMD_OUTFIT:
				g_Vars.currentplayer->bondtype = cmd->param1;
				cmd = (struct cmd32 *)((uintptr_t)cmd + 8);
				break;
			case INTROCMD_6:
				cmd = (struct cmd32 *)((uintptr_t)cmd + 40);
				break;
			case INTROCMD_WATCHTIME:
				g_Vars.currentplayer->bondwatchtime60 = 0;
				if (cmd->param2 > 0) {
					g_Vars.currentplayer->bondwatchtime60 += (cmd->param2 % 60) * 3600;
				}
				if (cmd->param1 > 0) {
					g_Vars.currentplayer->bondwatchtime60 += (cmd->param1 % 12) * 3600 * 60;
				}
				cmd = (struct cmd32 *)((uintptr_t)cmd + 12);
				break;
			case INTROCMD_CREDITOFFSET:
				thing = (struct gecreditsdata *)((uintptr_t)g_GeCreditsData + cmd->param1);
				g_CurrentGeCreditsData = thing;
				while (thing->text1 || thing->text2) {
					thing++;
				}
				cmd = (struct cmd32 *)((uintptr_t)cmd + 8);
				break;
			default:
				cmd = (struct cmd32 *)((uintptr_t)cmd + 4);
			}
		}
	}

	invGiveSingleWeapon(WEAPON_UNARMED);

	if (cheatIsActive(CHEAT_TRENTSMAGNUM)) {
		invGiveSingleWeapon(WEAPON_DY357LX);
		bgunSetAmmoQuantity(AMMOTYPE_MAGNUM, 80);
	}

	if (cheatIsActive(CHEAT_FARSIGHT)) {
		invGiveSingleWeapon(WEAPON_FARSIGHT);
		bgunSetAmmoQuantity(AMMOTYPE_FARSIGHT, 80);
	}

	if (cheatIsActive(CHEAT_CLOAKINGDEVICE)) {
		invGiveSingleWeapon(WEAPON_CLOAKINGDEVICE);
#if VERSION >= VERSION_PAL_FINAL
		bgunSetAmmoQuantity(AMMOTYPE_CLOAK, TICKS(7200));
#else
		bgunSetAmmoQuantity(AMMOTYPE_CLOAK, 7200);
#endif
	}

	if (cheatIsActive(CHEAT_PERFECTDARKNESS)) {
		invGiveSingleWeapon(WEAPON_NIGHTVISION);
	}

	if (cheatIsActive(CHEAT_RTRACKER)) {
		invGiveSingleWeapon(WEAPON_RTRACKER);
	}

	if (cheatIsActive(CHEAT_ROCKETLAUNCHER)) {
		invGiveSingleWeapon(WEAPON_ROCKETLAUNCHER);
		bgunSetAmmoQuantity(AMMOTYPE_ROCKET, 10);
	}

	if (cheatIsActive(CHEAT_SNIPERRIFLE)) {
		invGiveSingleWeapon(WEAPON_SNIPERRIFLE);
		bgunSetAmmoQuantity(AMMOTYPE_RIFLE, 200);
	}

	if (cheatIsActive(CHEAT_XRAYSCANNER)) {
		invGiveSingleWeapon(WEAPON_XRAYSCANNER);
	}

	if (cheatIsActive(CHEAT_SUPERDRAGON)) {
		invGiveSingleWeapon(WEAPON_SUPERDRAGON);
		bgunSetAmmoQuantity(AMMOTYPE_RIFLE, 200);
		bgunSetAmmoQuantity(AMMOTYPE_DEVASTATOR, 20);
	}

	if (cheatIsActive(CHEAT_LAPTOPGUN)) {
		invGiveSingleWeapon(WEAPON_LAPTOPGUN);
		bgunSetAmmoQuantity(AMMOTYPE_SMG, 200);
	}

	if (cheatIsActive(CHEAT_PHOENIX)) {
		invGiveSingleWeapon(WEAPON_PHOENIX);
		bgunSetAmmoQuantity(AMMOTYPE_PISTOL, 200);
	}

#if VERSION >= VERSION_NTSC_1_0
	if (cheatIsActive(CHEAT_PSYCHOSISGUN) || cheatIsActive(CHEAT_ALLGUNS)) {
		bgunSetAmmoQuantity(AMMOTYPE_PSYCHOSIS, 4);

		if (cheatIsActive(CHEAT_PSYCHOSISGUN)) {
			invGiveSingleWeapon(WEAPON_PSYCHOSISGUN);
		}
	}
#else
	if (cheatIsActive(CHEAT_PSYCHOSISGUN)) {
		invGiveSingleWeapon(WEAPON_PSYCHOSISGUN);
		bgunSetAmmoQuantity(AMMOTYPE_PSYCHOSIS, 4);
	}
#endif

	if (cheatIsActive(CHEAT_PP9I)) {
		invGiveSingleWeapon(WEAPON_PP9I);
		bgunSetAmmoQuantity(AMMOTYPE_PISTOL, 200);
	}

	if (cheatIsActive(CHEAT_CC13)) {
		invGiveSingleWeapon(WEAPON_CC13);
#ifndef PLATFORM_N64 // give the correct ammo for port
		bgunSetAmmoQuantity(AMMOTYPE_PISTOL, 200);
#else
		bgunSetAmmoQuantity(AMMOTYPE_RIFLE, 200);
#endif
	}

	if (cheatIsActive(CHEAT_KL01313)) {
		invGiveSingleWeapon(WEAPON_KL01313);
		bgunSetAmmoQuantity(AMMOTYPE_SMG, 200);
	}

	if (cheatIsActive(CHEAT_KF7SPECIAL)) {
		invGiveSingleWeapon(WEAPON_KF7SPECIAL);
		bgunSetAmmoQuantity(AMMOTYPE_RIFLE, 200);
	}

	if (cheatIsActive(CHEAT_ZZT)) {
		invGiveSingleWeapon(WEAPON_ZZT);
		bgunSetAmmoQuantity(AMMOTYPE_SMG, 200);
	}

	if (cheatIsActive(CHEAT_DMC)) {
		invGiveSingleWeapon(WEAPON_DMC);
		bgunSetAmmoQuantity(AMMOTYPE_SMG, 200);
	}

	if (cheatIsActive(CHEAT_AR53)) {
		invGiveSingleWeapon(WEAPON_AR53);
		bgunSetAmmoQuantity(AMMOTYPE_RIFLE, 200);
	}

	if (cheatIsActive(CHEAT_RCP45)) {
		invGiveSingleWeapon(WEAPON_RCP45);
		bgunSetAmmoQuantity(AMMOTYPE_SMG, 200);
	}

	if (!hasdefaultweapon) {
		g_DefaultWeapons[HAND_RIGHT] = WEAPON_UNARMED;
	}

	if (g_Vars.normmplayerisrunning && (g_Vars.stagenum == STAGE_VILLA || g_Vars.stagenum == STAGE_CRASHSITE || g_Vars.stagenum == STAGE_PELAGIC || g_Vars.stagenum == STAGE_MP_AREA52)) {
		playerResetCrashAuditLog("map-crash: playerReset pre-propAllocate stagenum=%02x spawnpoints=%d padsfile=%p\n",
				g_Vars.stagenum, g_NumSpawnPoints, g_PadsFile);
	}

	g_Vars.currentplayer->prop = propAllocate();
	if (g_Vars.normmplayerisrunning && (g_Vars.stagenum == STAGE_VILLA || g_Vars.stagenum == STAGE_CRASHSITE || g_Vars.stagenum == STAGE_PELAGIC || g_Vars.stagenum == STAGE_MP_AREA52)) {
		playerResetCrashAuditLog("map-crash: playerReset post-propAllocate stagenum=%02x prop=%p\n",
				g_Vars.stagenum, g_Vars.currentplayer->prop);
	}
	g_Vars.currentplayer->prop->chr = NULL;
	g_Vars.currentplayer->prop->type = PROPTYPE_PLAYER;

	propActivate(g_Vars.currentplayer->prop);
	propEnable(g_Vars.currentplayer->prop);
	chrInit(g_Vars.currentplayer->prop, NULL);

	if (g_Vars.normmplayerisrunning && (g_Vars.stagenum == STAGE_VILLA || g_Vars.stagenum == STAGE_CRASHSITE || g_Vars.stagenum == STAGE_PELAGIC || g_Vars.stagenum == STAGE_MP_AREA52)) {
		playerResetCrashAuditLog("map-crash: playerReset post-chrInit stagenum=%02x chr=%p team=%x\n",
				g_Vars.stagenum, g_Vars.currentplayer->prop->chr, g_Vars.currentplayer->prop->chr ? g_Vars.currentplayer->prop->chr->team : 0);
	}

	if (g_Vars.coopplayernum >= 0) {
		g_Vars.currentplayer->prop->chr->team = TEAM_ALLY;
	} else if (g_Vars.antiplayernum >= 0) {
		if (g_Vars.currentplayer == g_Vars.bond) {
			g_Vars.currentplayer->prop->chr->team = TEAM_ALLY;
		} else {
			g_Vars.currentplayer->prop->chr->team = TEAM_ENEMY;
		}
	} else {
		if (g_Vars.mplayerisrunning) {
			g_Vars.currentplayer->prop->chr->team = 1 << g_PlayerConfigsArray[g_Vars.currentplayerstats->mpindex].base.team;
		} else {
			g_Vars.currentplayer->prop->chr->team = TEAM_ALLY;
		}
	}

	if (haseyespy) {
		playerInitEyespy();
	}

	if (g_Vars.normmplayerisrunning && g_NumSpawnPoints == 0 && g_PadsFile != NULL) {
		s32 maxfallbackpads = 24;
		s32 fallbackcount = g_PadsFile->numpads;

		if (fallbackcount > maxfallbackpads) {
			fallbackcount = maxfallbackpads;
		}

		for (i = 0; i < fallbackcount; i++) {
			g_SpawnPoints[i] = (i * g_PadsFile->numpads) / fallbackcount;
		}

		g_NumSpawnPoints = fallbackcount;
	}

	if (g_Vars.normmplayerisrunning && (g_Vars.stagenum == STAGE_VILLA || g_Vars.stagenum == STAGE_CRASHSITE || g_Vars.stagenum == STAGE_PELAGIC || g_Vars.stagenum == STAGE_MP_AREA52)) {
		playerResetCrashAuditLog("map-crash: playerReset pre-spawnchoose stagenum=%02x spawnpoints=%d firstpad=%d\n",
				g_Vars.stagenum, g_NumSpawnPoints, g_NumSpawnPoints > 0 ? g_SpawnPoints[0] : -1);
	}

	if (g_NumSpawnPoints > 0) {
		if (g_Vars.coopplayernum >= 0) {
			turnanglerad = M_BADTAU - scenarioChooseSpawnLocation(30, &pos, rooms, g_Vars.currentplayer->prop);
		} else if (g_Vars.antiplayernum >= 0) {
			turnanglerad = M_BADTAU - scenarioChooseSpawnLocation(30, &pos, rooms, g_Vars.currentplayer->prop);
		} else {
			if (g_Vars.mplayerisrunning == 0) {
				g_NumSpawnPoints = 1;
			}

			turnanglerad = M_BADTAU - scenarioChooseSpawnLocation(30, &pos, rooms, g_Vars.currentplayer->prop);
		}
	}

	if (g_Vars.normmplayerisrunning && (g_Vars.stagenum == STAGE_VILLA || g_Vars.stagenum == STAGE_CRASHSITE || g_Vars.stagenum == STAGE_PELAGIC || g_Vars.stagenum == STAGE_MP_AREA52)) {
		playerResetCrashAuditLog("map-crash: playerReset post-spawnchoose stagenum=%02x pos=(%f,%f,%f) room0=%d angle=%f\n",
				g_Vars.stagenum, pos.x, pos.y, pos.z, rooms[0], turnanglerad);
	}

	groundy = cdFindGroundInfoAtCyl(&pos, 30, rooms,
			&g_Vars.currentplayer->floorcol,
			&g_Vars.currentplayer->floortype,
			&g_Vars.currentplayer->floorflags,
			&g_Vars.currentplayer->floorroom,
			0, 0);

	if (g_Vars.normmplayerisrunning && (g_Vars.stagenum == STAGE_VILLA || g_Vars.stagenum == STAGE_CRASHSITE || g_Vars.stagenum == STAGE_PELAGIC || g_Vars.stagenum == STAGE_MP_AREA52)) {
		playerResetCrashAuditLog("map-crash: playerReset post-groundinfo stagenum=%02x groundy=%f floorroom=%d\n",
				g_Vars.stagenum, groundy, g_Vars.currentplayer->floorroom);
	}

	pos.y = g_Vars.currentplayer->vv_eyeheight + groundy;
	g_Vars.currentplayer->vv_manground = groundy;
	g_Vars.currentplayer->vv_ground = groundy;
	g_Vars.currentplayer->vv_theta = (turnanglerad * 360.0f) / M_BADTAU;

	playerResetBond(&g_Vars.currentplayer->bond2, &pos);

	if (g_Vars.normmplayerisrunning && (g_Vars.stagenum == STAGE_VILLA || g_Vars.stagenum == STAGE_CRASHSITE || g_Vars.stagenum == STAGE_PELAGIC || g_Vars.stagenum == STAGE_MP_AREA52)) {
		playerResetCrashAuditLog("map-crash: playerReset post-playerResetBond stagenum=%02x\n", g_Vars.stagenum);
	}

	g_Vars.currentplayer->bond2.unk00.x = -sinf(turnanglerad);
	g_Vars.currentplayer->bond2.unk00.y = 0;
	g_Vars.currentplayer->bond2.unk00.z = cosf(turnanglerad);


	g_Vars.currentplayer->prop->pos.f[0] = g_Vars.currentplayer->bondprevpos.f[0] = pos.f[0];
	g_Vars.currentplayer->prop->pos.f[1] = g_Vars.currentplayer->bondprevpos.f[1] = pos.f[1];
	g_Vars.currentplayer->prop->pos.f[2] = g_Vars.currentplayer->bondprevpos.f[2] = pos.f[2];

	propDeregisterRooms(g_Vars.currentplayer->prop);

	g_Vars.currentplayer->prop->rooms[0] = rooms[0];
	g_Vars.currentplayer->prop->rooms[1] = -1;

	playerSetCamPropertiesWithRoom(&pos,
			&g_Vars.currentplayer->bond2.unk28,
			&g_Vars.currentplayer->bond2.unk1c, rooms[0]);

	if (g_Vars.normmplayerisrunning && (g_Vars.stagenum == STAGE_VILLA || g_Vars.stagenum == STAGE_CRASHSITE || g_Vars.stagenum == STAGE_PELAGIC || g_Vars.stagenum == STAGE_MP_AREA52)) {
		playerResetCrashAuditLog("map-crash: playerReset post-playerSetCamPropertiesWithRoom stagenum=%02x\n", g_Vars.stagenum);
	}

	numchrs = chrsGetNumSlots();

	for (i = 0; i < numchrs; i++) {
		chr = &g_ChrSlots[i];

		if (chr->target == -2) {
			chr->target = g_Vars.currentplayer->prop - g_Vars.props;
		}
	}

	bmoveUpdateRooms(g_Vars.currentplayer);

	if (g_Vars.normmplayerisrunning && (g_Vars.stagenum == STAGE_VILLA || g_Vars.stagenum == STAGE_CRASHSITE || g_Vars.stagenum == STAGE_PELAGIC || g_Vars.stagenum == STAGE_MP_AREA52)) {
		playerResetCrashAuditLog("map-crash: playerReset post-bmoveUpdateRooms stagenum=%02x\n", g_Vars.stagenum);
	}

	if (g_Vars.normmplayerisrunning) {
		playersBeginMpSwirl();
	} else {
		player0f0b9a20();
	}

	if (g_Vars.normmplayerisrunning && (g_Vars.stagenum == STAGE_VILLA || g_Vars.stagenum == STAGE_CRASHSITE || g_Vars.stagenum == STAGE_PELAGIC || g_Vars.stagenum == STAGE_MP_AREA52)) {
		playerResetCrashAuditLog("map-crash: playerReset post-swirl stagenum=%02x\n", g_Vars.stagenum);
	}

	g_NumDeathAnimations = 0;

	while (g_DeathAnimations[g_NumDeathAnimations] > 0) {
		g_NumDeathAnimations++;
	}

	g_Vars.currentplayer->tickdiefinished = false;
	g_Vars.currentplayer->chokehandle = NULL;

	for (i = 0; i < ARRAYCOUNT(g_Vars.aibuddies); i++) {
		g_Vars.aibuddies[i] = NULL;
	}

	playerChooseBodyAndHead(&bodynum, &headnum, 0);
	g_Vars.currentplayer->prop->chr->bodynum = bodynum;
	g_Vars.currentplayer->prop->chr->headnum = headnum;

	if (g_Vars.normmplayerisrunning && (g_Vars.stagenum == STAGE_VILLA || g_Vars.stagenum == STAGE_CRASHSITE || g_Vars.stagenum == STAGE_PELAGIC || g_Vars.stagenum == STAGE_MP_AREA52)) {
		playerResetCrashAuditLog("map-crash: playerReset end stagenum=%02x bodynum=%d headnum=%d\n",
				g_Vars.stagenum, bodynum, headnum);
	}
}
