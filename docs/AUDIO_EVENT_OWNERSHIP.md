# PERFECT DARK RENAISSANCE Audio Event Ownership

Updated: 2026-05-03

## Purpose

This registry documents the first native-owned audio trigger names for PERFECT DARK RENAISSANCE.

The runtime can still fall back to ROM-backed audio, but mod-authored code should refer to `MD_*` audio aliases first instead of scattering external filenames or trigger strings through gameplay code.

## Source Of Truth

- Code aliases: `src/include/md/audio_aliases.h`
- External music folder: `ext_music`
- External SFX folder: `ext_sfx`
- External music filename rules: `ext_music/NAMING_RULES.txt`
- External SFX event registry: `docs/references/sfx_event_registry.csv`
- External SFX audit: `tools/audit_ext_sfx.py`

## Music Keys

Music aliases are extensionless lookup keys. A key can resolve to a direct `.mp3`, a numbered pool, or a descriptive variant according to the existing `ext_music` resolver.

| Alias | Key | Current trigger |
| --- | --- | --- |
| `MD_MUSIC_KEY_OPENING_COMPANY_LOGOS` | `opening_company_logos` | Title startup logos |
| `MD_MUSIC_KEY_MAIN_TITLE_LOGO` | `main_title_logo` | Main title logo |
| `MD_MUSIC_KEY_MAIN_MENU` | `main_menu` | Frontend menu music |
| `MD_MUSIC_KEY_COMBAT_SIM_MENU` | `combat_sim_menu` | Battle Modes menu music |
| `MD_MUSIC_KEY_TERMINATION_MENU` | `termination_menu` | Termination level-select menu music |
| `MD_MUSIC_KEY_COMBAT_SIM_COMPLETE` | `combat_sim_complete` | Battle Modes completion/result music |
| `MD_MUSIC_KEY_MULTIPLAYER_DEATH` | `multiplayer_death` | MP death overlay |
| `MD_MUSIC_KEY_LMS_WIN` | `last_man_standing_win` | LMS win result |
| `MD_MUSIC_KEY_LMS_LOSE` | `last_man_standing_lose` | LMS loss result |
| `MD_MUSIC_KEY_TERMINATION_WIN` | `termination_win` | Termination win result |
| `MD_MUSIC_KEY_TERMINATION_LOSE` | `termination_lose` | Termination loss result |

## MP Track Bridge Keys

These aliases still point to inherited `mp_track_*` trigger keys. They do not mean the final PERFECT DARK RENAISSANCE soundtrack must preserve the original Perfect Dark track identity. For now, they make the bridge explicit and keep future renaming centralized.

Termination matches override the user's MP soundtrack selection and randomize only across the current Termination-curated pool in `src/game/mplayer/mplayer.c`: `MUSIC_G5_X`, `MUSIC_G5`, `MUSIC_CHICAGO_X`, `MUSIC_EXTRACTION_X`, `MUSIC_SKEDAR_MYSTERY`, `MUSIC_CI_OPERATIVE`, and `MUSIC_DARK_COMBAT`.

