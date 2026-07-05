# Perfect Dark Renaissance

Perfect Dark Renaissance v1.1 is an experimental, multiplayer-focused modification of the Perfect Dark PC port. It preserves selected prototypes and expanded Combat Simulator content from an earlier development project while presenting them as an unfinished public build that others can test and extend.

This release may contain glitches, incomplete modes, balance problems, or changes that affect the single-player campaign. Use a new game profile and clean save data when testing it.

## Important legal and compatibility notice

This repository does not include a Perfect Dark ROM. You must provide your own legally obtained US v1.1/NTSC-final ROM.

The required filename is:

```text
pd.ntsc-final.z64
```

The expected MD5 checksum is:

```text
e03b088b6ac9e0080440efed07c1e40f
```

Place the ROM here:

```text
mods/mod_allinone/pd.ntsc-final.z64
```

ROM files, save data, build output, and local configuration files are excluded by `.gitignore` and must not be committed to this repository.

Perfect Dark Renaissance is an unofficial fan project. It is not affiliated with or endorsed by Rare, Nintendo, Microsoft, or their respective rights holders. Perfect Dark and associated game content remain the property of their respective owners.

## Current status

- Primary focus: local multiplayer and Combat Simulator.
- Primary target: English US v1.1/NTSC-final.
- Tested development platform: Apple Silicon macOS.
- Windows and Linux builds use the underlying PC port build system but require platform-specific testing.
- Some custom modes remain experimental.
- Existing Perfect Dark EEPROM or profile data may be incompatible.

## Building on macOS

Install CMake, SDL2, Python 3, and zlib. With Homebrew:

```bash
brew install cmake sdl2 python3 zlib
```

Place your ROM at the path shown above, then run from the repository root:

```bash
./build-normal.sh
```

This script configures an Apple Silicon build, compiles `build/pd.arm64`, and starts it with the included Renaissance mod directories.

## Building on Windows

The upstream port builds on Windows through the MINGW64 or MINGW32 environment supplied by MSYS2. Do not use the plain MSYS shell.

Install the appropriate compiler, CMake, SDL2, zlib, Python 3, Make, and Git packages, then configure and build from the repository root:

```bash
cmake -G "Unix Makefiles" -B build -DROMID=ntsc-final
cmake --build build -j4
```

Run the resulting executable from the repository root with these mod-directory arguments:

```text
--moddir ./mods/mod_allinone
--gexmoddir ./mods/mod_gex
```

Windows packaging and runtime behavior should be verified on an actual Windows system before publishing a Windows release.

## Building on Linux

Install GCC/G++, Make, CMake, Python 3, SDL2, OpenGL development libraries, and zlib, then run:

```bash
cmake -G "Unix Makefiles" -B build -DROMID=ntsc-final
cmake --build build -j4
```

Start the executable using the same mod-directory arguments listed in the Windows section.

## Clean testing

The macOS port stores saves and configuration under:

```text
~/Library/Application Support/perfectdark
```

For a clean first-run test, quit the game and move the following files somewhere safe before launching:

```text
eeprom.bin
mpplayers.bin
mpsetups.bin
pd.ini
```

Older mod-specific configuration files in that directory should also be removed or archived. The game recreates required files with defaults.

## Custom music and sound

Users can provide personal MP3 replacements without rebuilding the game. See [the custom music guide](ext_music/README%20-%20Custom%20Music.txt) for exact filenames and supported events.

Bundled replacement sound effects and implementation notes are documented in [ext_sfx/README.txt](ext_sfx/README.txt). Do not redistribute music or sound files unless you have permission to do so.

If native music is choppy or audio latency is uncomfortable, quit the game and adjust
`BufferSize` or `QueueLimit` under `[Audio]` in `pd.ini`, then restart. Larger queues can
reduce choppiness at the cost of additional latency. The default values are:

```ini
[Audio]
BufferSize=1024
QueueLimit=8192
```

## Project lineage

Perfect Dark Renaissance is built upon:

- the [Perfect Dark decompilation project](https://github.com/n64decomp/perfect_dark);
- the [Perfect Dark PC port](https://github.com/fgsfdsfgs/perfect_dark);
- Perfect Dark All-in-One Mod work by Jonaeru and Atari-Dude;
- GoldenEye X and related community tools and research.

See [CREDITS.md](CREDITS.md) for fuller acknowledgements.

## Licence

The inherited port source is distributed under the MIT licence in [LICENSE](LICENSE). That licence does not grant rights to Nintendo, Rare, Microsoft, Perfect Dark, GoldenEye, or other third-party game assets and trademarks. See [NOTICE.md](NOTICE.md).
