// Perfect Dark Renaissance multiplayer extensions
// Updated: 2026-03-17
/**
 * Rescue the Hostages
 *
 * Prototype scenario:
 * - Forces the GoldenEye X Facility map.
 * - Spawns five activatable rescue chairs at fixed Facility pads.
 * - Players activate a chair, remain close for 10 seconds total, and score a rescue.
 * - Rescue progress pauses if the player moves away, instead of resetting.
 * - Rescued chairs immediately relocate elsewhere on the map.
 *
 * This is foundation work for a fuller mode with real seated hostage chrs.
 */

#define RTH_MAX_HOSTAGES 10
#define RTH_DEFAULT_HOSTAGES 5
#define RTH_NUM_HOSTAGE_PADS 12
#define RTH_RESCUE_TIME240 (5 * TICKS(240))
#define RTH_EXECUTION_TIME240 (60 * TICKS(240))
#define RTH_ESCAPE_TIME240 (6 * TICKS(240))
#define RTH_DEAD_RESPAWN_TIME240 (7 * TICKS(240))
#define RTH_HOSTAGE_LOSS_LIMIT 3
#define RTH_BOMBS_PER_HOSTAGE 1
#define RTH_VAPORIZE_WAIT240 TICKS(20)
#define RTH_SPLATTYPE_PUDDLE 1
#define RTH_DEFUSE_WIRES 3
#define RTH_DEFUSE_RANGE_XZ 100
#define RTH_DEFUSE_RANGE_Y 120
#define RTH_DEFUSE_STICK_COOLDOWN240 TICKS(18)
#define RTH_DEFUSE_WINDOW240 (20 * TICKS(240))

static s16 g_RthFacilityHostagePads[RTH_NUM_HOSTAGE_PADS] = {
	PAD_MP5_0006,
	PAD_MP5_000D,
	PAD_MP5_0014,
	PAD_MP5_001B,
	PAD_MP5_0022,
	PAD_MP5_0029,
	PAD_MP5_0030,
	PAD_MP5_0037,
	PAD_MP5_001E,
	PAD_MP5_0020,
	PAD_MP5_0024,
	PAD_MP5_0027,
};

extern struct menudialogdef g_ExtGameOptionsMenuDialog;

static s32 g_RthConfiguredHostageCount = RTH_DEFAULT_HOSTAGES;

static s16 g_RthHostageBodies[] = {
	BODY_SECRETARY,
	BODY_OFFICEWORKER,
	BODY_OFFICEWORKER2,
	BODY_STEWARDESS,
	BODY_CIFEMTECH,
};

static s16 g_RthHostageHeads[] = {
	HEAD_SECRETARY,
	HEAD_RANDOM,
	HEAD_RANDOM,
	HEAD_LAURA,
	HEAD_JULIANNE,
};

static const char *g_RthDefuseWireNames[RTH_DEFUSE_WIRES] = {
	"RED",
	"BLUE",
	"YELLOW",
};

static const u32 g_RthDefuseWireColours[RTH_DEFUSE_WIRES] = {
	0xff4040c0,
	0x4080ffc0,
	0xffd840c0,
};

static bool rthHostageChrIsDead(struct prop *prop);
static void rthStopRescueSound(s32 playernum);
static void rthExitDefuseMode(s32 playernum);
static void rthCompleteRescue(s32 playernum, s32 hostageindex);
static s16 rthChooseRespawnPad(s32 hostageindex);
static void rthStartEscapingHostage(s32 hostageindex);
static void rthMoveHostageToPad(s32 hostageindex, s16 padnum);
static void rthStopExecutionBeep(s32 hostageindex);
static bool rthWouldRescueEndMatch(s32 playernum);

MenuItemHandlerResult menuhandlerMpRthHostageCount(s32 operation, struct menuitem *item, union handlerdata *data)
{
	switch (operation) {
	case MENUOP_GETSLIDER:
		data->slider.value = g_RthConfiguredHostageCount - 1;
		break;
	case MENUOP_SET:
		g_RthConfiguredHostageCount = data->slider.value + 1;
		if (g_RthConfiguredHostageCount < 1) {
			g_RthConfiguredHostageCount = 1;
		} else if (g_RthConfiguredHostageCount > RTH_MAX_HOSTAGES) {
			g_RthConfiguredHostageCount = RTH_MAX_HOSTAGES;
		}
		g_ScenarioData.rth.numhostages = g_RthConfiguredHostageCount;
		break;
	case MENUOP_GETSLIDERLABEL:
		sprintf(data->slider.label, "%d\n", data->slider.value + 1);
		break;
	}

	return 0;
}

struct menuitem g_RthOptionsMenuItems[] = {
	{
		MENUITEMTYPE_SLIDER,
		0,
		MENUITEMFLAG_LOCKABLEMINOR | MENUITEMFLAG_LITERAL_TEXT,
		(uintptr_t)"Hostages",
		RTH_MAX_HOSTAGES - 1,
		menuhandlerMpRthHostageCount,
	},
	{
		MENUITEMTYPE_CHECKBOX,
		0,
		MENUITEMFLAG_LOCKABLEMINOR,
		L_MPMENU_222, // "One-Hit Kills"
		MPOPTION_ONEHITKILLS,
		menuhandlerMpOneHitKills,
	},
	{
		MENUITEMTYPE_CHECKBOX,
		0,
		MENUITEMFLAG_LOCKABLEMINOR | MENUITEMFLAG_LITERAL_TEXT,
		(uintptr_t)"One-Hit Headshots",
		MPOPTION_ONEHITHEADSHOTS,
		menuhandlerMpOneHitHeadshots,
	},
	{
		MENUITEMTYPE_DROPDOWN,
		0,
		MENUITEMFLAG_LOCKABLEMINOR,
		L_MPMENU_223, // "Slow Motion"
		0,
		menuhandlerMpSlowMotion,
	},
	{
		MENUITEMTYPE_CHECKBOX,
		0,
		MENUITEMFLAG_LOCKABLEMINOR,
		L_MPMENU_224, // "Fast Movement"
		MPOPTION_FASTMOVEMENT,
		menuhandlerMpCheckboxOption,
	},
	{
		MENUITEMTYPE_CHECKBOX,
		0,
		MENUITEMFLAG_LOCKABLEMINOR,
		L_MPMENU_225, // "Display Team"
		MPOPTION_DISPLAYTEAM,
		menuhandlerMpDisplayTeam,
	},
	{
		MENUITEMTYPE_CHECKBOX,
		0,
		MENUITEMFLAG_LOCKABLEMINOR,
		L_MPMENU_226, // "No Radar"
		MPOPTION_NORADAR,
		menuhandlerMpCheckboxOption,
	},
	{
		MENUITEMTYPE_CHECKBOX,
		0,
		MENUITEMFLAG_LOCKABLEMINOR,
		L_MPMENU_227, // "No Auto-Aim"
		MPOPTION_NOAUTOAIM,
		menuhandlerMpCheckboxOption,
	},
	{
		MENUITEMTYPE_SEPARATOR,
		0,
		0,
		0,
		0,
		NULL,
	},
	{
		MENUITEMTYPE_SELECTABLE,
		0,
		MENUITEMFLAG_SELECTABLE_CLOSESDIALOG,
		L_MPMENU_239, // "Back"
		0,
		NULL,
	},
	{ MENUITEMTYPE_END },
};

