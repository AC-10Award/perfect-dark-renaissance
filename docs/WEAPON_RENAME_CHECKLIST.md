# PERFECT DARK RENAISSANCE Weapon Rename Checklist

Updated: 2026-05-02

## Purpose

Use this note when renaming a weapon so the new name appears consistently across gameplay, menus, HUD, training text, and multiplayer.

This exists because weapon names currently come from more than one path. A rename that only changes one layer can still leave old names visible in other screens.

## Core Rename Targets

### 1. Base weapon language strings

Update the main weapon name in:

- `src/assets/*/lang/gun.json`

Typical target:

- `L_GUN_*` entry used by the weapon's `short name` and `name`
- matching `MD_WEAPON_TEXT_*` alias in `src/include/md/weapon_display.h`

Then regenerate any live external language payloads used by the mod:

```sh
tools/regenerate_weapon_lang
```

CMake currently generates language headers only. Runtime `langGet` loads packed `mods/*/files/Lgun*` files first, so stale external language files can still show old names even when JSON is correct.

For broader language sync work, use:

```sh
tools/regenerate_lang_asset gun mpweapons mpmenu options
```

Use explicit bank names and only audited languages. For `mpmenu`, use `tools/regenerate_lang_asset mpmenu --lang en --lang gb`; do not regenerate `LmpmenuJ` until its source ownership is resolved.

Example:

- `WEAPON_MAGSEC4` uses `L_GUN_010`

### 2. Weapon description text

Update any descriptive text that still mentions the old name in:

- `src/assets/*/lang/misc.json`
- `src/assets/*/lang/dish.json`

Typical targets:

- firing range descriptions
- training descriptions
- weapon overview text

### 3. Inventory weapon definition

Confirm the weapon's source definition in `src/game/invitems.c`.

Check:

- `short name`
- `name`
- `manufacturer`
- `description`

This verifies which `L_GUN_*` and related ids the weapon is actually wired to.

Current bridge:

- live registry-covered `short name` and `name` ids should use `MD_WEAPON_TEXT_*`
- alternate, mode, manufacturer, and description text ids may still use direct `L_GUN_*` references

### 4. Runtime weapon-name functions

Check runtime name resolution in `src/game/bondgun.c`.

Functions:

- `bgunGetName`
- `bgunGetShortName`

These affect:

- weapon wheel
- weapon selection menus
- some training displays
- multiplayer weapon picker labels via `mpGetWeaponLabel`

If asset-driven renames are not enough, this is the safest direct override point.

### 5. Inventory menu path

Check inventory name resolution in `src/game/inv.c`.

Functions:

- `invGetNameIdByIndex`
- `invGetNameByIndex`
- `invGetShortNameByIndex`

These affect:

- pause inventory menu
- any inventory-driven UI using indexed weapon names

### 6. Bottom-right HUD weapon label

Check HUD rendering in `src/game/bondgun.c`.

This path uses:

- `invGetNameIdByIndex`
- `langGet`

Important detail:

- this HUD path may not use `bgunGetShortName`
- it can therefore keep showing the old name even after the wheel and picker are fixed

### 7. Multiplayer-specific language strings

Check:

- `src/assets/*/lang/mpweapons.json`

This matters for:

- multiplayer unlock text
- weapon-related MP labels
- challenge and cheat reward names tied to MP weapon text ids

Note:

- not every active MP weapon label comes from `mpweapons.json`
- some MP weapon selection paths still call `bgunGetName`

### 8. Multiplayer weapon label path

Check `src/game/mplayer/mplayer.c`.

Functions:

- `mpGetWeaponLabelByMpWeaponNum`
- `mpGetWeaponLabel`

Current behavior:

- most active MP picker labels resolve through `bgunGetName`

### 9. Training and firing range text

Check:

- `src/game/training.c`
- `src/game/trainingmenus.c`

These may use:

- `langGet(L_MISC_*)`
- `langGet(L_DISH_*)`
- `bgunGetName`

### 10. Pickup / prop display names

Check `src/game/propobj.c`.

This affects:

- object naming
- pickup-related text
- contextual prop labels

Current bridge:

- live registry-covered weapon entries in `var8006aa94pf` should use `MD_WEAPON_TEXT_*`
- the `MD_WEAPON_TEXT_*` aliases still map to `L_GUN_*` ids for now

## Practical Rename Workflow

When renaming a weapon:

1. Update `gun.json`.
2. Regenerate live `mods/*/files/Lgun*` payloads for the active mod folders.
3. Update or add the matching `MD_WEAPON_TEXT_*` alias.
4. Update `misc.json` and `dish.json`.
5. Verify the `invitems.c` ids for that weapon.
6. Test the weapon wheel.
7. Test the multiplayer weapon picker.
8. Test the bottom-right HUD label.
9. Test the pause inventory menu.
10. Test pickup/object name text.
11. Test training/firing range text if relevant.
12. Check `mpweapons.json` if the weapon appears in MP unlocks, challenge text, or related labels.
13. If any screen still shows the old name, patch the runtime resolver in `bondgun.c` or `inv.c` for that weapon.

## Current Known Example

For `WEAPON_MAGSEC4 -> Matrix-85`, the rename had to touch:

- `gun.json`
- `misc.json`
- `dish.json`
- `bondgun.c`
- `inv.c`

The temporary direct runtime overrides in `bondgun.c` and `inv.c` were removed after regenerating the packed `mods/*/files/Lgun*` language payloads.

The current intended path is `MD_WEAPON_TEXT_MATRIX85 -> L_GUN_010 -> packed Lgun runtime asset`.
