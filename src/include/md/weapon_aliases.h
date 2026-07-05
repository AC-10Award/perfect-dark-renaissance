#ifndef _IN_MD_WEAPON_ALIASES_H
#define _IN_MD_WEAPON_ALIASES_H

#include "constants.h"

/*
 * PERFECT DARK RENAISSANCE author-facing weapon aliases.
 *
 * These aliases are compatibility handles: gameplay internals still use the
 * legacy Perfect Dark weapon ids, but mod-authored lists should prefer the
 * MD_WEAPON_* names where an alias exists.
 */

#define MD_WEAPON_FALCON2             WEAPON_FALCON2
#define MD_WEAPON_FALCON2_SILENCER    WEAPON_FALCON2_SILENCER
#define MD_WEAPON_FALCON2_SCOPE       WEAPON_FALCON2_SCOPE
#define MD_WEAPON_MATRIX85            WEAPON_TESTER
#define MD_WEAPON_MAULER              WEAPON_MAULER
#define MD_WEAPON_PHOENIX             WEAPON_PHOENIX
#define MD_WEAPON_DY357_MAGNUM        WEAPON_DY357MAGNUM
#define MD_WEAPON_DY357_LX            WEAPON_DY357LX
#define MD_WEAPON_CMP150              WEAPON_CMP150
#define MD_WEAPON_CYCLONE             WEAPON_CYCLONE
#define MD_WEAPON_CALLISTO_NTG        WEAPON_CALLISTO
#define MD_WEAPON_RCP120              WEAPON_RCP120
#define MD_WEAPON_LAPTOP_GUN          WEAPON_LAPTOPGUN
#define MD_WEAPON_DRAGON              WEAPON_DRAGON
#define MD_WEAPON_K7_AVENGER          WEAPON_K7AVENGER
#define MD_WEAPON_AR34                WEAPON_AR34
#define MD_WEAPON_SUPERDRAGON         WEAPON_SUPERDRAGON
#define MD_WEAPON_DEVASTATOR          WEAPON_DEVASTATOR
#define MD_WEAPON_REAPER              WEAPON_REAPER
#define MD_WEAPON_SHOTGUN             WEAPON_SHOTGUN
#define MD_WEAPON_SNIPER_RIFLE        WEAPON_SNIPERRIFLE
#define MD_WEAPON_FARSIGHT_XR20       WEAPON_FARSIGHT

/*
 * Multiplayer weapon ids use a separate legacy namespace. Keep matching
 * MD_MPWEAPON_* aliases here so future MP setup ownership can migrate without
 * mixing MPWEAPON_* and WEAPON_* values accidentally.
 */

#define MD_MPWEAPON_FALCON2           MPWEAPON_FALCON2
#define MD_MPWEAPON_FALCON2_SILENCER  MPWEAPON_FALCON2_SILENCER
#define MD_MPWEAPON_FALCON2_SCOPE     MPWEAPON_FALCON2_SCOPE
#define MD_MPWEAPON_MATRIX85          MPWEAPON_MATRIX85
#define MD_MPWEAPON_MAULER            MPWEAPON_MAULER
#define MD_MPWEAPON_PHOENIX           MPWEAPON_PHOENIX
#define MD_MPWEAPON_DY357_MAGNUM      MPWEAPON_DY357MAGNUM
#define MD_MPWEAPON_DY357_LX          MPWEAPON_DY357LX
#define MD_MPWEAPON_CMP150            MPWEAPON_CMP150
#define MD_MPWEAPON_CYCLONE           MPWEAPON_CYCLONE
#define MD_MPWEAPON_CALLISTO_NTG      MPWEAPON_CALLISTO
#define MD_MPWEAPON_RCP120            MPWEAPON_RCP120
#define MD_MPWEAPON_LAPTOP_GUN        MPWEAPON_LAPTOPGUN
#define MD_MPWEAPON_DRAGON            MPWEAPON_DRAGON
#define MD_MPWEAPON_K7_AVENGER        MPWEAPON_K7AVENGER
#define MD_MPWEAPON_AR34              MPWEAPON_AR34
#define MD_MPWEAPON_SUPERDRAGON       MPWEAPON_SUPERDRAGON
#define MD_MPWEAPON_DEVASTATOR        MPWEAPON_DEVASTATOR
#define MD_MPWEAPON_REAPER            MPWEAPON_REAPER
#define MD_MPWEAPON_SHOTGUN           MPWEAPON_SHOTGUN
#define MD_MPWEAPON_SNIPER_RIFLE      MPWEAPON_SNIPERRIFLE
#define MD_MPWEAPON_FARSIGHT_XR20     MPWEAPON_FARSIGHT

/*
 * Multiplayer setup slots also contain non-weapon equipment and sentinels.
 * Keep these out of MD_MPWEAPON_* so setup tables communicate intent.
 */

#define MD_MPSLOT_DISABLED            MPWEAPON_DISABLED

#define MD_MPDEVICE_SHIELD            MPWEAPON_SHIELD
#define MD_MPDEVICE_CLOAKING          MPWEAPON_CLOAKINGDEVICE
#define MD_MPDEVICE_COMBAT_BOOST      MPWEAPON_COMBATBOOST

#endif
