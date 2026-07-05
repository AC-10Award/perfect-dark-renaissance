#!/bin/zsh

set -euo pipefail

project_root="$(cd "$(dirname "$0")/.." && pwd)"
app_dir="$project_root/PERFECT DARK RENAISSANCE.app"
launcher="$app_dir/Contents/MacOS/pd-renaissance-launcher"
bundle_binary="$app_dir/Contents/MacOS/pd.arm64"
source_file="$project_root/tools/macos_app_launcher.c"

clang \
	-O2 \
	-framework CoreFoundation \
	-o "$launcher" \
	"$source_file"

ln -sfn ../../../build/pd.arm64 "$bundle_binary"

echo "App bundle ready:"
echo "$app_dir"