| Alias | Key | Legacy track bridge |
| --- | --- | --- |
| `MD_MUSIC_KEY_MP_DARK_COMBAT` | `mp_track_dark_combat` | `MUSIC_DARK_COMBAT` |
| `MD_MUSIC_KEY_MP_SKEDAR_MYSTERY` | `mp_track_skedar_mystery` | `MUSIC_SKEDAR_MYSTERY` |
| `MD_MUSIC_KEY_MP_CI_OPERATIVE` | `mp_track_ci_operative` | `MUSIC_CI_OPERATIVE` |
| `MD_MUSIC_KEY_MP_DATADYNE_ACTION` | `mp_track_datadyne_action` | `MUSIC_DATADYNE_ACTION` |
| `MD_MUSIC_KEY_MP_MAIAN_TEARS` | `mp_track_maian_tears` | `MUSIC_MAIAN_TEARS` |
| `MD_MUSIC_KEY_MP_ALIEN_CONFLICT` | `mp_track_alien_conflict` | `MUSIC_ALIEN_CONFLICT` |
| `MD_MUSIC_KEY_MP_CI` | `mp_track_carrington_institute` | `MUSIC_CI` |
| `MD_MUSIC_KEY_MP_DD_CENTRAL` | `mp_track_dd_central` | `MUSIC_DEFECTION` |
| `MD_MUSIC_KEY_MP_DD_CENTRAL_X` | `mp_track_dd_central_x` | `MUSIC_DEFECTION_X` |
| `MD_MUSIC_KEY_MP_DD_RESEARCH` | `mp_track_dd_research` | `MUSIC_INVESTIGATION` |
| `MD_MUSIC_KEY_MP_DD_RESEARCH_X` | `mp_track_dd_research_x` | `MUSIC_INVESTIGATION_X` |
| `MD_MUSIC_KEY_MP_DD_EXTRACTION` | `mp_track_dd_extraction` | `MUSIC_EXTRACTION` |
| `MD_MUSIC_KEY_MP_DD_EXTRACTION_X` | `mp_track_dd_extraction_x` | `MUSIC_EXTRACTION_X` |
| `MD_MUSIC_KEY_MP_VILLA` | `mp_track_carrington_villa` | `MUSIC_VILLA` |
| `MD_MUSIC_KEY_MP_VILLA_X` | `mp_track_carrington_villa_x` | `MUSIC_VILLA_X` |
| `MD_MUSIC_KEY_MP_CHICAGO` | `mp_track_chicago` | `MUSIC_CHICAGO` |
| `MD_MUSIC_KEY_MP_CHICAGO_X` | `mp_track_chicago_x` | `MUSIC_CHICAGO_X` |
| `MD_MUSIC_KEY_MP_G5` | `mp_track_g5_building` | `MUSIC_G5` |
| `MD_MUSIC_KEY_MP_G5_X` | `mp_track_g5_building_x` | `MUSIC_G5_X` |
| `MD_MUSIC_KEY_MP_A51_INFILTRATION` | `mp_track_a51_infiltration` | `MUSIC_INFILTRATION` |
| `MD_MUSIC_KEY_MP_A51_INFILTRATION_X` | `mp_track_a51_infiltration_x` | `MUSIC_INFILTRATION_X` |
| `MD_MUSIC_KEY_MP_A51_RESCUE` | `mp_track_a51_rescue` | `MUSIC_RESCUE` |
| `MD_MUSIC_KEY_MP_A51_RESCUE_X` | `mp_track_a51_rescue_x` | `MUSIC_RESCUE_X` |
| `MD_MUSIC_KEY_MP_A51_ESCAPE` | `mp_track_a51_escape` | `MUSIC_ESCAPE` |
| `MD_MUSIC_KEY_MP_A51_ESCAPE_X` | `mp_track_a51_escape_x` | `MUSIC_ESCAPE_X` |
| `MD_MUSIC_KEY_MP_AIR_BASE` | `mp_track_air_base` | `MUSIC_AIRBASE` |
| `MD_MUSIC_KEY_MP_AIR_BASE_X` | `mp_track_air_base_x` | `MUSIC_AIRBASE_X` |
| `MD_MUSIC_KEY_MP_AIR_FORCE_ONE` | `mp_track_air_force_one` | `MUSIC_AIRFORCEONE` |
| `MD_MUSIC_KEY_MP_AIR_FORCE_ONE_X` | `mp_track_air_force_one_x` | `MUSIC_AIRFORCEONE_X` |
| `MD_MUSIC_KEY_MP_CRASH_SITE` | `mp_track_crash_site` | `MUSIC_CRASHSITE` |
| `MD_MUSIC_KEY_MP_CRASH_SITE_X` | `mp_track_crash_site_x` | `MUSIC_CRASHSITE_X` |
| `MD_MUSIC_KEY_MP_PELAGIC_II` | `mp_track_pelagic_ii` | `MUSIC_PELAGIC` |
| `MD_MUSIC_KEY_MP_PELAGIC_II_X` | `mp_track_pelagic_ii_x` | `MUSIC_PELAGIC_X` |
| `MD_MUSIC_KEY_MP_DEEP_SEA` | `mp_track_deep_sea` | `MUSIC_DEEPSEA` |
| `MD_MUSIC_KEY_MP_DEEP_SEA_X` | `mp_track_deep_sea_x` | `MUSIC_DEEPSEA_X` |
| `MD_MUSIC_KEY_MP_INSTITUTE_DEFENSE` | `mp_track_institute_defense` | `MUSIC_DEFENSE` |
| `MD_MUSIC_KEY_MP_INSTITUTE_DEFENSE_X` | `mp_track_institute_defense_x` | `MUSIC_DEFENSE_X` |
| `MD_MUSIC_KEY_MP_ATTACK_SHIP` | `mp_track_attack_ship` | `MUSIC_ATTACKSHIP` |
| `MD_MUSIC_KEY_MP_ATTACK_SHIP_X` | `mp_track_attack_ship_x` | `MUSIC_ATTACKSHIP_X` |
| `MD_MUSIC_KEY_MP_SKEDAR_RUINS` | `mp_track_skedar_ruins` | `MUSIC_SKEDARRUINS` |
| `MD_MUSIC_KEY_MP_SKEDAR_RUINS_X` | `mp_track_skedar_ruins_x` | `MUSIC_SKEDARRUINS_X` |
| `MD_MUSIC_KEY_MP_END_CREDITS` | `mp_track_end_credits` | `MUSIC_CREDITS` |
| `MD_MUSIC_KEY_MP_SKEDAR_WARRIOR` | `mp_track_skedar_warrior` | `MUSIC_SKEDARRUINS_KING` |

