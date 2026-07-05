# Texture Asset Ownership

This document describes how PERFECT DARK RENAISSANCE should treat external textures while
the project is still migrating away from ROM-backed assets.

It is an ownership guide, not a file-move plan. Do not reorganize large texture
sets until the relevant loader behavior and test coverage are explicit.

## Current Runtime Shape

External PNG texture replacement is handled by `port/src/ext_tex.c`.

At startup, `extTexInit` resolves one active `ext_tex` root with
`fsFullPath("ext_tex")`. That path is affected by the current filesystem overlay:

1. active secondary mod dir for the current `g_ModNum`
2. primary `--moddir`
3. base dir fallback

After resolving the active root, the loader indexes these categories:

| category | current lookup shape | notes |
| --- | --- | --- |
| general textures | `ext_tex/<hex-id>.png` or nested folders containing hex-prefixed filenames | root general textures apply broadly unless a mod namespace overrides them |
| namespaced general textures | `ext_tex/gex/...`, `ext_tex/kakariko/...`, `ext_tex/darknoon/...`, `ext_tex/goldfinger64/...` | selected by `g_ModNum`; these override root general textures for that mod |
| font textures | folders named `fonthandelgothicsm`, `fonthandelgothicmd`, `fonthandelgothicxs`, `fonthandelgothiclg`, or `fontnumeric` | outline glyphs live in each font folder's `outlines` subfolder |
| model textures | top-level model-file folders whose names resolve through `romdataFileGetNumForName` | used for `G_TEXTYPE_MODEL` lookups |
| legacy binary mod textures | `textures/<hex-id>.bin` inside the active mod filesystem | loaded by `modTextureLoad`; this is separate from PNG `ext_tex` |

The current external PNG system is therefore not a global merge of every
`ext_tex` folder in the repository. It is a single resolved active root, with
mod namespaces inside that root for selected general-texture overrides.

## Current Repository Shape

The repository currently has several texture areas with different meanings:

| path | ownership meaning |
| --- | --- |
| `ext_tex/` | project-level/native replacement area; currently sparse |
| `ext_tex/gex/` | reserved namespace for GEX-specific general overrides in the project-level root |
| `mods/mod_allinone/ext_tex/` | active Perfect Dark Renaissance mod texture set used by the current `--moddir` workflow |
| `mods/mod_allinone/ext_tex/gex/` | GEX-specific general overrides when the active resolved root is the All In One/Perfect Dark Renaissance mod root |
| `mods/mod_gex/textures/` | legacy binary texture payload path for the GEX mod filesystem |

## Ownership Rules

Use these rules for new work until the loader is deliberately changed.

| asset kind | preferred owner | reason |
| --- | --- | --- |
| shared PERFECT DARK RENAISSANCE UI, effects, weapons, characters, and fonts | active Perfect Dark Renaissance `ext_tex` root | these belong to the main mod identity and should be available across normal play |
| GEX-only general texture overrides | `ext_tex/gex/` under the active resolved texture root | keeps GEX overrides from changing normal PERFECT DARK RENAISSANCE maps |
| future Kakariko/Dark Noon/Goldfinger-only general overrides | matching namespace folder under the active resolved texture root | follows the existing namespace model |
| model-specific replacement textures | top-level model-file folder under the active resolved texture root | matches `G_TEXTYPE_MODEL` lookup behavior |
| font glyphs and outlines | recognized font folders only | the font loader treats these as a separate texture type and orientation path |
| source art, experiments, exports, and scratch images | outside active lookup folders, or clearly marked archive folders | prevents accidental runtime pickup by hex-prefixed filenames |
| legacy `.bin` texture payloads | leave in `mods/*/textures/` until intentionally migrated | they are loaded through a different path than PNG `ext_tex` |

## Naming Rules

The loader accepts filenames that begin with a hex texture id. Descriptive text
after the id is allowed because the parser reads the leading hex prefix.

Preferred examples:

```text
0c54_blood_hit_side_01.png
0c55_blood_hit_side_02.png
098f_termination_hair_back.png
```

Avoid adding new files with spaces, mixed case, or unclear labels. Existing
files do not need to be renamed casually, especially while texture work is
still in progress.

