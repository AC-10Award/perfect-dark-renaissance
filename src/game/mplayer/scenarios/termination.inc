/**
 * Termination
 *
 * Prototype rules:
 * - Bot 0 is the Termination.
 * - Remaining bots are victims.
 * - Humans have 10 lives.
 * - Termination has 1 life and a large health pool.
 * - Victims have 5 lives each.
 * - Humans win by killing the Termination.
 * - Humans lose if all victim lives are exhausted.
 */

#include <string.h>

#define TERM_TERMINATION_BOTNUM    0
#define TERM_TERMINATION_TEAM      MPTEAM_1
#define TERM_HUMAN_TEAM           MPTEAM_0
#define TERM_VICTIM_TEAM          TERM_HUMAN_TEAM
#define TERM_TERMINATION_HEALTH    300.0f
#define TERM_TRANSFORM_FRACTION   0.25f
#define TERM_TRANSFORM_BODY       BODY_ROBOT_SKELINGTON
#define TERM_TRANSFORM_HEAD       HEAD_ROBOT_SKELINGTON
#define TERM_TRANSFORM_MPBODY     MPBODY_ROBOT_SKELINGTON
#define TERM_TRANSFORM_MPHEAD     MPHEAD_ROBOT_SKELINGTON
#define TERM_TRANSFORM_EXPLOSION  EXPLOSIONTYPE_ROCKET
#define TERM_VICTIM_HEALTH        4.0f
#define TERM_TERMINATION_WEAPON    WEAPON_FALCON2_SCOPE
#define TERM_TERMINATION_AMMO      600
#define TERM_MIN_BOTS             2

static bool termIsVictimChr(struct chrdata *chr)
{
	return chr
		&& chr->aibot
		&& chr->aibot->aibotnum > TERM_TERMINATION_BOTNUM;
}

static struct chrdata *termGetBotChrByNum(s32 botnum)
{
	s32 i;

	for (i = 0; i < g_BotCount; i++) {
		struct chrdata *chr = g_MpBotChrPtrs[i];

		if (chr && chr->aibot && chr->aibot->aibotnum == botnum) {
			return chr;
		}
	}

	return NULL;
}

static void termMakeChrDamageable(struct chrdata *chr)
{
	if (!chr) {
		return;
	}

	chr->chrflags &= ~CHRCFLAG_INVINCIBLE;
	chr->hidden &= ~CHRHFLAG_INFINITESHIELD;
	chrSetShield(chr, 0);
}

static void termPacifyVictimChr(struct chrdata *chr)
{
	if (!termIsVictimChr(chr) || chrIsDead(chr) || chr->actiontype == ACT_DIE) {
		return;
	}

	termMakeChrDamageable(chr);
	chr->maxdamage = TERM_VICTIM_HEALTH;

	if (chr->damage > TERM_VICTIM_HEALTH) {
		chr->damage = TERM_VICTIM_HEALTH;
	}

	botSetTarget(chr, -1);
	chr->aibot->attackingplayernum = -1;
	chr->aibot->followingplayernum = -1;
	chr->aibot->gotoprop = NULL;
	chr->aibot->command = AIBOTCMD_NORMAL;
	chr->aibot->loadedammo[HAND_LEFT] = 0;
	chr->aibot->loadedammo[HAND_RIGHT] = 0;

	botinvSwitchToWeapon(chr, WEAPON_UNARMED, FUNC_PRIMARY);
	botinvClear(chr);
}

