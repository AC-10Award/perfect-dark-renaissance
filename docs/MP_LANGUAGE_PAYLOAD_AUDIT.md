# PERFECT DARK RENAISSANCE MP Language Payload Audit

Updated: 2026-05-02

## Purpose

This note records which multiplayer language strings currently live only in packed runtime payloads under `mods/*/files/L*`.

Do not broadly regenerate these payloads from base JSON until the custom strings are migrated into the matching JSON bank.

## Current Rule

PERFECT DARK RENAISSANCE's active migration target is English NTSC. PAL-English and Japanese payloads are legacy compatibility files unless a future task explicitly promotes them back into active ownership.

Safe:

```sh
tools/regenerate_weapon_lang
tools/regenerate_lang_asset gun
tools/regenerate_lang_asset mpmenu --lang en
```

Unsafe until migrated:

```sh
tools/regenerate_lang_asset mpweapons
tools/regenerate_lang_asset mpmenu
tools/regenerate_lang_asset mpmenu --lang gb
tools/regenerate_lang_asset mpmenu --lang jp
tools/regenerate_lang_asset --existing
```

The unsafe commands can replace mod-specific text that is not yet mirrored in JSON or can spend migration effort on compatibility languages that are not part of the current native-app target. `mpmenu --lang gb` was audited and migrated once for parity with the existing PAL-English runtime payload, but it should not be treated as an active content target going forward.

## Audit Summary

Compared packed runtime payloads against generated payloads from:

- `src/assets/ntsc-final/lang/mpmenu.json`
- `src/assets/ntsc-final/lang/mpweapons.json`

Live payloads checked:

- `mods/mod_allinone/files/LmpmenuE`
- `mods/mod_allinone/files/LmpmenuP`
- `mods/mod_allinone/files/LmpmenuJ`
- `mods/mod_allinone/files/LmpweaponsE`
- `mods/mod_allinone/files/LmpweaponsP`
- `mods/mod_allinone/files/LmpweaponsJ`
- `mods/mod_gex/files/LmpweaponsE`
- `mods/mod_gex/files/LmpweaponsP`
- `mods/mod_gex/files/LmpweaponsJ`

## `mods/mod_allinone/files/LmpmenuE`

Status: migrated to `src/assets/ntsc-final/lang/mpmenu.json`.

`L_MPMENU_294` through `L_MPMENU_338` intentionally repurpose the unused PerfectHead/Game Boy Camera text range for PERFECT DARK RENAISSANCE MP category and level names.

Code that refers to this range should use `MD_MPSTAGE_TEXT_*` aliases from `src/include/md/mp_stage_aliases.h` instead of raw `L_MPMENU_*` ids. The language JSON remains the text source; the alias layer records PERFECT DARK RENAISSANCE ownership in touched menu/setup code.

The normal MP arena list and the exact custom labels in the Last Man Standing / Termination curated stage list now resolve through these aliases. The curated list still keeps literal fallbacks for vanilla Perfect Dark labels and duplicate-disambiguation labels such as `Temple GE64`, because those do not yet have dedicated owned language entries.

Mode menu labels that already have stable language entries should use `MD_MPMENU_TEXT_*` aliases from `src/include/md/mp_menu_aliases.h`. As of this pass, `Last Man Standing` and `Termination` are language-backed. The Last Man Standing difficulty dialog and extended game option labels are named as `MD_MPMENU_LITERAL_*` aliases in the same header, but still need dedicated payload entries before they are fully migrated.

Current literal backlog:

| Alias | Text | Current use |
| --- | --- | --- |
| `MD_MPMENU_LITERAL_LAST_MAN_STANDING` | `Last Man Standing` | Literal-only LMS endscreen dialog title |
| `MD_MPMENU_LITERAL_SELECT_DIFFICULTY` | `Select Difficulty` | Literal-only LMS difficulty dialog title |
| `MD_MPMENU_LITERAL_DIFFICULTY_EASY` | `Easy\n` | Literal-only LMS difficulty option |
| `MD_MPMENU_LITERAL_DIFFICULTY_NORMAL` | `Normal\n` | Literal-only LMS difficulty option |
| `MD_MPMENU_LITERAL_DIFFICULTY_HARD` | `Hard\n` | Literal-only LMS difficulty option |
| `MD_MPMENU_LITERAL_START_ARMED` | `Start Armed` | More Options checkbox |
| `MD_MPMENU_LITERAL_NO_DRUG_BLUR` | `No Drug Blur` | More Options checkbox |
| `MD_MPMENU_LITERAL_NO_PLAYER_ON_RADAR` | `No Player on Radar` | More Options checkbox |
| `MD_MPMENU_LITERAL_NO_DOORS` | `No Doors` | More Options checkbox |
| `MD_MPMENU_LITERAL_DESTRUCTIBLE_DOORS` | `Destructible Doors` | More Options checkbox |
| `MD_MPMENU_LITERAL_PICKUP_RESPAWN` | `Pickup Respawn` | More Options dropdown |
| `MD_MPMENU_LITERAL_PICKUP_RESPAWN_SFX` | `Pickup Respawn SFX` | More Options checkbox |
| `MD_MPMENU_LITERAL_MORE_OPTIONS` | `More Options\n` | Extended game options dialog title |
| `MD_MPMENU_LITERAL_TERMINATION_BODY` | `Termination\n` | MP body display label |

Curated LMS result note: manual in-game abort/exit is treated as a loss before ranking-based win inference. This prevents the result screen from showing the win card when a player quits before clearing the match.

Important custom range:

| Entry | Packed Runtime Text | Base JSON Text |
| --- | --- | --- |
| 294 | `Random Multi\n` | `PerfectHead\n` |
| 295 | `Random Solo\n` | `Load A Saved Head\n` |
| 296 | `GoldenEye X\n` | `Make A New Head\n` |
| 297 | `GoldenEye X Bonus\n` | `Edit A Loaded Head\n` |
| 298 | `Frigate` | `Edit A PerfectHead\n` |
| 299 | `Archives` | `Choose slot to edit:\n` |
| 300 | `Bunker` | `Cancel\n` |
| 301 | `Labyrinth` | `Make A New Head\n` |
| 302 | `Basement` | `Load A Saved Head\n` |
| 303 | `Library` | `Where do you want to be able\n` |
| 304 | `Cradle` | `to use this head?\n` |
| 305 | `Caverns` | `BATTLE MODES\n` |
| 306 | `Caves` | `All Missions\n` |
| 307 | `Facility BZ` | `Both\n` |
| 308 | `Citadel` | `Cancel\n` |
| 309 | `Stack` | `Camera Setup\n` |
| 310 | `Train` | `Insert your camera into your\n` |
| 311 | `Facility` | `Game Boy Pak.\n` |
| 312 | `Egyptian` | `Then, insert your Game Boy Pak\n` |
| 313 | `Aztec` | `into any controller.\n` |
| 314 | `Archives 1F\n` | `OK\n` |
| 315 | `Streets\n` | `Cancel\n` |
| 316 | `Icicle Pyramid` | `Error\n` |
| 317 | `Random GoldenEye X\n` | `Game Boy Camera Not Detected!\n` |
| 318 | `The Legend of Zelda\n` | `Please check all connections\n` |
| 319 | `Kakariko Village\n` | `and try again.\n` |
| 320 | `Dark Noon\n` | `Cancel\n` |
| 321 | `Valley\n` | `Choose Camera\n` |
| 322 | `Archives BZ\n` | `More than one camera detected.\n` |
| 323 | `Cliff Base\n` | `Please select which camera you\nwant to use\n` |
| 324 | `Suburb\n` | `OK\n` |
| 325 | `Training Day\n` | `Cancel\n` |
| 326 | `Bonus\n` | `PerfectHead\n` |
| 327 | `Runway\n` | `Take A Picture Now\n` |
| 328 | `Control\n` | `Load Picture From Camera\n` |
| 329 | `Tawfret Ruins\n` | `Cancel\n` |
| 330 | `Targitzan's Temple\n` | `Load From Camera\n` |
| 331 | `Junkyard\n` | `Choose picture to read from camera:\n` |
| 332 | `Steel Mill\n` | `Cancel\n` |
| 333 | `Mall\n` | `Take Picture\n` |
| 334 | `Tunnels\n` | `Shoot!\n` |
| 335 | `Rogue\n` | `Cancel\n` |
| 336 | `Paradox\n` | `Keep Picture\n` |
| 337 | `War Colors\n` | `Try Again\n` |
| 338 | `Grand Library\n` | `Load PerfectHead\n` |