struct menudialogdef g_RthOptionsMenuDialog = {
	MENUDIALOGTYPE_DEFAULT,
	(uintptr_t)"Rescue Options",
	g_RthOptionsMenuItems,
	mpOptionsMenuDialog,
	MENUDIALOGFLAG_MPLOCKABLE | MENUDIALOGFLAG_LITERAL_TEXT,
	&g_ExtGameOptionsMenuDialog,
};

static bool rthCanStartDefuse(s32 playernum, s32 *hostageindexptr)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	s32 i;

	if (playernum < 0 || playernum >= PLAYERCOUNT()) {
		return false;
	}

	if (g_Vars.currentplayer == NULL || g_Vars.currentplayer->prop == NULL) {
		return false;
	}

	if (bmoveGetCrouchPos() == CROUCHPOS_STAND) {
		return false;
	}

	for (i = 0; i < ARRAYCOUNT(data->hostages); i++) {
		struct prop *bombprop = data->hostagebombprops[i][0];
		struct coord dist;
		f32 rangexz;
		f32 rangey;

		if (bombprop == NULL) {
			continue;
		}

		if (data->executiontimers240[i] <= 0 || data->executiontimers240[i] > RTH_DEFUSE_WINDOW240) {
			continue;
		}

		dist.x = bombprop->pos.x - g_Vars.currentplayer->prop->pos.x;
		dist.y = bombprop->pos.y - g_Vars.currentplayer->prop->pos.y;
		dist.z = bombprop->pos.z - g_Vars.currentplayer->prop->pos.z;

		rangexz = sqrtf(dist.x * dist.x + dist.z * dist.z);
		rangey = ABS(dist.y);

		if (rangexz <= RTH_DEFUSE_RANGE_XZ && rangey <= RTH_DEFUSE_RANGE_Y) {
			if (hostageindexptr != NULL) {
				*hostageindexptr = i;
			}

			return true;
		}
	}

	return false;
}

static void rthStopExecutionBeep(s32 hostageindex)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;

	if (hostageindex < 0 || hostageindex >= ARRAYCOUNT(data->executionbeepaudiohandles)) {
		return;
	}

	if (data->executionbeepaudiohandles[hostageindex] != NULL) {
		audioStop(data->executionbeepaudiohandles[hostageindex]);
		data->executionbeepaudiohandles[hostageindex] = NULL;
	}
}

static void rthExitDefuseMode(s32 playernum)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;

	if (playernum < 0 || playernum >= PLAYERCOUNT()) {
		return;
	}

	data->activedefusehostage[playernum] = -1;
	data->defuseselectedwire[playernum] = 0;
	data->defusetargetwire[playernum] = 0;
	data->defusestickcooldown240[playernum] = 0;
}

static void rthCompleteRescue(s32 playernum, s32 hostageindex)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	s16 newpadnum;
	bool wonmatch;

	rthStopRescueSound(playernum);
	rthStopExecutionBeep(hostageindex);
	rthExitDefuseMode(playernum);
	data->rescuedcount[playernum]++;
	data->activehostage[playernum] = -1;
	data->rescuetime240[playernum] = 0;
	wonmatch = rthWouldRescueEndMatch(playernum);

	if (!wonmatch) {
		rthStartEscapingHostage(hostageindex);
		newpadnum = rthChooseRespawnPad(hostageindex);
		rthMoveHostageToPad(hostageindex, newpadnum);
	}

#if VERSION >= VERSION_JPN_FINAL
	hudmsgCreateWithFlags("Hostage rescued.\n", HUDMSGTYPE_MPSCENARIO, HUDMSGFLAG_ONLYIFALIVE | HUDMSGFLAG_NOWRAP);
#else
	hudmsgCreateWithFlags("Hostage rescued.\n", HUDMSGTYPE_MPSCENARIO, HUDMSGFLAG_ONLYIFALIVE);
#endif
	snd00010718(NULL, 0, AL_VOL_FULL, AL_PAN_CENTER, SFX_0060, 1, 1, -1, 1);
	snd00010718(NULL, 0, AL_VOL_FULL, AL_PAN_CENTER, SFX_0291, 1, 1, -1, 1);

	if (wonmatch && !data->matchending) {
		data->matchending = true;
		g_NumReasonsToEndMpMatch++;
	}
}

static bool rthWouldRescueEndMatch(s32 playernum)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	s32 scorelimit = g_MpSetup.scorelimit < 100 ? g_MpSetup.scorelimit + 1 : 0;
	s32 teamscorelimit = g_MpSetup.teamscorelimit < 400 ? mpCalculateTeamScoreLimit() + 1 : 0;
	s32 playerscore;
	s32 teamscore = 0;
	s32 team;
	s32 i;

	if (playernum < 0 || playernum >= PLAYERCOUNT()) {
		return false;
	}

	playerscore = data->rescuedcount[playernum] - data->lostpenaltycount[playernum];

	if (scorelimit > 0 && playerscore >= scorelimit) {
		return true;
	}

	if ((g_MpSetup.options & MPOPTION_TEAMSENABLED) == 0 || teamscorelimit <= 0) {
		return false;
	}

	team = g_PlayerConfigsArray[playernum].base.team;

	for (i = 0; i < PLAYERCOUNT(); i++) {
		if ((g_MpSetup.chrslots & (1 << i)) && g_PlayerConfigsArray[i].base.team == team) {
			teamscore += data->rescuedcount[i] - data->lostpenaltycount[i];
		}
	}

	return teamscore >= teamscorelimit;
}

static void rthDestroyHostageBomb(s32 hostageindex)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	s32 i;

	if (hostageindex < 0 || hostageindex >= ARRAYCOUNT(data->hostagebombprops)) {
		return;
	}

	for (i = 0; i < ARRAYCOUNT(data->hostagebombprops[hostageindex]); i++) {
		struct prop *prop = data->hostagebombprops[hostageindex][i];

		if (prop) {
			objFree(prop->obj, true, false);
			data->hostagebombprops[hostageindex][i] = NULL;
		}
	}
}

static void rthSpawnHostageBombAtPad(s32 hostageindex, s16 padnum)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	struct pad pad;
	f32 forwardx;
	f32 forwardz;
	f32 forwardlen;
	f32 rightx;
	f32 rightz;
	f32 rightlen;
	s32 i;
	static const f32 xoffsets[RTH_BOMBS_PER_HOSTAGE] = { 0.0f };
	static const f32 zoffsets[RTH_BOMBS_PER_HOSTAGE] = { 0.0f };

	if (hostageindex < 0 || hostageindex >= ARRAYCOUNT(data->hostagebombprops) || padnum < 0) {
		return;
	}

	rthDestroyHostageBomb(hostageindex);
	padUnpack(padnum, PADFIELD_POS | PADFIELD_LOOK | PADFIELD_ROOM, &pad);
	forwardx = pad.look.x;
	forwardz = pad.look.z;
	forwardlen = sqrtf(forwardx * forwardx + forwardz * forwardz);
	rightx = pad.look.z;
	rightz = -pad.look.x;
	rightlen = sqrtf(rightx * rightx + rightz * rightz);

	if (forwardlen > 0.0f) {
		forwardx /= forwardlen;
		forwardz /= forwardlen;
	} else {
		forwardx = 0.0f;
		forwardz = 1.0f;
	}

	if (rightlen > 0.0f) {
		rightx /= rightlen;
		rightz /= rightlen;
	} else {
		rightx = 1.0f;
		rightz = 0.0f;
	}

	for (i = 0; i < RTH_BOMBS_PER_HOSTAGE; i++) {
		struct prop *prop = scenarioCreateObj(
				MODEL_EXPLOSIVEBRICK,
				padnum,
				0.08f,
				OBJFLAG_INVINCIBLE | OBJFLAG_FORCENOBOUNCE,
				OBJFLAG2_IMMUNETOGUNFIRE | OBJFLAG2_IMMUNETOEXPLOSIONS,
				0);

		data->hostagebombprops[hostageindex][i] = prop;

		if (prop) {
			prop->pos.x += forwardx * 0.8f;
			prop->pos.z += forwardz * 0.8f;
			prop->pos.x += rightx * xoffsets[i];
			prop->pos.z += rightz * xoffsets[i];
			prop->pos.x += forwardx * zoffsets[i];
			prop->pos.z += forwardz * zoffsets[i];
			prop->pos.y -= 0.4f;

			if (prop->obj && prop->obj->model) {
				modelSetRootPosition(prop->obj->model, &prop->pos);
			}
		}
	}
}

