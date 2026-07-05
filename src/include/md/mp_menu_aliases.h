#ifndef _IN_MD_MP_MENU_ALIASES_H
#define _IN_MD_MP_MENU_ALIASES_H

#include "constants.h"

/*
 * PERFECT DARK RENAISSANCE author-facing multiplayer menu aliases.
 *
 * Language-backed aliases point at entries already owned or safely reused by
 * PERFECT DARK RENAISSANCE. Literal aliases name custom UI strings that still need dedicated
 * language payload entries before they can be localized or fully ROM-independent.
 */

#define MD_MPMENU_TEXT_LAST_MAN_STANDING L_MPMENU_341
#define MD_MPMENU_TEXT_TERMINATION        ((uintptr_t) "Termination Mode")

#define MD_MPMENU_LITERAL_LAST_MAN_STANDING "Last Man Standing"
#define MD_MPMENU_LITERAL_SELECT_DIFFICULTY "Select Difficulty"
#define MD_MPMENU_LITERAL_DIFFICULTY_EASY   "Easy\n"
#define MD_MPMENU_LITERAL_DIFFICULTY_NORMAL "Normal\n"
#define MD_MPMENU_LITERAL_DIFFICULTY_HARD   "Hard\n"
#define MD_MPMENU_LITERAL_START_ARMED        "Start Armed"
#define MD_MPMENU_LITERAL_NO_DRUG_BLUR       "No Drug Blur"
#define MD_MPMENU_LITERAL_NO_PLAYER_ON_RADAR "No Player on Radar"
#define MD_MPMENU_LITERAL_NO_DOORS           "No Doors"
#define MD_MPMENU_LITERAL_DESTRUCTIBLE_DOORS "Destructible Doors"
#define MD_MPMENU_LITERAL_PICKUP_RESPAWN     "Pickup Respawn"
#define MD_MPMENU_LITERAL_PICKUP_RESPAWN_SFX "Pickup Respawn SFX"
#define MD_MPMENU_LITERAL_MORE_OPTIONS       "More Options\n"
#define MD_MPMENU_LITERAL_TERMINATION_BODY    "Evil Cyborg\n"

#endif
