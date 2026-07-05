# Character Remap Workflow

This file defines the preferred format for future multiplayer character replacement requests.

Use this workflow whenever a character will:
- reuse an existing head model
- reuse an existing body model
- replace some or all of the original textures via `ext_tex`

## Goal

Make each character request specific enough that implementation can be done without guessing:
- which MP slot is being replaced
- which head model is used
- which body model is used
- which original texture IDs are being remapped
- which new texture IDs and files are intended to replace them

## Standard Request Format

Each character request should include these sections:

1. Character Slot
2. Geometry
3. Texture Remaps
4. Asset Paths
5. Rules

## Recommended Request Example

```text
Character Slot: MPBODY_DD_SECGUARD
Display Name: dataDyne Security Elite

Geometry:
Head Source: HEAD_DD_SHOCK
Body Source: BODY_AREA51TROOPER

Texture Remaps:
Head:
005d -> 22ab
02ff -> 22ac
00ef -> 34dd

Body:
0142 -> 4100
0143 -> 4101

Asset Paths:
ext_tex/22ab.png
ext_tex/22ac.png
ext_tex/34dd.png
ext_tex/4100.png
ext_tex/4101.png

Rules:
Apply head remaps only to the selected head model
Apply body remaps only to the selected body model
Do not alter hands
```

## Field Rules

### Character Slot

This should identify the target multiplayer character slot being overwritten.

Preferred values:
- `MPBODY_*`
- an existing roster entry name from `mp_character_reference.csv`

Also provide:
- `Display Name`

## Geometry

Always provide:
- `Head Source`
- `Body Source`

Preferred values:
- `HEAD_*`
- `BODY_*`

These should match the identifiers in:
- `mp_character_reference.csv`
- `src/game/mplayer/mplayer.c`
- `src/game/modeldata/robot.c`

## Texture Remaps

Separate remaps by scope:
- `Head`
- `Body`

Each remap should be:
- original texture ID -> replacement texture ID

Example:
- `055d -> 22ab`

Important:
- original texture IDs come from the stock/original asset pool
- replacement texture IDs are the IDs we will serve from `ext_tex`

If a texture is shared between head and body, call that out explicitly in `Rules`.

## Asset Paths

List the expected replacement files for the remapped IDs.

Preferred format:
- `ext_tex/22ab.png`
- `ext_tex/34dd.png`

If the files do not exist yet, say so explicitly.

## Rules

Use this section for anything implementation-specific, such as:
- head-only remaps
- body-only remaps
- shared remaps
- keep hands unchanged
- keep sunglasses hidden
- only affect one roster slot
- reuse same replacement texture on multiple characters

## Best Practices

- Always use code IDs instead of only plain-English names.
- Keep head remaps separate from body remaps.
- If you are unsure whether a texture belongs to the head or body, mark it as `Unknown` instead of guessing.
- If one character will become a template for several variants, create one base spec and then variant specs under it.

## Implementation Notes

The expected long-term setup is:
- original texture IDs remain the stable lookup keys
- `ext_tex` becomes the main replacement source
- character definitions control which head/body models are paired
- remap specs define which original IDs are redirected to which replacement assets

## Files Used With This Workflow

- `docs/references/mp_character_reference.csv`
- `docs/references/character_remap_template.csv`

## Minimal Request

If you want the shortest possible usable request, use this:

```text
Character Slot: MPBODY_...
Display Name: ...
Head Source: HEAD_...
Body Source: BODY_...

Head Remaps:
old -> new

Body Remaps:
old -> new

Assets:
ext_tex/....

Rules:
...
```