bool rthIsScenarioHostageChr(struct chrdata *chr)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	s32 i;

	if (g_MpSetup.scenario != MPSCENARIO_RESCUEHOSTAGES || chr == NULL) {
		return false;
	}

	for (i = 0; i < ARRAYCOUNT(data->hostagechrprops); i++) {
		if ((data->hostagechrprops[i] && data->hostagechrprops[i]->chr == chr)
				|| (data->escapinghostagechrprops[i] && data->escapinghostagechrprops[i]->chr == chr)) {
			return true;
		}
	}

	return false;
}

static void rthDestroyHostageChr(s32 hostageindex)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	struct prop *prop;

	if (hostageindex < 0 || hostageindex >= ARRAYCOUNT(data->hostagechrprops)) {
		return;
	}

	prop = data->hostagechrprops[hostageindex];

	if (prop) {
		chrRemove(prop, true);
		propDeregisterRooms(prop);
		propDelist(prop);
		propDisable(prop);
		propFree(prop);
		data->hostagechrprops[hostageindex] = NULL;
	}
}

static void rthDestroyEscapingHostageChr(s32 escapingindex)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	struct prop *prop;

	if (escapingindex < 0 || escapingindex >= ARRAYCOUNT(data->escapinghostagechrprops)) {
		return;
	}

	prop = data->escapinghostagechrprops[escapingindex];

	if (prop) {
		chrRemove(prop, true);
		propDeregisterRooms(prop);
		propDelist(prop);
		propDisable(prop);
		propFree(prop);
		data->escapinghostagechrprops[escapingindex] = NULL;
	}

	data->escapinghostagetimers240[escapingindex] = 0;
	data->escapinghostagestates[escapingindex] = 0;
}

static void rthStartEscapingHostage(s32 hostageindex)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	struct prop *prop = data->hostagechrprops[hostageindex];
	struct chrdata *chr;
	struct pad pad;
	f32 bestsqdist = -1;
	s32 slot = -1;
	s32 i;

	if (prop == NULL || prop->chr == NULL) {
		data->hostagechrprops[hostageindex] = NULL;
		return;
	}

	for (i = 0; i < ARRAYCOUNT(data->escapinghostagechrprops); i++) {
		if (data->escapinghostagechrprops[i] == NULL) {
			slot = i;
			break;
		}
	}

	if (slot < 0) {
		slot = 0;
		rthDestroyEscapingHostageChr(slot);
	}

	chr = prop->chr;
	data->hostagechrprops[hostageindex] = NULL;
	data->escapinghostagechrprops[slot] = prop;
	data->escapinghostagetimers240[slot] = RTH_ESCAPE_TIME240;
	data->escapinghostagestates[slot] = 0;
	chr->myspecial = -1;
	chr->specialdie = SPECIALDIE_NONE;
	chr->hidden |= CHRHFLAG_DONTSHOOTME | CHRHFLAG_ANTINONINTERACTABLE;
	chr->chrflags &= ~CHRCFLAG_INVINCIBLE;
	chr->team = TEAM_NONCOMBAT;
	chr->ailist = ailistFindById(GAILIST_IDLE);
	data->escapinghostagedstpos[slot] = prop->pos;
	roomsCopy(prop->rooms, data->escapinghostagedstrooms[slot]);

	for (i = 0; i < ARRAYCOUNT(g_RthFacilityHostagePads); i++) {
		struct pad testpad;
		f32 xdiff;
		f32 ydiff;
		f32 zdiff;
		f32 sqdist;

		padUnpack(g_RthFacilityHostagePads[i], PADFIELD_POS | PADFIELD_ROOM, &testpad);
		xdiff = testpad.pos.x - prop->pos.x;
		ydiff = testpad.pos.y - prop->pos.y;
		zdiff = testpad.pos.z - prop->pos.z;
		sqdist = xdiff * xdiff + ydiff * ydiff + zdiff * zdiff;

		if (sqdist > bestsqdist) {
			bestsqdist = sqdist;
			data->escapinghostagedstpos[slot] = testpad.pos;
			data->escapinghostagedstrooms[slot][0] = testpad.room;
			data->escapinghostagedstrooms[slot][1] = -1;
		}
	}

	chrStartAnim(chr, ANIM_STAND_UP_FROM_SITTING, 0, -1, CHRANIMFLAG_MOVEWHENINVIS, 16, 1);
}

static void rthSpawnHostageChrAtPad(s32 hostageindex, s16 padnum)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	struct pad pad;
	struct prop *prop;
	struct chrdata *chr;
	RoomNum rooms[2];
	f32 angle;

	if (hostageindex < 0 || hostageindex >= RTH_MAX_HOSTAGES || padnum < 0) {
		return;
	}

	rthDestroyHostageChr(hostageindex);

	padUnpack(padnum, PADFIELD_POS | PADFIELD_LOOK | PADFIELD_ROOM, &pad);
	angle = atan2f(pad.look.x, pad.look.z);
	rooms[0] = pad.room;
	rooms[1] = -1;

	prop = chrSpawnAtCoord(
			g_RthHostageBodies[hostageindex % ARRAYCOUNT(g_RthHostageBodies)],
			g_RthHostageHeads[hostageindex % ARRAYCOUNT(g_RthHostageHeads)],
			&pad.pos,
			rooms,
			angle,
			ailistFindById(GAILIST_DO_SITTING_ANIMATION),
			SPAWNFLAG_ALLOWONSCREEN | SPAWNFLAG_FIXEDHEIGHT | SPAWNFLAG_ANTINONINTERACTABLE | SPAWNFLAG_DONTSHOOTME | SPAWNFLAG_NOBLOOD);

	data->hostagechrprops[hostageindex] = prop;

	if (prop && prop->chr) {
		chr = prop->chr;
		chr->hidden |= CHRHFLAG_DONTSHOOTME | CHRHFLAG_ANTINONINTERACTABLE;
		chr->team = TEAM_NONCOMBAT;
		chr->noblood = true;
		chr->myspecial = (data->hostages[hostageindex] && data->hostages[hostageindex]->obj)
			? objGetTagNum(data->hostages[hostageindex]->obj)
			: -1;
		chr->specialdie = SPECIALDIE_ONCHAIR;
	}
}

