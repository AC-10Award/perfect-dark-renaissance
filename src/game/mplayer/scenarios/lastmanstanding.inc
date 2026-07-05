// Perfect Dark Renaissance multiplayer extensions
// Updated: 2026-03-19
/**
 * Last Man Standing
 *
 * Prototype rules:
 * - Human players get 10 lives.
 * - Bots get 15 lives.
 * - Bots escalate in difficulty as their remaining lives drop.
 * - Match ends when all human lives are gone or all bot lives are gone.
 */

#include <string.h>

#define LMS_PLAYER_LIFE_WARNING 3

struct lmsbotstyle {
	s32 difficulty;
	u8 body;
	const char *name;
};

static const struct lmsbotstyle g_LmsBotStyles[] = {
	{ BOTDIFF_MEAT,    MPBODY_DD_GUARD,      "RECRUIT" },
	{ BOTDIFF_EASY,    MPBODY_DD_GUARD,      "RECRUIT" },
	{ BOTDIFF_NORMAL,  MPBODY_G5_SWAT_GUARD, "AGENT"   },
	{ BOTDIFF_HARD,    MPBODY_MRBLONDE,      "ELITE"   },
	{ BOTDIFF_PERFECT, MPBODY_DJBOND,        "BOND"    },
	{ BOTDIFF_DARK,    MPBODY_DARK_COMBAT,   "DARK"    },
};

static s32 lmsChooseDifficultyForRemainingLives(s32 remaininglives)
{
	if (remaininglives <= 1) {
		return BOTDIFF_PERFECT;
	}

	if (remaininglives <= 3) {
		return BOTDIFF_HARD;
	}

	if (remaininglives <= 7) {
		return BOTDIFF_NORMAL;
	}

	return BOTDIFF_EASY;
}

static u32 lmsGetRadarColourForDifficulty(s32 difficulty)
{
	switch (difficulty) {
	case BOTDIFF_PERFECT:
		return 0xff000000;
	case BOTDIFF_HARD:
		return 0xff800000;
	case BOTDIFF_NORMAL:
		return 0xffff0000;
	case BOTDIFF_MEAT:
	case BOTDIFF_EASY:
	default:
		return 0x00ff0000;
	}
}

static const struct lmsbotstyle *lmsGetStyleForDifficulty(s32 difficulty)
{
	s32 i;

	for (i = 0; i < ARRAYCOUNT(g_LmsBotStyles); i++) {
		if (g_LmsBotStyles[i].difficulty == difficulty) {
			return &g_LmsBotStyles[i];
		}
	}

	return &g_LmsBotStyles[0];
}

static void lmsApplyBotStyle(s32 botnum, s32 difficulty)
{
	const struct lmsbotstyle *style = lmsGetStyleForDifficulty(difficulty);
	struct mpchrconfig *mpchr = &g_BotConfigsArray[botnum].base;

	g_BotConfigsArray[botnum].type = BOTTYPE_GENERAL;
	mpchr->team = MPTEAM_1;
	mpchr->mpbodynum = style->body;
	mpchr->mpheadnum = mpGetMpheadnumByMpbodynum(style->body);
	mpSetBotCustomName(botnum, style->name);

	if (botnum < g_BotCount && g_MpBotChrPtrs[botnum]) {
		g_MpBotChrPtrs[botnum]->team = MPTEAM_1;
	}
}

static void lmsEnforceLiveTeams(void)
{
	s32 i;

	for (i = 0; i < MAX_PLAYERS; i++) {
		if ((g_MpSetup.chrslots & MPCHRSLOT_BIT(i)) == 0) {
			continue;
		}

		g_PlayerConfigsArray[i].base.team = MPTEAM_0;

		if (g_MpAllChrPtrs[i]) {
			g_MpAllChrPtrs[i]->team = 1 << MPTEAM_0;
		}
	}

	for (i = 0; i < g_BotCount; i++) {
		s32 chrslot = MAX_PLAYERS + i;

		if ((g_MpSetup.chrslots & MPCHRSLOT_BIT(chrslot)) == 0) {
			continue;
		}

		g_BotConfigsArray[i].base.team = MPTEAM_1;

		if (g_MpBotChrPtrs[i]) {
			g_MpBotChrPtrs[i]->team = 1 << MPTEAM_1;
		}
	}
}

