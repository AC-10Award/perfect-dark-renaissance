# PERFECT DARK RENAISSANCE Native Ownership Plan

Updated: 2026-05-04

## Goal

Move the project from "redirect and reinterpret old Perfect Dark structures later in the pipeline" toward "PERFECT DARK RENAISSANCE owns the intended data and behavior earlier in the pipeline."

The long-term north star is ROM independence: PERFECT DARK RENAISSANCE should eventually boot from its own data and asset pack rather than requiring a Perfect Dark N64 ROM as the runtime source of truth.

This does not mean removing every Perfect Dark-derived engine structure immediately. It means turning legacy ids, ROM assets, and old tables into temporary compatibility handles while PERFECT DARK RENAISSANCE-authored data progressively becomes the source of meaning.

This is not a one-time rewrite. The intended approach is staged migration:

- keep the game runnable at all times
- migrate one small subsystem at a time
- remove one layer of redirect glue where practical
- stop after each slice and verify the game still builds and behaves correctly

## ROM Independence North Star

The project can move through several states:

1. ROM-required compatibility mod.
2. ROM-required engine with PERFECT DARK RENAISSANCE-authored names, registries, and overrides.
3. ROM-required engine with most high-visibility assets replaced by native assets.
4. Hybrid runtime where remaining ROM dependencies are explicit and measured.
5. PERFECT DARK RENAISSANCE-native runtime that can boot from its own asset/data bundle.

The practical test for each migration is:

- did this move meaning, authoring, or asset ownership earlier into PERFECT DARK RENAISSANCE-owned data?
- did this reduce the amount of behavior that only makes sense by remembering Perfect Dark-era names or file ids?
- did this reduce the number of places where late redirects, remaps, or patches are required?

## Migration Layers

### 1. Authoring Ownership

PERFECT DARK RENAISSANCE concepts become the authoring API, even if they still map to legacy engine ids underneath.

Examples:

- `MD_WEAPON_MATRIX85 -> WEAPON_MAGSEC4`
- `MD_WEAPON_TEXT_MATRIX85 -> L_GUN_010`
- `MD_SCENARIO_TERMINATION -> MPSCENARIO_TERMINATION`
- `MD_BODY_ROBOT_SKELINGTON -> BODY_ROBOT_SKELINGTON`
- `MD_MUSIC_MP_DD_CENTRAL_X -> "mp_track_dd_central_x"`
- `MD_SFX_FALCON_SILENCER_TOGGLE -> "SFX_0022_falcon_silencer_toggle.wav"`

This layer is low risk because it can start as aliases, documentation, and small registries.

### 2. Asset Ownership

PERFECT DARK RENAISSANCE assets replace ROM-backed assets one category at a time.

Major categories:

- textures and character textures
- fonts and UI presentation images
- music
- SFX
- weapon and prop models
- character models, heads, bodies, hands
- maps, pads, setup data, and scenario-specific objects
- animations and cutscene/video presentation

External texture, music, and SFX support are early examples of this layer. The next step is to make their event/name mapping more native-owned instead of scattering filename literals and texture redirects across gameplay code.

### 3. Gameplay Data Ownership

PERFECT DARK RENAISSANCE-authored registries and tables should become the clear source for mod-facing gameplay data.

Candidate registries:

- weapon registry
- multiplayer scenario registry
- external audio event registry
- custom character/body/head registry
- texture/model remap registry
- material and impact-effect registry
- stage/map registry
- title/frontend presentation registry

Early registry entries can still contain legacy ids. The key is that authors start from PERFECT DARK RENAISSANCE names first.

### 4. Runtime Ownership

The final layer is reducing assumptions that a Perfect Dark ROM must be present at runtime.

Long-term runtime targets:

- boot from a PERFECT DARK RENAISSANCE asset/data directory
- load native registries before legacy fallback tables
- make ROM-backed fallbacks optional rather than required
- isolate file-id compatibility into a smaller bridge layer
- replace generated ROM asset headers with PERFECT DARK RENAISSANCE-authored manifests where practical

This is the highest-risk layer and should come after authoring, asset, and gameplay-data ownership have made the remaining ROM dependencies explicit.

