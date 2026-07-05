#!/bin/zsh

set -euo pipefail

project_root="$(cd "$(dirname "$0")" && pwd)"
build_dir="$project_root/build"

find_cmake() {
  if [[ -n "${CMAKE_BIN:-}" && -x "$CMAKE_BIN" ]]; then
    printf '%s\n' "$CMAKE_BIN"
    return 0
  fi

  local candidates=(
    "/Applications/CMake.app/Contents/bin/cmake"
    "$HOME/Library/Python/3.9/bin/cmake"
    "/opt/homebrew/bin/cmake"
    "/usr/local/bin/cmake"
  )

  local candidate
  for candidate in "${candidates[@]}"; do
    if [[ -x "$candidate" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done

  if command -v cmake >/dev/null 2>&1; then
    command -v cmake
    return 0
  fi

  return 1
}

cmake_bin="$(find_cmake || true)"

if [[ -z "$cmake_bin" ]]; then
  echo "Could not find cmake."
  echo "Install CMake, or run with CMAKE_BIN=/full/path/to/cmake ./build-normal.sh"
  exit 1
fi

cd "$project_root"

if [[ -f "$build_dir/CMakeCache.txt" ]] && ! grep -q "CMAKE_HOME_DIRECTORY:INTERNAL=$project_root" "$build_dir/CMakeCache.txt"; then
  archived_build_dir="$project_root/build.pre-migration-$(date +%Y%m%d-%H%M%S)"
  echo "Archiving stale CMake build cache to: $archived_build_dir"
  mv "$build_dir" "$archived_build_dir"
fi

"$cmake_bin" -G"Unix Makefiles" -B"$build_dir" -DCMAKE_OSX_ARCHITECTURES=arm64 -DROMID=ntsc-final
"$cmake_bin" --build "$build_dir" --target pd -j4 --clean-first

"$build_dir/pd.arm64" \
  --moddir ./mods/mod_allinone \
  --gexmoddir ./mods/mod_gex \
  --kakarikomoddir ./mods/mod_kakariko \
  --darknoonmoddir ./mods/mod_dark_noon \
  --goldfinger64moddir ./mods/mod_goldfinger_64