## SFX Filenames

SFX aliases currently map directly to `.wav` filenames under `ext_sfx`.
Detailed row-level ownership lives in `docs/references/sfx_event_registry.csv`.

| Alias | Filename | Current trigger |
| --- | --- | --- |
| `MD_SFX_FALCON_SILENCER_TOGGLE` | `SFX_0022_falcon_silencer_toggle.wav` | Falcon/DMC function toggle fallback replacement |
| `MD_SFX_MATRIX85_FIRE` | `gun-shot-matrix85-01.wav` | Matrix-85 fire sound replacement |
| `MD_SFX_BODY_HIT_01` through `MD_SFX_BODY_HIT_03` | `bullet_body_hit_*.wav` | Custom bot body hit pool |
| `MD_SFX_METAL_HIT_01` through `MD_SFX_METAL_HIT_06` | `bullet_metal_hit_*.wav` | Termination and custom metal hit pool |
| `MD_SFX_TILE_HIT_01` | `tile_hit_01.wav` | Breakable tile hit |
| `MD_SFX_WALL_HIT_01` through `MD_SFX_WALL_HIT_14` | `wall_hit_*.wav` | Stack GE64 texture `0x0088` wall hit pool |
| `MD_SFX_DUST_01` through `MD_SFX_DUST_04` | `dust_*.wav` | Stack GE64 texture `0x0088` dust layer |
| `MD_SFX_DOOR_EXPLOSION_DEBRIS` | `door_explosion_debris.wav` | MP door destruction debris |

## Migration Rule

When adding or renaming custom external audio:

1. Add or update the `MD_*` alias first.
2. Update this registry.
3. Point gameplay code at the alias.
4. Keep the old filename/key stable unless the asset files are renamed in the same tested change.

This keeps future audio renaming closer to one code header plus one registry doc instead of requiring gameplay searches across unrelated systems.