## What "Native-Owned" Means

A subsystem is considered more native-owned when:

- the primary source of truth is a PERFECT DARK RENAISSANCE-authored table or code path
- editing the subsystem no longer requires remembering old PD-era names first
- fewer later-stage redirects are needed
- the user-facing name, author-facing name, and engine-facing identity are closer together
- ROM-backed data is a fallback or compatibility detail rather than the only meaningful source

## Working Rule

Use a weekly migration model, not a giant rewrite pass.

Recommended cadence:

1. Pick one bounded target.
2. Document where the current source of truth actually lives.
3. Define the desired native-owned source of truth.
4. Migrate only that slice.
5. Build and test.
6. Record which redirect logic or legacy indirection can now be reduced.

Compatibility rule:

- legacy `WEAPON_*`, `BODY_*`, `HEAD_*`, `FILE_*`, texture ids, stage ids, and language ids may remain in engine internals
- new mod-authored work should prefer `MD_*` aliases or PERFECT DARK RENAISSANCE registries where they exist
- when a native registry still maps to a legacy id, document that mapping explicitly
- avoid renumbering global legacy ids unless a smaller bridge-layer migration cannot solve the problem

## Active Language Scope

PERFECT DARK RENAISSANCE is currently an NTSC-English project. Treat English `ntsc-final` assets as the active content ownership path.

PAL-English and Japanese language payloads may remain in the tree while the inherited asset system still expects regional/language variants, but they are compatibility scaffolding rather than first-class migration targets. Do not spend migration effort preserving or expanding them unless a specific test or runtime dependency requires it.

Practical rule:

- author new menu, weapon, scenario, and frontend text in the English NTSC source JSON first
- regenerate English payloads by default
- only regenerate PAL-English or Japanese payloads when explicitly requested or when a compatibility payload has already been audited
- do not treat old Japanese/PAL text mismatches as bugs unless they affect the NTSC-English runtime path
- if PERFECT DARK RENAISSANCE later supports additional languages, rebuild localization deliberately from the native asset system instead of carrying forward inherited regional ROM baggage

## 2026-05-03 Checkpoint

Recent migration work moved several MP frontend and scenario text paths closer to PERFECT DARK RENAISSANCE ownership without changing the underlying legacy ids.

Completed:

- `port/src/fs.c` read fallback now checks the active specialized mod folder first, then shared `mods/mod_allinone`, then base/ROM data. This fixed stale `LmpmenuE` resolution when GoldenEye X maps switched `g_ModNum` away from the shared mod.
- `port/src/fs.c` write paths now use a write-specific resolver so saves/config/output paths do not accidentally write into mod overlay folders because of the read fallback order.
- MP scenario names, short names, and the PvE group label now use `MD_MPSCENARIO_*` aliases instead of direct raw custom text ids.
- MP arena/category labels for the repurposed `L_MPMENU_294` through `L_MPMENU_338` range now use `MD_MPSTAGE_TEXT_*` aliases from `src/include/md/mp_stage_aliases.h`.
- Last Man Standing / Termination curated stage lists now resolve exact custom map names through `MD_MPSTAGE_TEXT_*` where safe, while preserving literal fallbacks for vanilla PD labels and duplicate-disambiguation names such as `Temple GE64`.
- MP mode menu labels now use `MD_MPMENU_TEXT_*` where the menu path safely accepts language ids.
- Literal-only menu/body labels are centralized as `MD_MPMENU_LITERAL_*` aliases in `src/include/md/mp_menu_aliases.h`. These are owned names, but not fully payload-backed yet.
- LMS manual in-game abort/exit now records the curated LMS result as a loss before ranking-based win inference, preventing quit matches from showing the win result screen.

Important lesson:

- Do not blindly replace literal menu text with language ids. Some menu dialogs/items require `MENUITEMFLAG_LITERAL_TEXT` or `MENUDIALOGFLAG_LITERAL_TEXT`, and switching those paths to language ids can corrupt displayed text. Use `MD_*_TEXT_*` only for language-backed paths that are known safe; use `MD_*_LITERAL_*` for owned text that still needs a renderer/payload migration.

Current references:

- `docs/AUDIO_EVENT_OWNERSHIP.md`
- `docs/MP_LANGUAGE_PAYLOAD_AUDIT.md`
- `docs/MP_SCENARIO_REGISTRY.md`
- `src/include/md/audio_aliases.h`
- `src/include/md/scenario_aliases.h`
- `src/include/md/mp_stage_aliases.h`
- `src/include/md/mp_menu_aliases.h`

Backlog:

- Add dedicated payload entries for the `MD_MPMENU_LITERAL_*` backlog and migrate each literal-only menu path one screen at a time.
- Continue the audio migration by grouping the `MD_MUSIC_KEY_MP_*` bridge aliases into future PERFECT DARK RENAISSANCE track categories once the replacement soundtrack direction is defined.
- Consider a third curated LMS result state later: `WIN`, `LOSE`, and `QUIT`, with separate art/music/text for a future `You Quit!` result screen.
- Continue reducing PerfectHead/Game Boy Camera era text dependencies once their reused ids are fully documented and bridged through PERFECT DARK RENAISSANCE aliases.

## 2026-05-04 Checkpoint

Recent font texture work moved HD font replacement closer to an owned, auditable asset workflow.

Completed:

- Font texture loading now supports a font-only prefixed filename form. Existing leading-id names such as `41.png` and `41-b.png` still work, and font-only names such as `b-main-menu-41.png` or `O-sub-menu-2e.png` can now sort by author-facing character/use case while mapping to the same legacy glyph id.
- Font glyph scanning now bounds-checks glyph ids before indexing the font texture table.
- Shared font glyph naming and replacement rules live in `mods/mod_allinone/ext_tex/fonts/GLYPH_NAMING.md`.
- `fonthandelgothicmd` has a folder-local README pointing to the shared rules.
- `tools/audit_font_textures.py` audits live font folders for unmapped PNGs, duplicate glyph ids, out-of-range glyph ids, unreadable PNGs, non-RGBA PNGs, mixed canvas sizes, and mixed PNG formats.
- `tools/audit_native_assets.py` now runs the font texture audit as part of the native asset audit wrapper.

Important lesson:

- Treat each HD font family as a full-set asset. Mixing glyphs from different export passes can make individual glyphs appear wrongly sized or warped even when the PNG itself is valid. Preserve known-good font folders and replace live glyph sets as a coherent batch.

Current references:

- `mods/mod_allinone/ext_tex/fonts/GLYPH_NAMING.md`
- `mods/mod_allinone/ext_tex/fonts/fonthandelgothicmd/README.md`
- `tools/audit_font_textures.py`
- `port/src/ext_tex.c`

Backlog:

- Consider whether `fonthandelgothicsm`, `fonthandelgothicxs`, `fonthandelgothiclg`, and `fontnumeric` need folder-local notes once their replacement passes begin.
- Longer term, migrate from bitmap-only replacement toward owned font metrics: width, height, baseline, and kerning.

## Migration Priorities

### Good First Targets

- Weapon names and weapon ordering
- Title flow / intro timing / frontend branding
- Custom scenario registration and menu ownership
- External music and SFX event mapping
- Recurring custom texture/model ownership for major custom characters or props
- A first `MD_*` alias header for safe author-facing constants

### Medium-Risk Later Targets

- Multiplayer weapon-set ownership
- Stage-specific custom object naming and pickups
- Asset registry cleanup for repeated redirect-heavy content
- More direct ownership of model/texture bindings for custom actors
- Native manifests for groups of replaced assets

### Avoid Early

- Global asset renumbering across the whole engine
- Broad core render-system rewrites
- Deep player/AI refactors that are not actively blocking work
- Any migration that requires many unrelated systems to change at once
- Removing the ROM boot requirement before replacement assets and data registries exist

## Decision Filter

A subsystem is a good native-ownership candidate if one or more of these are true:

- editing it still requires old PD-era names first
- the source of truth is ambiguous
- there are repeated redirects or remaps every time it is touched
- future mod work would become simpler if the data lived earlier in the pipeline
- it is a visible ROM dependency that PERFECT DARK RENAISSANCE expects to replace eventually
- it can be bridged with a small registry or alias layer before deeper runtime changes

