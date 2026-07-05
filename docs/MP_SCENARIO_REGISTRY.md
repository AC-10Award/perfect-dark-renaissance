# PERFECT DARK RENAISSANCE MP Scenario Registry

Updated: 2026-05-02

## Purpose

This document is the author-facing bridge between PERFECT DARK RENAISSANCE scenario names and the inherited Perfect Dark multiplayer scenario ids.

The engine still stores legacy `MPSCENARIO_*` values. New mod-facing setup/menu work should prefer the `MD_MPSCENARIO_*` aliases in `src/include/md/scenario_aliases.h` so future scenario edits start from PERFECT DARK RENAISSANCE intent instead of raw legacy ids.

## Current Aliases

| PERFECT DARK RENAISSANCE Alias | Legacy Id | Current Display Name | Notes |
| --- | --- | --- | --- |
| `MD_MPSCENARIO_COMBAT` | `MPSCENARIO_COMBAT` | Combat | Standard free-for-all/team combat scenario |
| `MD_MPSCENARIO_HOLD_THE_BRIEFCASE` | `MPSCENARIO_HOLDTHEBRIEFCASE` | Hold the Briefcase | Uses briefcase objective callbacks |
| `MD_MPSCENARIO_HACKER_CENTRAL` | `MPSCENARIO_HACKERCENTRAL` | Hacker Central | Legacy code still calls some callbacks `htm` / Hack That Mac |
| `MD_MPSCENARIO_POP_A_CAP` | `MPSCENARIO_POPACAP` | Pop a Cap | Target assassination scenario |
| `MD_MPSCENARIO_KING_OF_THE_HILL` | `MPSCENARIO_KINGOFTHEHILL` | King of the Hill | Team-only in the scenario menu |
| `MD_MPSCENARIO_CAPTURE_THE_CASE` | `MPSCENARIO_CAPTURETHECASE` | Capture the Case | Team-only in the scenario menu |
| `MD_MPSCENARIO_RESCUE_HOSTAGES` | `MPSCENARIO_RESCUEHOSTAGES` | Rescue the Hostages (Test) | Custom/test PvE-style mode |
| `MD_MPSCENARIO_LAST_MAN_STANDING` | `MPSCENARIO_LASTMANSTANDING` | Last Man Standing | Custom PvE group entry |
| `MD_MPSCENARIO_TERMINATION` | `MPSCENARIO_TERMINATION` | Termination | Custom PvE group entry |

## Current Source Of Truth

- Scenario ids: `src/include/constants.h`
- PERFECT DARK RENAISSANCE aliases: `src/include/md/scenario_aliases.h`
- Scenario callback table: `src/game/mplayer/scenarios.c`
- Scenario menu overview table: `src/game/mplayer/scenarios.c`
- Scenario behavior implementations: `src/game/mplayer/scenarios/*.inc`
- Setup defaults and special-case limits: `src/game/mplayer/mplayer.c`
- Setup menu wiring: `src/game/mplayer/setup.c`
- Menu language source: `src/assets/ntsc-final/lang/mpmenu.json`

## Scenario Text Ownership

English NTSC scenario menu text is recorded in `src/assets/ntsc-final/lang/mpmenu.json`. The matching `gb` fields are mirrored for compatibility because some menu/runtime paths can still expose PAL-English payload text.

Runtime display for the three custom scenario names now uses the owned `L_MPMENU_339` through `L_MPMENU_344` language ids via `MD_MPSCENARIO_TEXT_*` aliases.

In testing, those reused PerfectHead-era ids previously resolved to stale text such as `wish to load into any` or `Controller Pak:` in live match HUD/menu paths. The root cause was the mod overlay order. MP setup loaded `mods/mod_allinone/files/LmpmenuE` while `g_ModNum == MOD_NORMAL`, but GEX/extra stages switch `g_ModNum` to `MOD_GEX`; `mods/mod_gex/files` did not provide `LmpmenuE`, so the loader fell back to base ROM/data instead of shared `mod_allinone` data. `port/src/fs.c` now checks the active specialized mod directory first, then shared `mods/mod_allinone`, then base data.

| Text Alias | Language Id | English Text |
| --- | --- | --- |
| `MD_MPSCENARIO_TEXT_RESCUE_HOSTAGES_NAME` | `L_MPMENU_339` | `Rescue the Hostages (Test)` |
| `MD_MPSCENARIO_TEXT_RESCUE_HOSTAGES_SHORT` | `L_MPMENU_340` | `Rescue (Test)` |
| `MD_MPSCENARIO_TEXT_LAST_MAN_STANDING_NAME` | `L_MPMENU_341` | `Last Man Standing` |
| `MD_MPSCENARIO_TEXT_LAST_MAN_STANDING_SHORT` | `L_MPMENU_342` | `Last Stand` |
| `MD_MPSCENARIO_TEXT_TERMINATION_NAME` | `L_MPMENU_343` | `Termination` |
| `MD_MPSCENARIO_TEXT_TERMINATION_SHORT` | `L_MPMENU_343` | `Termination` |
| `MD_MPSCENARIO_GROUP_PVE_MODES` | `L_MPMENU_344` | `-PvE Modes-\n` |

## Migration Rule

Keep legacy `MPSCENARIO_*` ids in low-level engine internals for now. Use `MD_MPSCENARIO_*` aliases in new or touched PERFECT DARK RENAISSANCE-authored menu/setup code, especially where the code is registering, naming, grouping, or selecting scenarios.

Do not renumber scenario ids yet. Save files, setup serialization, menu indexing, bot logic, and scenario callback arrays all assume stable numeric ids.

## Next Ownership Steps

1. Replace touched setup/menu scenario references with `MD_MPSCENARIO_*` aliases where the meaning is author-facing.
2. Add a scenario metadata table only after we know which fields we actually need, such as group, display name, short name, default stage, max teams, and test status.
3. Leave deep behavior checks in bot/combat code alone until a scenario-specific refactor needs them.