static void rthMoveHostageToPad(s32 hostageindex, s16 padnum)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	struct prop *prop;

	if (hostageindex < 0 || hostageindex >= ARRAYCOUNT(data->hostages)) {
		return;
	}

	prop = data->hostages[hostageindex];

	if (padnum < 0) {
		return;
	}

	if (prop && prop->obj) {
		objFree(prop->obj, true, false);
	}

	rthDestroyHostageBomb(hostageindex);
	rthDestroyHostageChr(hostageindex);

	data->hostages[hostageindex] = scenarioCreateObj(
			MODEL_LAB_CHAIR,
			padnum,
			0.72f,
			OBJFLAG_INVINCIBLE,
			OBJFLAG2_IMMUNETOGUNFIRE | OBJFLAG2_IMMUNETOEXPLOSIONS,
			OBJFLAG3_INTERACTABLE | OBJFLAG3_RESCUEHOSTAGE);

	htbRemoveAmmoCrateAtPad(padnum);
	data->hostagepadnums[hostageindex] = padnum;
	data->executiontimers240[hostageindex] = RTH_EXECUTION_TIME240;
	data->deadhostagetimers240[hostageindex] = 0;
	rthStopExecutionBeep(hostageindex);
	rthSpawnHostageBombAtPad(hostageindex, padnum);
	rthSpawnHostageChrAtPad(hostageindex, padnum);
}

static void rthBroadcastHudmsg(const char *msg)
{
	s32 prevplayernum = g_Vars.currentplayernum;
	s32 i;

	for (i = 0; i < PLAYERCOUNT(); i++) {
		setCurrentPlayerNum(i);
#if VERSION >= VERSION_JPN_FINAL
		hudmsgCreateWithFlags((char *)msg, HUDMSGTYPE_MPSCENARIO, HUDMSGFLAG_ONLYIFALIVE | HUDMSGFLAG_NOWRAP);
#else
		hudmsgCreateWithFlags((char *)msg, HUDMSGTYPE_MPSCENARIO, HUDMSGFLAG_ONLYIFALIVE);
#endif
	}

	setCurrentPlayerNum(prevplayernum);
}

static void rthExecuteHostage(s32 hostageindex)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	struct prop *prop;
	struct prop *bombprop = NULL;
	s32 i;

	if (hostageindex < 0 || hostageindex >= ARRAYCOUNT(data->hostagechrprops)) {
		return;
	}

	prop = data->hostagechrprops[hostageindex];

	if (prop == NULL || prop->chr == NULL || rthHostageChrIsDead(prop)) {
		return;
	}

	for (i = 0; i < ARRAYCOUNT(data->hostagebombprops[hostageindex]); i++) {
		if (data->hostagebombprops[hostageindex][i] != NULL) {
			bombprop = data->hostagebombprops[hostageindex][i];
			break;
		}
	}

	if (bombprop != NULL) {
		rthStopExecutionBeep(hostageindex);
		explosionCreateSimple(bombprop, &bombprop->pos, bombprop->rooms, EXPLOSIONTYPE_9, 0);
	} else if (data->hostages[hostageindex] != NULL) {
		rthStopExecutionBeep(hostageindex);
		explosionCreateSimple(data->hostages[hostageindex], &data->hostages[hostageindex]->pos, data->hostages[hostageindex]->rooms, EXPLOSIONTYPE_9, 0);
	}

	data->executiontimers240[hostageindex] = 0;
	data->vaporizewaittimers240[hostageindex] = RTH_VAPORIZE_WAIT240;
	data->deadhostagetimers240[hostageindex] = 0;
	data->hostageslost++;

	for (i = 0; i < PLAYERCOUNT(); i++) {
		data->lostpenaltycount[i]++;
	}

	for (i = 0; i < PLAYERCOUNT(); i++) {
		if (data->activehostage[i] == hostageindex) {
			rthStopRescueSound(i);
			data->activehostage[i] = -1;
			data->rescuetime240[i] = 0;
		}

		if (data->activedefusehostage[i] == hostageindex) {
			rthExitDefuseMode(i);
		}
	}

	rthBroadcastHudmsg("Hostage executed.\n");

	if (data->hostageslost >= RTH_HOSTAGE_LOSS_LIMIT && !data->matchending) {
		data->matchending = true;
		rthBroadcastHudmsg("Too many hostages lost.\n");
		g_NumReasonsToEndMpMatch++;
	}
}

static s16 rthChooseRespawnPad(s32 hostageindex)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	s16 choices[ARRAYCOUNT(data->padnums)];
	s32 numchoices = 0;
	s32 i;
	s16 currentpad = data->hostagepadnums[hostageindex];

	if (g_MpSetup.stagenum == STAGE_EXTRA10) {
		for (i = 0; i < ARRAYCOUNT(g_RthFacilityHostagePads); i++) {
			s16 padnum = g_RthFacilityHostagePads[i];
			bool used = false;
			s32 j;

			if (padnum == currentpad) {
				continue;
			}

			for (j = 0; j < ARRAYCOUNT(data->hostagepadnums); j++) {
				if (j != hostageindex && data->hostagepadnums[j] == padnum) {
					used = true;
					break;
				}
			}

			if (!used) {
				choices[numchoices++] = padnum;
			}
		}

		if (numchoices > 0) {
			return choices[rngRandom() % numchoices];
		}
	}

	for (i = 0; i < data->numpads && i < ARRAYCOUNT(data->padnums); i++) {
		s16 padnum = data->padnums[i];
		bool used = false;
		s32 j;

		if (padnum < 0 || padnum == currentpad) {
			continue;
		}

		for (j = 0; j < ARRAYCOUNT(data->hostagepadnums); j++) {
			if (j != hostageindex && data->hostagepadnums[j] == padnum) {
				used = true;
				break;
			}
		}

		if (!used) {
			choices[numchoices++] = padnum;
		}
	}

	if (numchoices > 0) {
		return choices[rngRandom() % numchoices];
	}

	return currentpad;
}

static void rthStopRescueSound(s32 playernum)
{
	if (playernum >= 0 && playernum < PLAYERCOUNT() && g_ScenarioData.rth.rescueaudiohandles[playernum]) {
		audioStop(g_ScenarioData.rth.rescueaudiohandles[playernum]);
		g_ScenarioData.rth.rescueaudiohandles[playernum] = NULL;
	}

	if (playernum >= 0 && playernum < PLAYERCOUNT() && g_ScenarioData.rth.rescueaudiohandles2[playernum]) {
		audioStop(g_ScenarioData.rth.rescueaudiohandles2[playernum]);
		g_ScenarioData.rth.rescueaudiohandles2[playernum] = NULL;
	}
}