static void termArmTerminationChr(struct chrdata *chr)
{
	if (!chr || !chr->aibot || chrIsDead(chr)) {
		return;
	}

	if (!botinvGetItem(chr, TERM_TERMINATION_WEAPON)) {
		botinvGiveSingleWeapon(chr, TERM_TERMINATION_WEAPON);
	}

	if (botactGetAmmoQuantityByWeapon(chr->aibot, TERM_TERMINATION_WEAPON, FUNC_PRIMARY, true) < TERM_TERMINATION_AMMO / 2) {
		botactGiveAmmoByWeapon(chr->aibot, TERM_TERMINATION_WEAPON, FUNC_PRIMARY, TERM_TERMINATION_AMMO);
	}

	if (chr->aibot->weaponnum != TERM_TERMINATION_WEAPON || chr->aibot->gunfunc != FUNC_PRIMARY) {
		botinvSwitchToWeapon(chr, TERM_TERMINATION_WEAPON, FUNC_PRIMARY);
	}

	if (chr->aibot->weaponnum == TERM_TERMINATION_WEAPON
			&& chr->aibot->gunfunc == FUNC_PRIMARY
			&& !chr->weapons_held[HAND_RIGHT]) {
		s32 modelnum = playermgrGetModelOfWeapon(TERM_TERMINATION_WEAPON);

		if (modelnum >= 0) {
			chr->aibot->changeguntimer60 = 0;
			chrGiveWeapon(chr, modelnum, TERM_TERMINATION_WEAPON, 0);
		}
	}

	if (chr->weapons_held[HAND_RIGHT]
			&& chr->aibot->loadedammo[HAND_RIGHT] <= 0) {
		botactReload(chr, HAND_RIGHT, false);
	}
}

static void termReattachHeldWeapon(struct chrdata *chr, s32 handnum)
{
	struct prop *prop;
	struct modelnode *node;

	if (!chr || !chr->model || !chr->model->definition) {
		return;
	}

	prop = chr->weapons_held[handnum];

	if (!prop || !prop->weapon || !prop->weapon->base.model) {
		return;
	}

	if (chr->model->definition->skel == &g_SkelChr) {
		node = modelGetPart(chr->model->definition,
				handnum == HAND_RIGHT ? MODELPART_CHR_RIGHTHAND : MODELPART_CHR_LEFTHAND);
	} else if (chr->model->definition->skel == &g_SkelSkedar) {
		node = modelGetPart(chr->model->definition,
				handnum == HAND_RIGHT ? MODELPART_SKEDAR_RIGHTHAND : MODELPART_SKEDAR_LEFTHAND);
	} else {
		return;
	}

	if (!node) {
		return;
	}

	prop->weapon->base.model->attachedtomodel = chr->model;
	prop->weapon->base.model->attachedtonode = node;
}

static void termReattachHeldWeapons(struct chrdata *chr)
{
	termReattachHeldWeapon(chr, HAND_RIGHT);
	termReattachHeldWeapon(chr, HAND_LEFT);
}

static void termApplyTerminationModelSwap(struct chrdata *chr)
{
	struct model *model;
	f32 angle;

	if (!chr || !chr->prop || !chr->model) {
		return;
	}

	model = bodyAllocateModel(TERM_TRANSFORM_BODY, TERM_TRANSFORM_HEAD, 0);

	if (!model) {
		return;
	}

	angle = modelGetChrRotY(chr->model);

	modelSetAnim70(model, chr0f01f378);
	model->chr = chr;
	model->unk01 = 1;
	modelSetAnimPlaySpeed(model, 1.0f, 0);
	modelSetRootPosition(model, &chr->prop->pos);
	modelSetChrRotY(model, angle);

	chr->model = model;
	chr->bodynum = TERM_TRANSFORM_BODY;
	chr->headnum = TERM_TRANSFORM_HEAD;
	chr->race = bodyGetRace(chr->bodynum);
	chr->voicebox = bodyChooseVoicebox(chr->bodynum, chr->headnum);
	chr->noblood = true;
	termReattachHeldWeapons(chr);

	if (chr->aibot) {
		chr->myaction = MA_AIBOTMAINLOOP;
		chr->aibot->command = AIBOTCMD_ATTACK;
		chr->aibot->forcemainloop = true;
		chr->aibot->distmode = -1;
		chr->aibot->shootdelaytimer60 = TICKS(60);
	}
}

