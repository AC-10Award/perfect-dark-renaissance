# Material And Impact Ownership

This document maps the current PERFECT DARK RENAISSANCE material/impact hooks to owned SFX
aliases and effect texture groups.

It is a documentation checkpoint only. Runtime behavior still lives in the
existing gameplay call sites.

## Current Runtime Helper

The current SFX selection pools are centralized in:

- `src/include/md/material_impacts.h`
- `src/game/md_material_impacts.c`

Gameplay call sites still decide when an impact event fires, but owned external
SFX filenames now flow through named material-impact helper functions.

## Current Owned Impact Events

| event | current trigger | code owner | owned SFX | related texture registry group | notes |
| --- | --- | --- | --- | --- | --- |
| custom body hit | bot/character body hit path | `src/game/chr.c` | `MD_SFX_BODY_HIT_01` through `MD_SFX_BODY_HIT_03` | `blood_hit_side`, `blood_hit_front` | SFX pool is native-owned; blood texture rendering still follows existing character hit logic |
| Termination metal hit | Termination mode boss damage path | `src/game/chraction.c` | `MD_SFX_METAL_HIT_01` through `MD_SFX_METAL_HIT_06` | `spark_floor`, `smoke_puff` | SFX pool is native-owned; future registry should own the Termination material profile directly |
| custom background metal hit | selected stage/material texture hit path | `src/game/prop.c` | `MD_SFX_METAL_HIT_01` through `MD_SFX_METAL_HIT_06` | `spark_floor`, `smoke_puff` | Currently hard-coded to `STAGE_EXTRA15` and texture ids `0x007f` / `0x007d` |
| custom background wall hit | selected stage/material texture hit path | `src/game/prop.c` | `MD_SFX_WALL_HIT_01` through `MD_SFX_WALL_HIT_14`; `MD_SFX_DUST_01` through `MD_SFX_DUST_04` | `wall_hit_front`, `wall_hit_side` | Currently hard-coded to `STAGE_EXTRA15` and texture id `0x0088` |
| breakable tile hit | background geometry damage path | `src/game/bg.c` | `MD_SFX_TILE_HIT_01` | `smoke_puff`, `unclassified_effect` | SFX is native-owned; damaged geometry and puff visuals remain legacy-code driven |
| MP door destruction debris | multiplayer door destruction path | `src/game/propobj.c` | `MD_SFX_DOOR_EXPLOSION_DEBRIS` | `spark_floor`, `unclassified_effect` | SFX is native-owned; debris sparks remain legacy-code driven |

## Current Registries

| registry | purpose |
| --- | --- |
| `docs/references/material_impact_registry.csv` | current material/impact event ownership rows |
| `docs/references/sfx_event_registry.csv` | owned external SFX events and WAV filenames |
| `docs/references/effects_texture_registry.csv` | current custom impact/effect PNG texture files |
| `docs/references/termination_texture_registry.csv` | Termination/Robot Skelington character texture ownership |

## Current Audits

Run the complete native asset audit:

```sh
python3 tools/audit_native_assets.py
```

Or run the relevant focused audits:

```sh
python3 tools/audit_ext_sfx.py
python3 tools/audit_texture_registry.py
python3 tools/audit_material_impact.py
```

## Migration Direction

The next ownership layer should be a material/impact registry that describes
gameplay concepts instead of scattering stage, texture, and SFX decisions across
damage code.

Future registry fields should likely include:

- native event name, such as `termination_metal_hit`
- material category, such as `body`, `metal`, `tile`, or `door_debris`
- stage scope, if any
- legacy texture ids or surface types
- owned SFX event group
- owned texture/effect group
- fallback behavior
- test notes

## Runtime Refactor Direction

The first low-risk code refactor centralized the current hard-coded SFX pools
behind named helper functions, without changing when they fire.

Do not change effect spawning, blood rendering, debris behavior, or material
matching until the current events have focused in-game tests.

## Known Legacy Dependencies

- `src/game/prop.c` still uses explicit stage and texture-id checks for one
  custom background hit sound.
- `src/game/chraction.c` detects Termination boss damage through scenario-specific
  helpers.
- effect textures are registered as files, but gameplay does not yet choose
  them through a native material registry.
- unclassified effect texture ids `002a`, `0ce4`, `0ce5`, `0ce6`, and `0ce7`
  need better labels before they should drive runtime ownership decisions.