void rthInit(void)
{
	s32 i;

	g_MpSetup.options |= MPOPTION_TEAMSENABLED;
	g_Vars.mpquickteam = MPQUICKTEAM_PLAYERSVSSIMS;
	g_ScenarioData.rth.numpads = 0;
	g_ScenarioData.rth.numhostages = g_RthConfiguredHostageCount;

	for (i = 0; i < MAX_PLAYERS; i++) {
		g_PlayerConfigsArray[i].base.team = MPTEAM_0;
		g_Vars.mpplayerteams[i] = MPTEAM_0;
	}

	for (i = 0; i < ARRAYCOUNT(g_BotConfigsArray); i++) {
		g_BotConfigsArray[i].base.team = MPTEAM_1;
	}

	for (i = 0; i < ARRAYCOUNT(g_ScenarioData.rth.padnums); i++) {
		g_ScenarioData.rth.padnums[i] = -1;
	}

	for (i = 0; i < ARRAYCOUNT(g_ScenarioData.rth.hostagepadnums); i++) {
		g_ScenarioData.rth.hostagepadnums[i] = -1;
	}

	for (i = 0; i < ARRAYCOUNT(g_ScenarioData.rth.hostages); i++) {
		g_ScenarioData.rth.hostages[i] = NULL;
		{
			s32 j;
			for (j = 0; j < ARRAYCOUNT(g_ScenarioData.rth.hostagebombprops[i]); j++) {
				g_ScenarioData.rth.hostagebombprops[i][j] = NULL;
			}
		}
		g_ScenarioData.rth.hostagechrprops[i] = NULL;
		g_ScenarioData.rth.executiontimers240[i] = 0;
		g_ScenarioData.rth.vaporizewaittimers240[i] = 0;
		g_ScenarioData.rth.deadhostagetimers240[i] = 0;
		g_ScenarioData.rth.executionbeepaudiohandles[i] = NULL;
		g_ScenarioData.rth.escapinghostagechrprops[i] = NULL;
		g_ScenarioData.rth.escapinghostagetimers240[i] = 0;
		g_ScenarioData.rth.escapinghostagestates[i] = 0;
	}

	g_ScenarioData.rth.hostageslost = 0;
	g_ScenarioData.rth.matchending = false;

	for (i = 0; i < PLAYERCOUNT(); i++) {
		g_ScenarioData.rth.activehostage[i] = -1;
		g_ScenarioData.rth.rescuetime240[i] = 0;
		g_ScenarioData.rth.rescuedcount[i] = 0;
		g_ScenarioData.rth.lostpenaltycount[i] = 0;
		g_ScenarioData.rth.activedefusehostage[i] = -1;
		g_ScenarioData.rth.defuseselectedwire[i] = 0;
		g_ScenarioData.rth.defusetargetwire[i] = 0;
		g_ScenarioData.rth.defusestickcooldown240[i] = 0;
		g_ScenarioData.rth.rescueaudiohandles[i] = NULL;
		g_ScenarioData.rth.rescueaudiohandles2[i] = NULL;
	}
}

void rthReset(void)
{
	s32 i;

	g_ScenarioData.rth.numpads = 0;
	g_ScenarioData.rth.numhostages = g_RthConfiguredHostageCount;

	for (i = 0; i < ARRAYCOUNT(g_ScenarioData.rth.padnums); i++) {
		g_ScenarioData.rth.padnums[i] = -1;
	}

	for (i = 0; i < ARRAYCOUNT(g_ScenarioData.rth.hostagepadnums); i++) {
		g_ScenarioData.rth.hostagepadnums[i] = -1;
	}

	for (i = 0; i < ARRAYCOUNT(g_ScenarioData.rth.hostages); i++) {
		g_ScenarioData.rth.hostages[i] = NULL;
		{
			s32 j;
			for (j = 0; j < ARRAYCOUNT(g_ScenarioData.rth.hostagebombprops[i]); j++) {
				g_ScenarioData.rth.hostagebombprops[i][j] = NULL;
			}
		}
		g_ScenarioData.rth.hostagechrprops[i] = NULL;
		g_ScenarioData.rth.executiontimers240[i] = 0;
		g_ScenarioData.rth.vaporizewaittimers240[i] = 0;
		g_ScenarioData.rth.deadhostagetimers240[i] = 0;
		rthStopExecutionBeep(i);
		g_ScenarioData.rth.escapinghostagechrprops[i] = NULL;
		g_ScenarioData.rth.escapinghostagetimers240[i] = 0;
		g_ScenarioData.rth.escapinghostagestates[i] = 0;
	}

	g_ScenarioData.rth.hostageslost = 0;
	g_ScenarioData.rth.matchending = false;

	for (i = 0; i < PLAYERCOUNT(); i++) {
		rthStopRescueSound(i);
		g_ScenarioData.rth.activehostage[i] = -1;
		g_ScenarioData.rth.rescuetime240[i] = 0;
		g_ScenarioData.rth.lostpenaltycount[i] = 0;
		g_ScenarioData.rth.activedefusehostage[i] = -1;
		g_ScenarioData.rth.defuseselectedwire[i] = 0;
		g_ScenarioData.rth.defusetargetwire[i] = 0;
		g_ScenarioData.rth.defusestickcooldown240[i] = 0;
		g_ScenarioData.rth.rescueaudiohandles[i] = NULL;
		g_ScenarioData.rth.rescueaudiohandles2[i] = NULL;
	}
}

void rthAddPad(s16 padnum)
{
	if (g_ScenarioData.rth.numpads < ARRAYCOUNT(g_ScenarioData.rth.padnums)) {
		g_ScenarioData.rth.padnums[g_ScenarioData.rth.numpads] = padnum;
		g_ScenarioData.rth.numpads++;
	}
}

s32 rthNumProps(void)
{
	return g_RthConfiguredHostageCount;
}

static s16 rthChooseInitialPadForHostage(s32 hostageindex)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	s32 usable;
	s32 choiceindex;

	if (g_MpSetup.stagenum == STAGE_EXTRA10 && hostageindex < ARRAYCOUNT(g_RthFacilityHostagePads)) {
		return g_RthFacilityHostagePads[hostageindex];
	}

	if (data->numpads <= 0) {
		return -1;
	}

	usable = data->numpads;

	if (usable > 6) {
		usable -= 2;
		choiceindex = 2 + (hostageindex * usable) / g_RthConfiguredHostageCount;
	} else {
		choiceindex = (hostageindex * usable) / g_RthConfiguredHostageCount;
	}

	if (choiceindex >= data->numpads) {
		choiceindex = data->numpads - 1;
	}

	return data->padnums[choiceindex];
}

void rthInitProps(void)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	s32 i;

	data->numhostages = 0;

	for (i = 0; i < g_RthConfiguredHostageCount; i++) {
		s16 padnum = rthChooseInitialPadForHostage(i);

		if (padnum >= 0) {
			struct prop *prop = scenarioCreateObj(
					MODEL_LAB_CHAIR,
					padnum,
					0.72f,
					OBJFLAG_INVINCIBLE,
					OBJFLAG2_IMMUNETOGUNFIRE | OBJFLAG2_IMMUNETOEXPLOSIONS,
					OBJFLAG3_INTERACTABLE | OBJFLAG3_RESCUEHOSTAGE);

			data->hostages[i] = prop;
			data->hostagepadnums[i] = padnum;
			data->executiontimers240[i] = RTH_EXECUTION_TIME240;
			data->vaporizewaittimers240[i] = 0;
			data->deadhostagetimers240[i] = 0;
			data->numhostages++;
			htbRemoveAmmoCrateAtPad(padnum);
			rthSpawnHostageBombAtPad(i, padnum);
			rthSpawnHostageChrAtPad(i, padnum);
		}
	}
}

static bool rthHostageChrIsDead(struct prop *prop)
{
	if (prop == NULL || prop->chr == NULL) {
		return true;
	}

	return prop->chr->actiontype == ACT_DIE
		|| prop->chr->actiontype == ACT_DEAD
		|| prop->chr->actiontype == ACT_DRUGGEDKO;
}

