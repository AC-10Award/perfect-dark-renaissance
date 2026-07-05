PERFECT DARK RENAISSANCE - CUSTOM SOUND EFFECTS
================================================

This folder contains WAV files used by the Renaissance custom SFX system and
patched sound-bank data.

Runtime support is implemented in:

    port/src/ext_sfx.c
    port/include/ext_sfx.h

Sound-event ownership is documented in:

    docs/references/sfx_event_registry.csv

The sound-bank patching utility is:

    tools/patch_sfx_bank.py

To audit the bundled filenames and registered events, run this command from
the repository root:

    python3 tools/audit_ext_sfx.py

WAV files used by the runtime should remain mono PCM files in the format
expected by their assigned event or bank slot. Changing a filename does not
automatically change the corresponding source hook or sound-bank assignment.

Only distribute sound recordings that you created or have permission to
redistribute.
