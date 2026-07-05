# PERFECT DARK RENAISSANCE Audio Asset Audit

Updated: 2026-05-03

## Purpose

This document is the stable entry point for audio asset coverage checks.

Do not hard-code current coverage counts here. Music and SFX assets are changing
often, and the live audit scripts are the source of truth.

## Current Audit Commands

Run these from the project root:

```sh
python3 tools/audit_native_assets.py
```

Or run the audio audits individually:

```sh
python3 tools/audit_ext_music.py
python3 tools/audit_ext_sfx.py
```

Optional broader checkpoint:

```sh
python3 tools/audit_texture_registry.py
cmake --build build --target pd -j4
```

## Source Files

| Area | Source of truth |
| --- | --- |
| Audio aliases | `src/include/md/audio_aliases.h` |
| Music docs | `ext_music/NAMING_RULES.txt`, `ext_music/TRACK_MAP_TEMPLATE.csv` |
| Missing music tracker | `ext_music/MISSING_ACTIVE_TRACKS.md` |
| SFX registry | `docs/references/sfx_event_registry.csv` |
| Audio ownership overview | `docs/AUDIO_EVENT_OWNERSHIP.md` |

## Expected Current Gaps

The music audit is expected to list active keys without MP3 files while
replacement tracks are still being selected. Those gaps are tracked in
`ext_music/MISSING_ACTIVE_TRACKS.md`.

The SFX audit is expected to recognize these archive/source WAV files in the
live `ext_sfx` folder:

- `falcon_silencer_on.wav`
- `door_explosion_debris (older).wav`
- `tile_hit_01 (oringla0.wav`

They are not runtime-owned SFX events unless they receive explicit `MD_SFX_*`
aliases and registry rows later.

## Update Rule

When audio assets, aliases, or registry rows change:

1. Update the relevant source file or registry.
2. Run the matching audit script.
3. Update this document only if the audit workflow or expected known gaps
   change.