## First Migration Target: Weapon Names

### Why This Is First

This is already causing editing friction. Weapon display names may be changed, but ordering and logic still depend on legacy `WEAPON_*` identities and old PD naming assumptions.

Current symptom:

- when reorganizing a weapon list, the author still has to think in terms of legacy engine weapon ids and names first
- the displayed weapon name is not the only or earliest source of truth

### Current Source-of-Truth Problem

Weapon identity is currently split across multiple layers:

- engine ids and enums such as `WEAPON_AR34`, `WEAPON_CMP150`, `WEAPON_DY357MAGNUM`
- display-name mapping table in `src/game/propobj.c`
- inventory weapon definitions in `src/game/invitems.c`
- localized text strings in `src/assets/*/lang/gun.json`
- gameplay ordering arrays such as `g_AutoSwitchWeaponsPrimary` and `g_AutoSwitchWeaponsSecondary` in `src/game/bondgun.c`
- multiplayer presets and challenge setups that still use legacy weapon ids and assumptions

In other words, names are partially re-skinned, but the authoring workflow is still legacy-first.

### Native-Owned End State for Weapon Names

Desired outcome:

- the project has one explicit PERFECT DARK RENAISSANCE weapon registry or alias table
- that registry is the first place authors look when identifying, ordering, or renaming weapons
- gameplay lists are authored from that registry or from a clearly documented translation layer
- legacy `WEAPON_*` ids remain as engine compatibility values, but not as the main author-facing naming system
- replacement models, textures, SFX, and display text can later attach to the PERFECT DARK RENAISSANCE weapon entry instead of being found through scattered legacy references

### Suggested Stages for Weapon Name Migration

#### Stage 1: Define a Stable Author-Facing Alias Layer

Create one document or table that maps:

- legacy engine id
- current displayed name
- intended PERFECT DARK RENAISSANCE author-facing name
- notes on any temporary mismatch

This does not replace engine ids yet. It creates one clear author-facing reference.

Suggested outputs:

- a dedicated weapon alias document in `docs/`
- a small code-side `MD_WEAPON_*` alias layer for mod-authored weapon lists
- a small code-side `MD_WEAPON_TEXT_*` alias layer for mod-authored display-name references
- optional registry data that can later grow fields for display strings, models, textures, SFX, and balancing notes

Current live reference:

- `docs/WEAPON_ALIAS_REGISTRY.md`
- `src/include/md/weapon_aliases.h`
- `src/include/md/weapon_display.h`

#### Stage 2: Centralize Name Resolution

Audit weapon-name lookup paths and make one mapping layer the primary source for author-facing names.

Current live anchor:

- `src/game/propobj.c`
- `src/game/invitems.c`

Goal:

- avoid name definitions being scattered conceptually across many unrelated files
- treat this mapping as the canonical bridge from legacy engine id to PERFECT DARK RENAISSANCE naming
- keep legacy `L_GUN_*` ids as the storage mechanism for now, while code-side lookup tables prefer `MD_WEAPON_TEXT_*` names where a live alias exists

#### Stage 3: Decouple Ordering From Legacy Mental Model

Audit gameplay ordering lists and annotate or migrate them so editing no longer depends on remembering PD names first.

Known example:

- `src/game/bondgun.c`

Goal:

- add clear comments or a helper layer so the authored order reflects PERFECT DARK RENAISSANCE intent
- remove the need to mentally translate "AR34 means my renamed rifle" every time a reorder happens

Possible intermediate step:

- keep the legacy constants, but add comment labels beside each entry using the intended PERFECT DARK RENAISSANCE names
- then replace safe mod-authored list entries with `MD_WEAPON_*` aliases while leaving inherited engine logic alone

#### Stage 4: Audit User-Facing Weapon Strings

Check for leftover legacy user-facing text in:

- `src/assets/*/lang/gun.json`
- `src/assets/*/lang/mpweapons.json`
- challenge and training text
- mission reward strings

Goal:

- make the displayed naming consistently match the author-facing naming

#### Stage 5: Decide What Should Stay Legacy Internally

Not every legacy id needs to be renamed at the engine level.

Keep unchanged for now:

