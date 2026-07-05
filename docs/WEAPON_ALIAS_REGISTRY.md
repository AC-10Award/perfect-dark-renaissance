# PERFECT DARK RENAISSANCE Weapon Alias Registry

Updated: 2026-05-02

## Purpose

This document is the author-facing reference for weapon naming in PERFECT DARK RENAISSANCE.

Use it when:

- renaming weapons
- reordering weapon lists
- reviewing weapon-related docs
- translating between legacy engine ids and PERFECT DARK RENAISSANCE names

This is a migration aid. The engine still uses legacy `WEAPON_*` constants internally, but author-facing work should prefer the PERFECT DARK RENAISSANCE name first and translate to the engine id second.

The first code-side alias layers now live in:

- `src/include/md/weapon_aliases.h`
- `src/include/md/weapon_display.h`

Use:

- `MD_WEAPON_*` for legacy `WEAPON_*` values
- `MD_MPWEAPON_*` for legacy `MPWEAPON_*` values
- `MD_MPDEVICE_*` for non-weapon MP equipment values
- `MD_MPSLOT_*` for MP setup sentinel values
- `MD_WEAPON_TEXT_*` for legacy `L_GUN_*` weapon display-name values

Keep these namespaces separate. `WEAPON_*` and `MPWEAPON_*` values are not interchangeable.

## Current Live Anchors

- Display-name mapping: `src/game/propobj.c`
- Auto-switch weapon order: `src/game/bondgun.c`
- Localized weapon strings: `src/assets/*/lang/gun.json`
- Code-side author aliases: `src/include/md/weapon_aliases.h`
- Code-side display-name aliases: `src/include/md/weapon_display.h`

## Working Rule

When discussing weapon changes, use this form:

`PERFECT DARK RENAISSANCE name -> legacy engine id`

Example:

`Matrix-85 -> WEAPON_MAGSEC4`

For reorder work, say:

`Move Matrix-85 above Phoenix in auto-switch order.`

Then implement that against the legacy ids:

`Move WEAPON_MAGSEC4 above WEAPON_PHOENIX in g_AutoSwitchWeaponsPrimary.`

## Active Registry

| Legacy Engine ID | PERFECT DARK RENAISSANCE Name | Status | Notes |
| --- | --- | --- | --- |
| `WEAPON_FALCON2` | `Falcon 2` | live | Legacy name still used as the live display name. |
| `WEAPON_FALCON2_SILENCER` | `Falcon 2 (silenced)` | live | Display name already localized through language assets. |
| `WEAPON_FALCON2_SCOPE` | `Falcon 2 (scope)` | live | Display name already localized through language assets. |
| `WEAPON_MAGSEC4` | `Matrix-85` | live | First explicit alias target for native-owned weapon naming. |
| `WEAPON_MAULER` | `Mauler` | live | Legacy name still used as the live display name. |
| `WEAPON_PHOENIX` | `Phoenix` | live | Legacy name still used as the live display name. |
| `WEAPON_DY357MAGNUM` | `DY357 Magnum` | live | Candidate for later PERFECT DARK RENAISSANCE rename if desired. |
| `WEAPON_DY357LX` | `DY357-LX` | live | Candidate for later PERFECT DARK RENAISSANCE rename if desired. |
| `WEAPON_CMP150` | `CMP150` | live | Candidate for later PERFECT DARK RENAISSANCE rename if desired. |
| `WEAPON_CYCLONE` | `Cyclone` | live | Candidate for later PERFECT DARK RENAISSANCE rename if desired. |
| `WEAPON_CALLISTO` | `Callisto NTG` | live | Candidate for later PERFECT DARK RENAISSANCE rename if desired. |
| `WEAPON_RCP120` | `RCP120` | live | Candidate for later PERFECT DARK RENAISSANCE rename if desired. |
| `WEAPON_LAPTOPGUN` | `Laptop Gun` | live | Candidate for later PERFECT DARK RENAISSANCE rename if desired. |
| `WEAPON_DRAGON` | `Dragon` | live | Candidate for later PERFECT DARK RENAISSANCE rename if desired. |
| `WEAPON_K7AVENGER` | `K7 Avenger` | live | Candidate for later PERFECT DARK RENAISSANCE rename if desired. |
| `WEAPON_AR34` | `AR34` | live | Candidate for later PERFECT DARK RENAISSANCE rename if desired. |
| `WEAPON_SUPERDRAGON` | `SuperDragon` | live | Candidate for later PERFECT DARK RENAISSANCE rename if desired. |
| `WEAPON_DEVASTATOR` | `Devastator` | live | Candidate for later PERFECT DARK RENAISSANCE rename if desired. |
| `WEAPON_REAPER` | `Reaper` | live | Candidate for later PERFECT DARK RENAISSANCE rename if desired. |
| `WEAPON_SHOTGUN` | `Shotgun` | live | Candidate for later PERFECT DARK RENAISSANCE rename if desired. |
| `WEAPON_SNIPERRIFLE` | `Sniper Rifle` | live | Candidate for later PERFECT DARK RENAISSANCE rename if desired. |
| `WEAPON_FARSIGHT` | `FarSight XR-20` | live | Candidate for later PERFECT DARK RENAISSANCE rename if desired. |

## MP Setup Non-Weapon Classification

These aliases exist because multiplayer setup slots can contain equipment or sentinel values, not only weapons.