Use `_Testing` as the standard suffix for intentionally temporary texture
assets:

```text
0df2_termination_top_head_front_Testing.png
```

`_Testing` means the texture id may be correct, but the art, region label, or
final ownership status is still being checked. Registry rows for these files
should use `TESTING` unless the row needs a stronger warning such as
`NEEDS_REVIEW`.

Older suffixes such as `UNCLEAR` or `UNSURE` were ad hoc trial-and-error labels.
Do not use them for new files; prefer `_Testing` going forward.

Run this audit from the project root to check texture registry file paths,
duplicate texture ids, and `_Testing` status consistency:

```sh
python3 tools/audit_texture_registry.py
```

## Native Ownership Direction

Long term, texture ownership should move from "magic texture id files scattered
through folders" toward explicit registries for major content.

Good first registry targets:

- recurring custom characters, especially Termination/Robot Skelington-style body/head/hand
  texture sets; first pass lives at `docs/references/termination_texture_registry.csv`
- custom weapon model and pickup textures
- custom impact/effect texture groups; first pass lives at `docs/references/effects_texture_registry.csv`
- mod-scoped map packs such as GEX-specific overrides
- font/UI replacement sets

The registry should eventually describe:

- PERFECT DARK RENAISSANCE concept name
- legacy texture id or model file id
- replacement path
- scope: shared, GEX-only, Kakariko-only, Dark Noon-only, Goldfinger-only
- status: active, waiting for art, experimental, archive
- notes for shared textures or hand/body/head reuse

Current Robot Skelington runtime note:

- `src/game/tex.c` redirects selected `FILE_CBOILERTREVZ` source texture IDs to the owned Robot Skelington hand texture IDs documented in `docs/references/termination_texture_registry.csv`.
- The same `FILE_CBOILERTREVZ` display-list expansion path now switches known Robot Skelington texture IDs to texture-edge alpha mode. This was tested successfully on 2026-05-11 and should be left stable unless a specific PNG alpha issue needs a narrower texture ID list.

Current Termination UI texture note:

- Termination Mode result cards now use `0c42` for win and `0c43` for lose, while LMS continues to use `02c8` and `02c9`.
- `0c42` and `0c43` are drawn with 32x32 texture coordinates because those legacy texture IDs are small native slots. The external PNGs can still be full 16:9 HD replacements such as 3840x2160.
- Termination's stage-select menu background uses `0c18` only when `g_MpTerminationMenuDialog` is the active MP setup dialog.
- `0c18` is owned by `mods/mod_allinone/ext_tex/menus/0C18-Termination-menu-BG.png`. It was previously misplaced under `ext_tex/weapons`; keep only the menu-owned PNG active so the external texture loader has a single `0c18` replacement.

## Near-Term Safe Steps

1. Keep active texture files where they are until a move has a test plan.
2. Do not add placeholder textures just to satisfy an ownership checklist.
3. Put GEX-specific PNG overrides in the existing `gex` namespace under the
   active resolved `ext_tex` root.
4. Use lowercase hex-prefixed filenames for new texture assets.
5. For large character sets, add registry rows before moving files.
6. Treat `mods/*/textures/*.bin` as legacy binary payloads, not as the same
   system as PNG `ext_tex`.

## Open Questions

- Should the project-level `ext_tex/` become the primary native asset root, or
  should `mods/mod_allinone/ext_tex/` remain the active root until packaging is
  redesigned?
- Should `extTexInit` eventually merge shared root assets and mod-specific
  assets instead of choosing one resolved root?
- Do model-specific texture folders need mod namespaces, or should model folder
  ownership be captured in a registry first?
- Which character texture sets are stable enough to register first?

## Validation For Future Texture Moves

When moving or reclassifying texture assets:

1. Run the game with normal PERFECT DARK RENAISSANCE maps and a GEX map.
2. Confirm font rendering, menu icons, weapon textures, and custom character
   textures still appear.
3. Check `pd.log` for `ext_tex` duplicate or preload warnings.
4. Confirm GEX-only textures do not appear in normal maps unless intentionally
   shared.
5. Commit texture moves separately from runtime loader changes.