void rthTickChr(struct chrdata *chr)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	s32 playernum;
	s32 hostageindex;
	s32 mpindex;
	s32 controlmode;
	s8 contpad1;
	s8 contpad2;
	u32 usemask;
	u32 usepressmask;

	if (chr) {
		return;
	}

	playernum = g_Vars.currentplayernum;
	mpindex = g_Vars.currentplayerstats ? g_Vars.currentplayerstats->mpindex : playernum;
	controlmode = optionsGetControlMode(mpindex);
	contpad1 = optionsGetContpadNum1(mpindex);
	contpad2 = optionsGetContpadNum2(mpindex);
	usemask = (controlmode == CONTROLMODE_PC)
		? (B_BUTTON | BUTTON_CANCEL_USE | BUTTON_ACCEPT_USE)
		: B_BUTTON;
	usepressmask = (controlmode == CONTROLMODE_PC)
		? (BUTTON_ACCEPT_USE | BUTTON_CANCEL_USE)
		: B_BUTTON;
	hostageindex = data->activehostage[playernum];

	if (contpad1 >= 0 && data->activedefusehostage[playernum] >= 0) {
		s32 defusehostage = data->activedefusehostage[playernum];
		s8 sticky1 = joyGetStickY(contpad1);
		s8 rsticky1 = joyGetRStickY(contpad1);
		s8 sticky2 = contpad2 >= 0 ? joyGetStickY(contpad2) : 0;
		u32 digitalmask = U_JPAD | D_JPAD | L_JPAD | R_JPAD | U_CBUTTONS | D_CBUTTONS | L_CBUTTONS | R_CBUTTONS;
		u32 pressed1 = joyGetButtonsPressedThisFrame(contpad1, digitalmask);
		u32 pressed2 = contpad2 >= 0 ? joyGetButtonsPressedThisFrame(contpad2, digitalmask) : 0;

		if (defusehostage < 0
				|| defusehostage >= ARRAYCOUNT(data->hostages)
				|| data->hostages[defusehostage] == NULL
				|| data->executiontimers240[defusehostage] <= 0) {
			rthExitDefuseMode(playernum);
			return;
		}

		if (g_Vars.currentplayer != NULL) {
			g_Vars.currentplayer->joybutinhibit = 0xffffffff;
			g_Vars.currentplayer->speedforwards = 0;
			g_Vars.currentplayer->speedsideways = 0;
			g_Vars.currentplayer->speedtheta = 0;
			g_Vars.currentplayer->speedstrafe = 0;
			g_Vars.currentplayer->speedgo = 0;
			g_Vars.currentplayer->speedthetacontrol = 0;
		}

		if (data->defusestickcooldown240[playernum] > 0) {
			data->defusestickcooldown240[playernum] -= g_Vars.lvupdate240;
		}

		if (data->defusestickcooldown240[playernum] <= 0) {
			if ((pressed1 & (U_JPAD | L_JPAD | U_CBUTTONS | L_CBUTTONS)) != 0
					|| (pressed2 & (U_JPAD | L_JPAD | U_CBUTTONS | L_CBUTTONS)) != 0
					|| sticky1 > 45
					|| rsticky1 > 45
					|| sticky2 > 45) {
				if (data->defuseselectedwire[playernum] == 0) {
					data->defuseselectedwire[playernum] = RTH_DEFUSE_WIRES - 1;
				} else {
					data->defuseselectedwire[playernum]--;
				}

				data->defusestickcooldown240[playernum] = RTH_DEFUSE_STICK_COOLDOWN240;
			} else if ((pressed1 & (D_JPAD | R_JPAD | D_CBUTTONS | R_CBUTTONS)) != 0
					|| (pressed2 & (D_JPAD | R_JPAD | D_CBUTTONS | R_CBUTTONS)) != 0
					|| sticky1 < -45
					|| rsticky1 < -45
					|| sticky2 < -45) {
				data->defuseselectedwire[playernum] = (data->defuseselectedwire[playernum] + 1) % RTH_DEFUSE_WIRES;
				data->defusestickcooldown240[playernum] = RTH_DEFUSE_STICK_COOLDOWN240;
			}
		}

		if (joyGetButtonsPressedThisFrame(contpad1, usepressmask) != 0) {
			if (data->defuseselectedwire[playernum] == data->defusetargetwire[playernum]) {
				rthStopExecutionBeep(defusehostage);
				snd00010718(NULL, 0, AL_VOL_FULL, AL_PAN_CENTER, SFX_RELOAD_DEFAULT, 1, 1, -1, 1);
				rthCompleteRescue(playernum, defusehostage);
			} else {
				rthExitDefuseMode(playernum);
				rthExecuteHostage(defusehostage);
			}

			return;
		}

		if (joyGetButtonsPressedThisFrame(contpad1, START_BUTTON | B_BUTTON) != 0
				|| (contpad2 >= 0 && joyGetButtonsPressedThisFrame(contpad2, START_BUTTON | B_BUTTON) != 0)) {
			rthExitDefuseMode(playernum);
		}

		return;
	}

	if (contpad1 >= 0 && joyGetButtonsPressedThisFrame(contpad1, usepressmask) != 0) {
		s32 defusehostage = -1;

		if (rthCanStartDefuse(playernum, &defusehostage)) {
			rthStopRescueSound(playernum);
			data->activehostage[playernum] = -1;
			data->rescuetime240[playernum] = 0;
			data->activedefusehostage[playernum] = defusehostage;
			data->defuseselectedwire[playernum] = 0;
			data->defusetargetwire[playernum] = rngRandom() % RTH_DEFUSE_WIRES;
			data->defusestickcooldown240[playernum] = RTH_DEFUSE_STICK_COOLDOWN240;
			return;
		}
	}

	if (hostageindex < 0 || hostageindex >= ARRAYCOUNT(data->hostages)) {
		if (contpad1 >= 0 && joyGetButtons(contpad1, usemask) != 0) {
			s32 i;

			for (i = 0; i < ARRAYCOUNT(data->hostages); i++) {
				struct prop *testprop = data->hostages[i];

				if (testprop != NULL) {
					if (data->deadhostagetimers240[i] > 0) {
						continue;
					}

					struct coord dist;
					f32 rangexz;
					f32 rangey;

					dist.x = testprop->pos.x - g_Vars.currentplayer->prop->pos.x;
					dist.y = testprop->pos.y - g_Vars.currentplayer->prop->pos.y;
					dist.z = testprop->pos.z - g_Vars.currentplayer->prop->pos.z;

					rangexz = sqrtf(dist.x * dist.x + dist.z * dist.z);
					rangey = ABS(dist.y);

					if (rangexz <= 140 && rangey <= 180) {
						data->activehostage[playernum] = i;
						data->rescuetime240[playernum] = 0;
						hostageindex = i;
						break;
					}
				}
			}
		}
	}

	if (hostageindex >= 0 && hostageindex < ARRAYCOUNT(data->hostages)) {
		struct prop *prop = data->hostages[hostageindex];

		if (prop == NULL) {
			rthStopRescueSound(playernum);
			data->activehostage[playernum] = -1;
			data->rescuetime240[playernum] = 0;
			return;
		}

		{
			struct coord dist;
			f32 rangexz;
			f32 rangey;

			dist.x = prop->pos.x - g_Vars.currentplayer->prop->pos.x;
			dist.y = prop->pos.y - g_Vars.currentplayer->prop->pos.y;
			dist.z = prop->pos.z - g_Vars.currentplayer->prop->pos.z;

			rangexz = sqrtf(dist.x * dist.x + dist.z * dist.z);
			rangey = ABS(dist.y);

			if (rangexz > 140 || rangey > 180) {
				rthStopRescueSound(playernum);
				return;
			}
		}

		if (contpad1 < 0 || joyGetButtons(contpad1, usemask) == 0) {
			rthStopRescueSound(playernum);
			return;
		}

		if (data->rescueaudiohandles[playernum] == NULL) {
			snd00010718(&data->rescueaudiohandles[playernum], 0, AL_VOL_FULL, AL_PAN_CENTER, SFX_003B, 1, 1, -1, 1);
		}

		if (data->rescueaudiohandles2[playernum] == NULL) {
			snd00010718(&data->rescueaudiohandles2[playernum], 0, AL_VOL_FULL, AL_PAN_CENTER, SFX_0038, 1, 1, -1, 1);
		}

		data->rescuetime240[playernum] += g_Vars.lvupdate240;

		if (data->rescuetime240[playernum] >= RTH_RESCUE_TIME240) {
			rthCompleteRescue(playernum, hostageindex);
		}
	}
}