- internal `WEAPON_*` enum constants if changing them would cause large code churn
- model file numbers and animation names that are stable and harmless

Migrate first:

- author-facing reference layer
- ordering lists
- display-name lookup paths

#### Stage 6: Grow Weapon Entries Toward Asset Ownership

Once names and ordering are stable, use the same weapon registry to track asset replacement status.

Potential fields:

- native display name
- legacy engine id
- pickup/display string ids
- native model status
- native texture status
- native SFX status
- native balancing/status notes

This turns the weapon migration into a direct contributor to ROM independence rather than only a naming cleanup.

## ROM Dependency Scorecard

Use this checklist to measure migration progress over time.

| Area | Current State | Desired Direction |
| --- | --- | --- |
| Weapon names/order | Partially native, still legacy-id authored | `MD_WEAPON_*` aliases and registry-first authoring |
| Language assets | English NTSC weapon JSON is author-owned; some packed `mods/*/files/L*` payloads still contain mod-specific text not mirrored in JSON | Move active English NTSC mod text into JSON before regeneration; keep PAL/Japanese as legacy compatibility unless explicitly audited |
| MP language payloads | `LmpmenuE/P` custom MP names are JSON-owned; `LmpmenuJ` and some `Lmpweapons*` still contain custom packed text | Continue migrating active English MP strings into JSON, then regenerate audited banks/languages only |
| MP scenarios | First `MD_MPSCENARIO_*` alias layer exists; registration still spread across legacy-style hooks | Scenario registry owns labels, ids, options, hooks, music, and test notes |
| External music | Native files exist, event mapping mostly code/filename based | Audio event registry maps gameplay event to key/pool/fallback |
| External SFX | Native WAV playback exists, trigger calls are scattered | SFX event registry with named events and optional multi-voice support later |
| External textures | Native overrides exist, ids and remaps still legacy-heavy; see `docs/TEXTURE_ASSET_OWNERSHIP.md` | Texture/model ownership registry for major custom actors and props |
| Robot Skelington/Termination | Working through body/head/file/texture redirects | Custom character registry owns body/head/files/textures/scale/hand policy |
| Impact effects | Prototype stage/material hooks exist; see `docs/MATERIAL_IMPACT_OWNERSHIP.md` | Material/impact registry owns texture ids, SFX pools, sprite effects, stage scope |
| Title/frontend | Static presentation assets are preferred | Native frontend presentation registry and frame-sequence path |
| Profiles | Filesystem-backed MP profiles exist | Keep native profile storage independent of legacy pak assumptions |
| Boot/runtime | ROM still required | Native asset/data bundle becomes primary, ROM compatibility bridge shrinks |

## Template For Future Weekly Migrations

Use this format for each subsystem:

### Target

One sentence naming the subsystem.

### Current Source of Truth

List the real files/tables/functions that currently define behavior.

### Problem

Describe the legacy dependency or redirect burden.

### Native-Owned Goal

Describe what should become the earlier, clearer source of truth.

### ROM Dependency Reduced

Describe which ROM-backed name, file id, texture id, language id, asset, or runtime assumption becomes less central after the change.

### Safe First Step

Make the smallest useful change that improves ownership without destabilizing the game.

### Validation

- build passes
- affected flow still works in game
- documentation updated if the author-facing workflow changed

## Recommended Near-Term Weekly Order

1. Weapon names, ordering, and first `MD_WEAPON_*` aliases
2. Scenario/menu registration ownership
3. External music/SFX event ownership
4. High-traffic custom texture/model ownership for recurring custom content
5. Title/frontend ownership cleanup and frame-sequence groundwork
6. Material/impact registry cleanup for custom hit effects

## Practical Reminder

The project does not need to stop being built on Perfect Dark internals in order to become PERFECT DARK RENAISSANCE in practice.

The real objective is:

- PERFECT DARK RENAISSANCE chooses the meaning earlier
- authors work from PERFECT DARK RENAISSANCE concepts first
- native assets and data replace ROM-backed sources one category at a time
- legacy PD structures become implementation details instead of the main design language
- the ROM requirement shrinks from "required source of truth" to "temporary compatibility bridge"