static void termTryTransformTermination(struct chrdata *chr)
{
	if (!chr || !chr->prop || g_ScenarioData.term.transformed || chrIsDead(chr) || chr->actiontype == ACT_DIE) {
		return;
	}

	if (chr->maxdamage <= 0.0f) {
		return;
	}

	if (chr->damage < chr->maxdamage * (1.0f - TERM_TRANSFORM_FRACTION)) {
		return;
	}

	g_ScenarioData.term.transformed = true;

	explosionCreateSimple(chr->prop, &chr->prop->pos, chr->prop->rooms, TERM_TRANSFORM_EXPLOSION, g_Vars.currentplayernum);
	termApplyTerminationModelSwap(chr);
	termArmTerminationChr(chr);
}

static void termConfigureBot(s32 botnum)
{
	struct mpchrconfig *mpchr = &g_BotConfigsArray[botnum].base;
	struct chrdata *chr = termGetBotChrByNum(botnum);

	if (botnum == TERM_TERMINATION_BOTNUM) {
		g_BotConfigsArray[botnum].type = BOTTYPE_KAZE;
		mpchr->team = TERM_TERMINATION_TEAM;
		mpchr->mpbodynum = g_ScenarioData.term.transformed ? TERM_TRANSFORM_MPBODY : MPBODY_EVIL_CYBORG;
		mpchr->mpheadnum = g_ScenarioData.term.transformed ? TERM_TRANSFORM_MPHEAD : MPHEAD_EVIL_CYBORG;
		mpSetBotDifficulty(botnum, BOTDIFF_HARD);
		mpSetBotCustomName(botnum, "TERMINATION");

		if (chr) {
			chr->team = 1 << TERM_TERMINATION_TEAM;
			termMakeChrDamageable(chr);

			if (!chrIsDead(chr) && chr->actiontype != ACT_DIE) {
				chr->maxdamage = TERM_TERMINATION_HEALTH;

				if (chr->damage > TERM_TERMINATION_HEALTH) {
					chr->damage = TERM_TERMINATION_HEALTH;
				}

				if (g_ScenarioData.term.transformed && chr->bodynum != TERM_TRANSFORM_BODY) {
					termApplyTerminationModelSwap(chr);
				}

				termArmTerminationChr(chr);
			}
		}
	} else {
		g_BotConfigsArray[botnum].type = BOTTYPE_PEACE;
		mpchr->team = TERM_VICTIM_TEAM;
		mpchr->mpbodynum = MPBODY_CASSANDRA;
		mpchr->mpheadnum = mpGetMpheadnumByMpbodynum(mpchr->mpbodynum);
		mpSetBotDifficulty(botnum, BOTDIFF_MEAT);
		mpSetBotCustomName(botnum, "VICTIM");

		if (chr) {
			chr->team = 1 << TERM_VICTIM_TEAM;
			termPacifyVictimChr(chr);
		}
	}
}

static void termEnforceTeams(void)
{
	s32 i;

	for (i = 0; i < MAX_PLAYERS; i++) {
		if ((g_MpSetup.chrslots & MPCHRSLOT_BIT(i)) == 0) {
			continue;
		}

		g_PlayerConfigsArray[i].base.team = TERM_HUMAN_TEAM;
		g_Vars.mpplayerteams[i] = TERM_HUMAN_TEAM;

		if (g_MpAllChrPtrs[i]) {
			g_MpAllChrPtrs[i]->team = 1 << TERM_HUMAN_TEAM;
		}
	}

	for (i = 0; i < MAX_BOTS_RUNTIME; i++) {
		if (g_MpSetup.chrslots & MPCHRSLOT_BIT(MAX_PLAYERS + i)) {
			termConfigureBot(i);
		}
	}
}

static struct chrdata *termGetTerminationChr(void)
{
	if (g_BotCount <= TERM_TERMINATION_BOTNUM) {
		return NULL;
	}