void rthTick(void)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	s32 i;
	s32 j;

	for (i = 0; i < ARRAYCOUNT(data->hostagechrprops); i++) {
		if (data->hostagechrprops[i] != NULL && data->deadhostagetimers240[i] <= 0) {
			s32 oldtimer = data->executiontimers240[i];
			s32 oldsecond = oldtimer > 0 ? oldtimer / TICKS(240) : 0;
			data->executiontimers240[i] -= g_Vars.lvupdate240;
			s32 newtimer = data->executiontimers240[i];
			s32 newsecond = newtimer > 0 ? newtimer / TICKS(240) : 0;

			if (oldtimer > 0
					&& oldsecond != newsecond
					&& newsecond >= 0
					&& newsecond < 10) {
				rthStopExecutionBeep(i);

				if (newsecond > 0) {
					snd00010718(&data->executionbeepaudiohandles[i], 0, AL_VOL_FULL, AL_PAN_CENTER,
							newsecond <= 2 ? SFX_SLAYER_BEEP : SFX_EQUIP_HORIZONSCANNER,
							1, 1, -1, 1);
				}
			}

			if (data->executiontimers240[i] > TICKS(240) * 9) {
				rthStopExecutionBeep(i);
			}

			if (data->executiontimers240[i] <= 0) {
				rthExecuteHostage(i);
			}
		}

		if (data->vaporizewaittimers240[i] > 0) {
			data->vaporizewaittimers240[i] -= g_Vars.lvupdate240;

			if (data->vaporizewaittimers240[i] <= 0) {
				if (data->hostagechrprops[i] != NULL) {
					s32 splatindex;

					for (splatindex = 0; splatindex < 5; splatindex++) {
						splatsCreate(1, 1.1f, data->hostagechrprops[i], NULL, NULL, NULL, false, RTH_SPLATTYPE_PUDDLE, TICKS(150), NULL, rngRandom() & 8);
					}
				}

				rthStopExecutionBeep(i);
				rthDestroyHostageChr(i);
				rthDestroyHostageBomb(i);

				if (data->hostages[i] != NULL && data->hostages[i]->obj != NULL) {
					objFree(data->hostages[i]->obj, true, false);
					data->hostages[i] = NULL;
				}

				data->deadhostagetimers240[i] = TICKS(240);
			}
		}

		if (data->hostagechrprops[i] != NULL
				&& data->deadhostagetimers240[i] <= 0
				&& rthHostageChrIsDead(data->hostagechrprops[i])) {
			if (data->executiontimers240[i] > 0 && data->vaporizewaittimers240[i] <= 0) {
				data->hostageslost++;
				rthBroadcastHudmsg("Hostage killed.\n");

				for (j = 0; j < PLAYERCOUNT(); j++) {
					data->lostpenaltycount[j]++;
				}

				if (data->hostageslost >= RTH_HOSTAGE_LOSS_LIMIT && !data->matchending) {
					data->matchending = true;
					rthBroadcastHudmsg("Too many hostages lost.\n");
					g_NumReasonsToEndMpMatch++;
				}
			}

			data->executiontimers240[i] = 0;
			data->vaporizewaittimers240[i] = 0;
			data->deadhostagetimers240[i] = RTH_DEAD_RESPAWN_TIME240;
			rthStopExecutionBeep(i);

			for (j = 0; j < PLAYERCOUNT(); j++) {
				if (data->activehostage[j] == i) {
					rthStopRescueSound(j);
					data->activehostage[j] = -1;
					data->rescuetime240[j] = 0;
				}

				if (data->activedefusehostage[j] == i) {
					rthExitDefuseMode(j);
				}
			}

		}

		if (data->deadhostagetimers240[i] > 0) {
			data->deadhostagetimers240[i] -= g_Vars.lvupdate240;

			if (data->deadhostagetimers240[i] <= 0) {
				rthMoveHostageToPad(i, rthChooseRespawnPad(i));
			}
		}
	}

	for (i = 0; i < ARRAYCOUNT(data->escapinghostagechrprops); i++) {
		if (data->escapinghostagechrprops[i] != NULL) {
			if (data->escapinghostagestates[i] == 0
					&& data->escapinghostagechrprops[i]->chr
					&& chrIsStopped(data->escapinghostagechrprops[i]->chr)) {
				chrGoToRoomPos(
						data->escapinghostagechrprops[i]->chr,
						&data->escapinghostagedstpos[i],
						data->escapinghostagedstrooms[i],
						GOPOSFLAG_RUN);
				data->escapinghostagestates[i] = 1;
			}

			data->escapinghostagetimers240[i] -= g_Vars.lvupdate240;

			if (data->escapinghostagetimers240[i] <= 0) {
				rthDestroyEscapingHostageChr(i);
			}
		}
	}
}