Other English differences:

| Entry | Packed Runtime Text | Base JSON Text |
| --- | --- | --- |
| 68 | `Select Tune\n` | `Select Tunes\n` |
| 69 | `Select Tunes\n` | `Select Tune\n` |
| 129 | `Stack` | `Rooftop` |

## `mods/mod_allinone/files/LmpmenuP`

Status: migrated to `src/assets/ntsc-final/lang/mpmenu.json` for compatibility parity, but not an active future content target.

The PAL-English payload has the same custom MP range as `LmpmenuE`.

Additional PAL-English differences:

| Entry | Packed Runtime Text | Base JSON Text |
| --- | --- | --- |
| 132 | `Car Park` | `Car Parking Lot` |
| 145 | `Stats for %s\n` | `Statistics\n` |
| 156 | `Time:\n` | `Time Spent:\n` |
| 184 | `Do you want to save\nover your original\ngame file?\n` | `Do you want to save over\nyour original game file?\n` |
| 389 | `to the Controller Pak?\n` | `to the controller pak?\n` |
| 394 | `a Controller Pak if you wish.\n` | `a controller pak if you wish.\n` |
| 396 | `ensure a Controller Pak is inserted\n` | `ensure a controller pak is inserted\n` |
| 409 | `Head Colour\n` | `Head Color\n` |
| 491 | `Checking Controller Pak\n` | `Initializing Controller Pak\n` |

## `mods/mod_allinone/files/LmpmenuJ`

Status: legacy compatibility; not migrated.

The Japanese payload differs heavily from generated `ntsc-final` JSON output. Do not regenerate it from `src/assets/ntsc-final/lang/mpmenu.json`. If PERFECT DARK RENAISSANCE later supports Japanese localization, rebuild it from native-owned English source content and a deliberate localization pass instead of preserving inherited ROM-era text.

## `mpweapons`

`mods/mod_allinone/files/LmpweaponsE` matches generated JSON and already uses the intended PERFECT DARK RENAISSANCE wording, `Cheat BATTLE MODES\n`.

These compatibility payloads still differ at one known entry:

| File | Entry | Packed Runtime Text | Base JSON Text |
| --- | --- | --- | --- |
| `mods/mod_allinone/files/LmpweaponsP` | 131 | `Cheat Combat Simulator\n` | `Cheat BATTLE MODES\n` |
| `mods/mod_allinone/files/LmpweaponsJ` | 131 | `Cheat Combat Simulator\n` | `Cheat BATTLE MODES\n` |
| `mods/mod_gex/files/LmpweaponsE` | 131 | `Cheat Combat Simulator\n` | `Cheat BATTLE MODES\n` |
| `mods/mod_gex/files/LmpweaponsP` | 131 | `Cheat Combat Simulator\n` | `Cheat BATTLE MODES\n` |
| `mods/mod_gex/files/LmpweaponsJ` | 131 | `Cheat Combat Simulator\n` | `Cheat BATTLE MODES\n` |

## Migration Recommendation

1. Treat English NTSC JSON as the source of truth for active mod text.
2. Keep `L_MPWEAPONS_131` as `Cheat BATTLE MODES\n`; this is an intentional PERFECT DARK RENAISSANCE rename, not a regression.
3. Regenerate only the audited English bank after each JSON migration.
4. Leave PAL/Japanese payloads alone unless a future compatibility task explicitly needs them.