	return termGetBotChrByNum(TERM_TERMINATION_BOTNUM);
}

static s32 termCountRemainingVictimLives(void)
{
	s32 totallives = 0;
	s32 i;

	for (i = TERM_TERMINATION_BOTNUM + 1; i < MAX_BOTS_RUNTIME; i++) {
		s32 chrslot = MAX_PLAYERS + i;
		s32 remaininglives = mpGetRemainingLivesForChrSlot(chrslot);

		if (remaininglives > 0) {
			totallives += remaininglives;
		}
	}

	return totallives;
}

static bool termHasActiveVictimChr(void)
{
	s32 i;

	for (i = 0; i < g_BotCount; i++) {
		struct chrdata *chr = g_MpBotChrPtrs[i];

		if (termIsVictimChr(chr) && !chrIsDead(chr)) {
			return true;
		}
	}

	return false;
}

static bool termIsTerminationGone(struct chrdata *termination)
{
	if (!termination) {
		return true;
	}

	if (termination->hidden & CHRHFLAG_DELETING) {
		return true;
	}

	if ((termination->actiontype == ACT_DEAD || termination->actiontype == ACT_DRUGGEDKO)
			&& termination->fadealpha <= 0) {
		return true;
	}

	return false;
}

static void termEndMatch(bool waswin)
{
	if (g_ScenarioData.term.matchending) {
		return;
	}

	g_ScenarioData.term.matchending = true;
	g_ScenarioData.term.waswin = waswin;
	g_NumReasonsToEndMpMatch++;

	mainEndStage();
}

void termInit(void)
{
	s32 i;

	g_MpSetup.options |= MPOPTION_TEAMSENABLED;
	g_MpSetup.options &= ~MPOPTION_ONEHITHEADSHOTS;
	g_Vars.mpquickteam = MPQUICKTEAM_PLAYERSVSSIMS;
	g_MpSetup.timelimit = 60;
	g_MpSetup.scorelimit = 100;
	g_MpSetup.teamscorelimit = 400;
	g_ScenarioData.term.matchending = false;
	g_ScenarioData.term.waswin = false;
	g_ScenarioData.term.bossdefeated = false;
	g_ScenarioData.term.transformed = false;

	for (i = 0; i < MAX_PLAYERS; i++) {
		g_PlayerConfigsArray[i].base.team = TERM_HUMAN_TEAM;
		g_PlayerConfigsArray[i].base.displayoptions &= ~MPDISPLAYOPTION_HIGHLIGHTTEAMS;
		g_Vars.mpplayerteams[i] = TERM_HUMAN_TEAM;
	}

	termEnforceTeams();
}

void termTick(void)
{
	struct chrdata *termination;

	termEnforceTeams();
	termination = termGetTerminationChr();

	if (g_ScenarioData.term.matchending || g_NumReasonsToEndMpMatch > 0) {
		return;
	}

	if (g_ScenarioData.term.bossdefeated) {
		if (termIsTerminationGone(termination)) {
			termEndMatch(true);
		}

		return;
	}

	if (!termination
			|| termination->damage >= termination->maxdamage
			|| !mpHasRemainingLives(MAX_PLAYERS)
			|| chrIsDead(termination)) {
		g_ScenarioData.term.bossdefeated = true;
		return;
	}

	termTryTransformTermination(termination);

	if (g_BotCount < TERM_MIN_BOTS) {
		return;
	}

	if (termCountRemainingVictimLives() <= 0 || !termHasActiveVictimChr()) {
		termEndMatch(false);
	}
}

void termTickChr(struct chrdata *chr)
{
	if (!chr || !chr->aibot) {
		return;
	}

	if (chr->aibot->aibotnum == TERM_TERMINATION_BOTNUM) {
		termArmTerminationChr(chr);
	} else {
		termPacifyVictimChr(chr);
	}
}