Gfx *rthRenderHud(Gfx *gdl)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	s32 rescuetime = data->rescuetime240[g_Vars.currentplayernum];
	s32 hostageindex = data->activehostage[g_Vars.currentplayernum];
	s32 defusehostage = data->activedefusehostage[g_Vars.currentplayernum];

	if (defusehostage >= 0 && defusehostage < ARRAYCOUNT(data->hostages)) {
		s32 viewleft = viGetViewLeft();
		s32 viewtop = viGetViewTop();
		s32 viewwidth = viGetViewWidth();
		s32 viewheight = viGetViewHeight();
		s32 boxleft = viewleft + viewwidth / 5;
		s32 boxtop = viewtop + viewheight / 4;
		s32 boxright = viewleft + (viewwidth * 4) / 5;
		s32 boxbottom = boxtop + 88;
		s32 textx;
		s32 texty;
		s32 textwidth;
		s32 textheight;
		s32 selected = data->defuseselectedwire[g_Vars.currentplayernum] % RTH_DEFUSE_WIRES;
		s32 target = data->defusetargetwire[g_Vars.currentplayernum] % RTH_DEFUSE_WIRES;
		s32 i;
		char header[48];

		sprintf(header, "CUT THE %s WIRE", g_RthDefuseWireNames[target]);

		gdl = text0f153628(gdl);
		gdl = textSetPrimColour(gdl, 0x101820d0);
		gDPFillRectangle(gdl++, boxleft, boxtop, boxright, boxbottom);
		gdl = textSetPrimColour(gdl, 0x708090ff);
		gDPFillRectangle(gdl++, boxleft, boxtop, boxright, boxtop + 1);
		gDPFillRectangle(gdl++, boxleft, boxbottom - 1, boxright, boxbottom);
		gDPFillRectangle(gdl++, boxleft, boxtop, boxleft + 1, boxbottom);
		gDPFillRectangle(gdl++, boxright - 1, boxtop, boxright, boxbottom);

		textMeasure(&textheight, &textwidth, header, g_CharsHandelGothicXs, g_FontHandelGothicXs, 0);
		textx = viewleft + (viewwidth - textwidth) / 2;
		texty = boxtop + 10;
		gdl = textRender(gdl, &textx, &texty, header, g_CharsHandelGothicXs, g_FontHandelGothicXs, 0xffffffff, 0x000000a0, viGetWidth(), viGetHeight(), 0, 0);

		for (i = 0; i < RTH_DEFUSE_WIRES; i++) {
			s32 wireleft = boxleft + 24;
			s32 wireright = boxright - 24;
			s32 wiretop = boxtop + 34 + i * 14;

			gdl = text0f153838(gdl);
			gdl = textSetPrimColour(gdl, g_RthDefuseWireColours[i]);
			gDPFillRectangle(gdl++, wireleft, wiretop, wireright, wiretop + 4);

			if (i == selected) {
				gdl = textSetPrimColour(gdl, 0xffffffff);
				gDPFillRectangle(gdl++, wireleft - 6, wiretop - 2, wireleft - 2, wiretop + 6);
				gDPFillRectangle(gdl++, wireright + 2, wiretop - 2, wireright + 6, wiretop + 6);
			}
		}

		textx = boxleft + 20;
		texty = boxbottom - 18;
		gdl = textRender(gdl, &textx, &texty, "DIGITAL OR STICK SELECT  ACTION CUT  B/START CANCEL", g_CharsHandelGothicXs, g_FontHandelGothicXs, 0xc0d0e0c0, 0x00000080, viGetWidth(), viGetHeight(), 0, 0);
		gdl = text0f153780(gdl);
		return gdl;
	}

	if (rthCanStartDefuse(g_Vars.currentplayernum, NULL)) {
		s32 centrex = viGetViewLeft() + viGetViewWidth() / 2;
		s32 centrey = viGetViewTop() + viGetViewHeight() / 2;
		s32 textx = centrex - 50;
		s32 texty = centrey + 12;

		gdl = text0f153838(gdl);
		gdl = textSetPrimColour(gdl, 0xff4040d0);
		gDPFillRectangle(gdl++, centrex - 6, centrey - 1, centrex + 6, centrey + 1);
		gDPFillRectangle(gdl++, centrex - 1, centrey - 6, centrex + 1, centrey + 6);
		gdl = textRender(gdl, &textx, &texty, "PRESS ACTION TO DEFUSE", g_CharsHandelGothicXs, g_FontHandelGothicXs, 0xff8080c0, 0x00000080, viGetWidth(), viGetHeight(), 0, 0);
		gdl = text0f153780(gdl);
	}

	if (hostageindex >= 0 && hostageindex < ARRAYCOUNT(data->hostages)) {
		s32 viewleft = viGetViewLeft();
		s32 viewright = viGetViewLeft() + viGetViewWidth();
		s32 viewtop = viGetViewTop();
		s32 centre = (viewleft + viewright) / 2;
		s32 width = viGetViewWidth() / 3;
		s32 barleft = centre - width / 2;
		s32 barright = centre + width / 2;
		f32 progress = rescuetime / (f32)RTH_RESCUE_TIME240;
		s32 fillright;
		s32 cursor = barleft + 1;
		s32 chunkleft = barleft;

		if (progress < 0.0f) {
			progress = 0.0f;
		} else if (progress > 1.0f) {
			progress = 1.0f;
		}

		fillright = barleft + (s32)(width * progress);

		gdl = text0f153628(gdl);
		gdl = textSetPrimColour(gdl, 0x20306090);
		gDPFillRectangle(gdl++, barleft, viewtop + 8, barright, viewtop + 16);

		gdl = text0f153838(gdl);
		gdl = textSetPrimColour(gdl, 0x20d060d0);

		while (cursor < fillright) {
			gDPFillRectangle(gdl++, chunkleft, viewtop + 8, cursor, viewtop + 16);
			cursor += 2;
			chunkleft += 2;
		}

		gdl = text0f153838(gdl);
		gdl = text0f153780(gdl);
	}

	return gdl;
}

void rthCalculatePlayerScore(struct mpchrconfig *mpchr, s32 mpchrnum, s32 *score, s32 *deaths)
{
	s32 index = func0f18d0e8(mpchrnum);

	*score = 0;

	if (index >= 0 && index < PLAYERCOUNT()) {
		*score = g_ScenarioData.rth.rescuedcount[index] - g_ScenarioData.rth.lostpenaltycount[index];
	}

	*deaths = mpchr->numdeaths;
}

bool rthHighlightProp(struct prop *prop, s32 *colour)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	s32 i;
	s32 j;

	for (i = 0; i < ARRAYCOUNT(data->hostagebombprops); i++) {
		if (data->executiontimers240[i] <= 0 || data->executiontimers240[i] > TICKS(240) * 30) {
			continue;
		}

		for (j = 0; j < ARRAYCOUNT(data->hostagebombprops[i]); j++) {
			if (data->hostagebombprops[i][j] == prop) {
				if (data->executiontimers240[i] <= TICKS(240) * 10) {
					colour[0] = 0xff;
					colour[1] = 0x20;
					colour[2] = 0x20;
				} else if (data->executiontimers240[i] <= RTH_DEFUSE_WINDOW240) {
					colour[0] = 0xff;
					colour[1] = 0x90;
					colour[2] = 0x20;
				} else {
					colour[0] = 0xff;
					colour[1] = 0xd0;
					colour[2] = 0x20;
				}

				colour[3] = 96 + (menuGetSinOscFrac(12) * 159.0f);
				return true;
			}
		}
	}

	return false;
}

Gfx *rthRadarExtra(Gfx *gdl)
{
	struct scenariodata_rth *data = &g_ScenarioData.rth;
	s32 i;

	for (i = 0; i < ARRAYCOUNT(data->hostages); i++) {
		if (data->hostages[i]) {
			struct coord dist;
			u32 colour = 0x30ff70d0;

			dist.x = data->hostages[i]->pos.x - g_Vars.currentplayer->prop->pos.x;
			dist.y = data->hostages[i]->pos.y - g_Vars.currentplayer->prop->pos.y;
			dist.z = data->hostages[i]->pos.z - g_Vars.currentplayer->prop->pos.z;

			if (data->executiontimers240[i] > 0 && data->executiontimers240[i] <= TICKS(240) * 10) {
				colour = 0xff4040d0;
			} else if (data->executiontimers240[i] > 0 && data->executiontimers240[i] <= RTH_DEFUSE_WINDOW240) {
				colour = 0xffa020d0;
			} else if (data->executiontimers240[i] > 0 && data->executiontimers240[i] <= TICKS(240) * 30) {
				colour = 0xffd840d0;
			}

			gdl = radarDrawDot(gdl, data->hostages[i], &dist, colour, 0x00000000, true);
		}
	}

	return gdl;
}
