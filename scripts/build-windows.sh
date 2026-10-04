#!/usr/bin/env bash
# Builds the Windows VST3 from WSL and installs it for FL Studio.
#
# The source is mirrored to a folder on the Windows drive (MSVC builds from a
# \\wsl$ path are slow and unreliable), built there with Visual Studio, tested,
# and the plugin is copied into C:\Program Files\Common Files\VST3.
#
#   scripts/build-windows.sh              Release build + tests + install
#   scripts/build-windows.sh --no-install build and test only
#   scripts/build-windows.sh --debug      Debug configuration

set -euo pipefail

config=Release
install=1
for arg in "$@"; do
    case "$arg" in
        --debug)      config=Debug ;;
        --no-install) install=0 ;;
        *) echo "unknown option: $arg" >&2; exit 2 ;;
    esac
done

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
win_home="$(wslpath "$(cmd.exe /c 'echo %USERPROFILE%' 2>/dev/null | tr -d '\r')")"
mirror="${ARPFORGE_WIN_DIR:-$win_home/dev/ArpForge}"
cmake="/mnt/c/Program Files/CMake/bin/cmake.exe"
vst3_dir="/mnt/c/Program Files/Common Files/VST3"

[[ -x "$cmake" ]] || { echo "CMake not found at $cmake" >&2; exit 1; }

echo "==> Mirroring source to $mirror"
mkdir -p "$mirror"
rsync -a --delete \
    --exclude '/build/' --exclude '.git/' --exclude 'tests/engine_tests' \
    "$repo/CMakeLists.txt" "$repo/src" "$repo/resources" "$repo/tests" "$repo/libs" \
    "$mirror/"

cd "$mirror"

if [[ ! -f build/CMakeCache.txt ]]; then
    echo "==> Configuring"
    "$cmake" -S . -B build -G "Visual Studio 17 2022" -A x64
fi

echo "==> Building ($config)"
"$cmake" --build build --config "$config" --parallel -- /nologo /verbosity:minimal

echo "==> Running engine tests"
"./build/$config/EngineTests.exe"

plugin="build/ArpForge_artefacts/$config/VST3/ArpForge.vst3"
[[ -d "$plugin" ]] || { echo "Build output missing: $plugin" >&2; exit 1; }

if [[ $install == 1 ]]; then
    echo "==> Installing to C:\\Program Files\\Common Files\\VST3"
    if ! rsync -a --delete "$plugin/" "$vst3_dir/ArpForge.vst3/"; then
        echo "Install failed. If FL Studio has ArpForge loaded, close FL Studio and run this again." >&2
        exit 1
    fi
fi

echo "==> Done: $(wslpath -w "$mirror/$plugin")"