bool termRadarChr(Gfx **gdl, struct prop *prop)
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

	if (prop->chr->aibot->aibotnum == TERM_TERMINATION_BOTNUM) {
		colour = 0xff000000;
	} else {
		colour = 0x00ff0000;
	}

	*gdl = radarDrawDot(*gdl, prop, &pos, colour, 0, 0);
	return true;
}

Gfx *termRenderHud(Gfx *gdl)
{
	struct chrdata *termination = termGetTerminationChr();
	s32 x = viGetViewLeft() + 8;
	s32 y = viGetViewTop() + 8;
	s32 textwidth;
	s32 textheight;
	char text[64];

	if (g_BotCount < TERM_MIN_BOTS) {
		strcpy(text, "ADD AT LEAST 2 SIMS");
	} else {
		sprintf(text, "VICTIM LIVES: %d", termCountRemainingVictimLives());
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
	gdl = textRender(gdl, &x, &y, text, g_CharsHandelGothicXs, g_FontHandelGothicXs, 0x00ff00a0, 0x000000a0, viGetWidth(), viGetHeight(), 0, 0);
#else
	{
		s32 x2 = x + textwidth;
		s32 y2 = y + textheight;
		gdl = text0f153858(gdl, &x, &y, &x2, &y2);
		gdl = textRender(gdl, &x, &y, text, g_CharsHandelGothicXs, g_FontHandelGothicXs, 0x00ff00a0, 0x88, viGetWidth(), viGetHeight(), 0, 0);
	}
#endif
	gdl = text0f153780(gdl);

	if (termination && !chrIsDead(termination)) {
		char label[] = "TERMINATION";
		s32 viewleft = viGetViewLeft();
		s32 viewtop = viGetViewTop();
		s32 viewwidth = viGetViewWidth();
		s32 barwidth = viewwidth * 2 / 3;
		s32 barheight = 8;
		s32 barleft;
		s32 bartop = viewtop + 26;
		s32 barright;
		s32 barbottom = bartop + barheight;
		s32 fillright;
		f32 healthfrac;

		if (barwidth > 220) {
			barwidth = 220;
		}

		barleft = viewleft + (viewwidth - barwidth) / 2;
		barright = barleft + barwidth;
		healthfrac = (termination->maxdamage - termination->damage) / termination->maxdamage;

		if (healthfrac < 0.0f) {
			healthfrac = 0.0f;
		} else if (healthfrac > 1.0f) {
			healthfrac = 1.0f;
		}

		fillright = barleft + 2 + (s32)((barwidth - 4) * healthfrac);

		x = barleft;
		y = bartop - 11;
		gdl = text0f153628(gdl);
		gdl = textRender(gdl, &x, &y, label, g_CharsHandelGothicXs, g_FontHandelGothicXs, 0xff3030e0, 0x000000a0, viGetWidth(), viGetHeight(), 0, 0);

		gdl = text0f153838(gdl);
		gdl = textSetPrimColour(gdl, 0x200000b0);
		gDPFillRectangle(gdl++, barleft, bartop, barright, barbottom);
		gdl = textSetPrimColour(gdl, 0xff3030e0);
		gDPFillRectangle(gdl++, barleft + 2, bartop + 2, fillright, barbottom - 2);
		gdl = textSetPrimColour(gdl, 0xffffffff);
		gDPFillRectangle(gdl++, barleft, bartop, barright, bartop + 1);
		gDPFillRectangle(gdl++, barleft, barbottom - 1, barright, barbottom);
		gDPFillRectangle(gdl++, barleft, bartop, barleft + 1, barbottom);
		gDPFillRectangle(gdl++, barright - 1, bartop, barright, barbottom);
		gdl = text0f153780(gdl);
	}

#ifndef PLATFORM_N64
	gSPClearExtraGeometryModeEXT(gdl++, G_ASPECT_MODE_EXT);
#endif

	return gdl;
}
