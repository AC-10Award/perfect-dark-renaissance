PERFECT DARK RENAISSANCE - CUSTOM MUSIC GUIDE
================================================

This folder is optional. It should be named exactly:

    ext_music

Do not rename it. The game searches for that exact folder name when it starts.
If the folder is absent, empty, or contains no matching file, the game uses the
music from the user's own Perfect Dark ROM.


QUICK START
-----------

1. Use an MP3 file no larger than 32 MB.
2. Choose the event or multiplayer music key from the lists below.
3. Rename the MP3 to that key, for example:

       main_menu.mp3

4. Put the MP3 anywhere inside this ext_music folder. Subfolders are allowed.
5. Fully quit and restart the game. Music files are discovered at startup.
6. Visit the relevant menu or play the relevant multiplayer track to test it.

No rebuild is needed when adding, replacing, or removing MP3 files.


FILENAME RULES
--------------

Use one of these forms:

    <key>.mp3
    <key>_01.mp3
    <key>_02.mp3
    <key>-any-description-01.mp3

Examples:

    main_menu.mp3
    combat_sim_menu_01.mp3
    combat_sim_menu_02.mp3
    mp_track_chicago-my-custom-song-01.mp3

Important rules:

- The filename must begin with an exact key listed below.
- The extension must be .mp3.
- Use an underscore or hyphen between the key and any suffix.
- Do not put spaces inside the key.
- Lowercase names are recommended for portability.
- A direct file such as main_menu.mp3 is the single-track form.
- Numbered or descriptive variants form a random pool. If variants exist, the
  game chooses among them instead of the direct file.
- Variant numbers must be greater than zero, such as _01, _02, and _03.
- The game searches subfolders recursively, so folders such as menu_music and
  level_music are only for organization and do not affect matching.
- Keep only one copy of a particular filename/key variant to avoid ambiguous
  results.


MENU AND EVENT KEYS
-------------------

    opening_company_logos
    main_title_logo
    main_menu
    combat_sim_menu
    termination_menu
    combat_sim_complete
    last_man_standing_win
    last_man_standing_lose
    termination_win
    termination_lose
    multiplayer_death
    mission_success
    mission_unknown
    mission_failed
    pause_menu
    solo_death
    ci_training

multiplayer_death is a short death overlay rather than normal background music.
Short files are recommended for that key.


MULTIPLAYER MUSIC KEYS
----------------------

These keys correspond to the stock music choices shown in the Combat Simulator
music menu. The visible menu labels remain the original Perfect Dark labels.

    mp_track_dark_combat
    mp_track_skedar_mystery
    mp_track_ci_operative
    mp_track_datadyne_action
    mp_track_maian_tears
    mp_track_alien_conflict
    mp_track_carrington_institute
    mp_track_dd_central
    mp_track_dd_central_x
    mp_track_dd_research
    mp_track_dd_research_x
    mp_track_dd_extraction
    mp_track_dd_extraction_x
    mp_track_carrington_villa
    mp_track_carrington_villa_x
    mp_track_chicago
    mp_track_chicago_x
    mp_track_g5_building
    mp_track_g5_building_x
    mp_track_a51_infiltration
    mp_track_a51_infiltration_x
    mp_track_a51_rescue
    mp_track_a51_rescue_x
    mp_track_a51_escape
    mp_track_a51_escape_x
    mp_track_air_base
    mp_track_air_base_x
    mp_track_air_force_one
    mp_track_air_force_one_x
    mp_track_crash_site
    mp_track_crash_site_x
    mp_track_pelagic_ii
    mp_track_pelagic_ii_x
    mp_track_deep_sea
    mp_track_deep_sea_x
    mp_track_institute_defense
    mp_track_institute_defense_x
    mp_track_attack_ship
    mp_track_attack_ship_x
    mp_track_skedar_ruins
    mp_track_skedar_ruins_x
    mp_track_end_credits
    mp_track_skedar_warrior


EXAMPLE SETUP
-------------

    ext_music/
        README.txt
        menu_music/
            main_menu.mp3
            combat_sim_menu_01.mp3
            combat_sim_menu_02.mp3
        level_music/
            mp_track_chicago.mp3
            mp_track_chicago_x.mp3
        multiplayer_death/
            multiplayer_death_01.mp3
            multiplayer_death_02.mp3


TROUBLESHOOTING
---------------

- Fully restart the game after changing files.
- Confirm the folder is named ext_music, not ext_text or custom_music.
- Confirm the filename begins with an exact key and ends in .mp3.
- Confirm the MP3 is 32 MB or smaller.
- Check pd.log. A successful startup contains:

      ext_music: directory found

  A successful override contains a line similar to:

      ext_music: started external track key=main_menu

  "no file for key" means the folder was found but no correctly named MP3
  matched that event.
- If an override is missing or cannot be decoded, the game normally falls back
  to the soundtrack contained in the user's ROM.


ADDITIONAL REFERENCE FILES
--------------------------

- MUSIC_TRACK_MAP_TEMPLATE.csv can be copied and filled in to plan a soundtrack.
- Run tools/audit_ext_music.py from the project root to audit current filenames.

Only distribute music that you own or have permission to redistribute. Each
user can keep personal replacement music in their own local ext_music folder.