void lmsInit(void)
{
	s32 i;

	g_MpSetup.options |= MPOPTION_TEAMSENABLED;
	g_Vars.mpquickteam = MPQUICKTEAM_PLAYERSVSSIMS;
	g_MpSetup.timelimit = 60;
	g_MpSetup.scorelimit = 100;
	g_MpSetup.teamscorelimit = 400;

	for (i = 0; i < MAX_PLAYERS; i++) {
		g_PlayerConfigsArray[i].base.team = MPTEAM_0;
		g_PlayerConfigsArray[i].base.displayoptions &= ~MPDISPLAYOPTION_HIGHLIGHTTEAMS;
		g_Vars.mpplayerteams[i] = MPTEAM_0;
	}

	for (i = 0; i < ARRAYCOUNT(g_ScenarioData.lms.lastbotdifficulties); i++) {
		g_ScenarioData.lms.lastbotdifficulties[i] = 0xff;
	}
}

void lmsTick(void)
{
	s32 i;

	lmsEnforceLiveTeams();

	for (i = 0; i < g_BotCount && i < ARRAYCOUNT(g_ScenarioData.lms.lastbotdifficulties); i++) {
		s32 chrslot = MAX_PLAYERS + i;
		s32 remaininglives;
		s32 difficulty;

		if ((g_MpSetup.chrslots & MPCHRSLOT_BIT(chrslot)) == 0) {
			continue;
		}

		remaininglives = mpGetRemainingLivesForChrSlot(chrslot);

		if (remaininglives <= 0) {
			continue;
		}

		difficulty = lmsChooseDifficultyForRemainingLives(remaininglives);

		if (g_ScenarioData.lms.lastbotdifficulties[i] != difficulty) {
			mpSetBotDifficulty(i, difficulty);
			lmsApplyBotStyle(i, difficulty);
			g_ScenarioData.lms.lastbotdifficulties[i] = difficulty;
		}
	}
}

bool lmsRadarChr(Gfx **gdl, struct prop *prop)
{
	struct coord pos;
	u32 colour;

	if (!prop || prop->type != PROPTYPE_CHR || !prop->chr || !prop->chr->aibot) {
		return false;
	}

	if (chrIsDead(prop->chr) || (prop->chr->hidden & CHRHFLAG_CLOAKED) != 0) {
		return true;
	}

	pos.x = prop->pos.x - g_Vars.currentplayer->prop->pos.x;
	pos.y = prop->pos.y - g_Vars.currentplayer->prop->pos.y;
	pos.z = prop->pos.z - g_Vars.currentplayer->prop->pos.z;

	colour = lmsGetRadarColourForDifficulty(prop->chr->aibot->config->difficulty);
	*gdl = radarDrawDot(*gdl, prop, &pos, colour, 0, 0);

	return true;
}

Gfx *lmsRenderHud(Gfx *gdl)
{
	s32 playernum = g_Vars.currentplayerstats->mpindex;
	s32 remaininglives = mpGetRemainingLivesForChrSlot(playernum);
	s32 x = viGetViewLeft() + 8;
	s32 y = viGetViewTop() + 8;
	s32 textwidth;
	s32 textheight;
	u32 colour = 0x00ff00a0;
	char text[32];

	if (remaininglives < 0) {
		return gdl;
	}

	if (remaininglives <= 1) {
		strcpy(text, "FINAL LIFE");
		colour = 0xff4040a0;
	} else {
		sprintf(text, "%d LIVES LEFT", remaininglives);

		if (remaininglives <= LMS_PLAYER_LIFE_WARNING) {
			colour = 0xffc000a0;
		}
	}

#ifndef PLATFORM_N64
	{
		const s32 playercount = PLAYERCOUNT();
		if (playercount < 2 || (playercount == 2 && optionsGetScreenSplit() == SCREENSPLIT_HORIZONTAL)) {
			gSPSetExtraGeometryModeEXT(gdl++, G_ASPECT_CENTER_EXT);
		}
	}
#endif

	gdl = text0f153628(gdl);
	textMeasure(&textheight, &textwidth, text, g_CharsHandelGothicXs, g_FontHandelGothicXs, 0);

#if VERSION >= VERSION_NTSC_1_0
	gdl = text0f153990(gdl, x, y, x + textwidth, y + textheight);
	gdl = textRender(gdl, &x, &y, text, g_CharsHandelGothicXs, g_FontHandelGothicXs, colour, 0x000000a0, viGetWidth(), viGetHeight(), 0, 0);
#else
	{
		s32 x2 = x + textwidth;
		s32 y2 = y + textheight;
		gdl = text0f153858(gdl, &x, &y, &x2, &y2);
		gdl = textRender(gdl, &x, &y, text, g_CharsHandelGothicXs, g_FontHandelGothicXs, colour, 0x88, viGetWidth(), viGetHeight(), 0, 0);
	}
#endif

	gdl = text0f153780(gdl);

#ifndef PLATFORM_N64
	gSPClearExtraGeometryModeEXT(gdl++, G_ASPECT_MODE_EXT);
#endif

	return gdl;
}