| Legacy MP ID | PERFECT DARK RENAISSANCE Alias | Category | Notes |
| --- | --- | --- | --- |
| `MPWEAPON_DISABLED` | `MD_MPSLOT_DISABLED` | slot sentinel | Empty/disabled setup slot, not a weapon. |
| `MPWEAPON_SHIELD` | `MD_MPDEVICE_SHIELD` | device/equipment | Kept out of `MD_MPWEAPON_*`. |
| `MPWEAPON_CLOAKINGDEVICE` | `MD_MPDEVICE_CLOAKING` | device/equipment | Kept out of `MD_MPWEAPON_*`. |
| `MPWEAPON_COMBATBOOST` | `MD_MPDEVICE_COMBAT_BOOST` | device/equipment | Kept out of `MD_MPWEAPON_*`. |

Still intentionally unresolved:

- `MPWEAPON_COMBATKNIFE`
- `MPWEAPON_CROSSBOW`
- `MPWEAPON_TRANQUILIZER`
- `MPWEAPON_GRENADE`
- `MPWEAPON_NBOMB`
- `MPWEAPON_TIMEDMINE`
- `MPWEAPON_PROXIMITYMINE`
- `MPWEAPON_REMOTEMINE`
- `MPWEAPON_ROCKETLAUNCHER`
- `MPWEAPON_SLAYER`

These may become `MD_MPWEAPON_*`, `MD_MPEXPLOSIVE_*`, or another category after their PERFECT DARK RENAISSANCE naming/role is decided.

## Display-Name Alias Layer

`src/include/md/weapon_display.h` maps live PERFECT DARK RENAISSANCE weapon display-name concepts to legacy `L_GUN_*` text ids.

This is the current bridge layer:

- `src/assets/*/lang/gun.json` owns the author-side localized strings
- the live runtime strings are the packed external language files in `mods/*/files/LgunE`, `mods/*/files/LgunP`, and `mods/*/files/LgunJ`
- `src/game/propobj.c` uses `MD_WEAPON_TEXT_*` for registry-covered pickup/object weapon names
- `src/game/invitems.c` uses `MD_WEAPON_TEXT_*` for registry-covered inventory short/name ids
- future weapon renames should update or add the `MD_WEAPON_TEXT_*` alias before changing scattered runtime lookup code
- unresolved/projectile/explosive entries remain direct `L_GUN_*` references until their PERFECT DARK RENAISSANCE category is decided

Important runtime finding:

- CMake currently invokes `tools/assetmgr/mklang` with `--headers-only`, so editing `src/assets/*/lang/gun.json` updates generated text-id headers but does not automatically rewrite the live `mods/*/files/Lgun*` payloads.
- `port/src/romdata.c` loads `files/LgunE` from the active `--moddir` before falling back to the ROM, so a stale external `LgunE` can override the edited JSON and still show old names.
- `WEAPON_MAGSEC4 -> Matrix-85` exposed this: `gun.json` contained `Matrix-85`, but `mods/mod_allinone/files/LgunE` still decoded to `MagSec 4`.
- Regenerate the packed language files after weapon-name JSON edits.

Current regeneration command:

```sh
tools/regenerate_weapon_lang
```

By default this updates `mods/mod_allinone/files/Lgun*` and `mods/mod_gex/files/Lgun*` from `src/assets/ntsc-final/lang/gun.json`.

General language payload regeneration is also available:

```sh
tools/regenerate_lang_asset gun mpweapons mpmenu options
```

Use explicit bank names and only audited languages. For `mpmenu`, English and PAL-English are now JSON-owned and can be regenerated with:

```sh
tools/regenerate_lang_asset mpmenu --lang en --lang gb
```

Do not regenerate `mpmenu` without the language filter until `LmpmenuJ` ownership is resolved.

## Next Migration Use

When a future session says "continue the weapon-name migration," use this document first, then audit:

- `src/game/propobj.c`
- `src/game/invitems.c`
- `src/assets/*/lang/gun.json`
- training and challenge descriptions
- `src/game/bondgun.c`
- multiplayer weapon set definitions
- `src/include/md/weapon_display.h`

Current first code migration:

- `src/game/bondgun.c` includes `src/include/md/weapon_aliases.h`
- `g_AutoSwitchWeaponsPrimary` and `g_AutoSwitchWeaponsSecondary` use `MD_WEAPON_*` aliases for weapons covered by this registry
- `src/game/mpconfigs.c` includes `src/include/md/weapon_aliases.h`
- `g_MpConfigs` uses `MD_MPWEAPON_*` aliases for multiplayer weapon ids covered by this registry
- `g_MpConfigs` uses `MD_MPDEVICE_*` and `MD_MPSLOT_*` aliases for classified non-weapon entries
- `src/game/propobj.c` includes `src/include/md/weapon_display.h`
- `var8006aa94pf` uses `MD_WEAPON_TEXT_*` aliases for pickup/object names covered by this registry
- `src/game/invitems.c` includes `src/include/md/weapon_display.h`
- registry-covered inventory short/name ids use `MD_WEAPON_TEXT_*` aliases
- Matrix-85 now resolves through `MD_WEAPON_TEXT_MATRIX85 -> L_GUN_010` and the regenerated packed `mods/*/files/Lgun*` runtime assets
- unresolved weapon/explosive/projectile entries remain legacy constants until their category and PERFECT DARK RENAISSANCE names are decided

The next practical upgrade after this document is to decide which unresolved weapon, explosive, projectile, or GoldenEye-only entries deserve PERFECT DARK RENAISSANCE registry entries.
