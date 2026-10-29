#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Assembles a portable Windows folder from an MSYS2 UCRT64 build:
#   chobi.exe + Qt plugins + every DLL from the MSYS2 prefix that they need.
# Usage (inside the MSYS2 UCRT64 shell): make-portable.sh BUILD_DIR DEST_DIR
#
# This is a development build. Release packages will use a pinned libmpv/FFmpeg built in our
# own CI and ship a corresponding-source archive (see docs/adr/0001-architecture.md).
set -euo pipefail

build="${1:?usage: make-portable.sh BUILD_DIR DEST_DIR}"
dest="${2:?usage: make-portable.sh BUILD_DIR DEST_DIR}"
prefix="${MINGW_PREFIX:-/ucrt64}"

exe="$(find "$build" -name chobi.exe -not -path '*/CMakeFiles/*' | head -1)"
[[ -n "$exe" ]] || { echo "chobi.exe not found under $build" >&2; exit 1; }
echo "using $exe"

rm -rf "$dest"
mkdir -p "$dest"
cp "$exe" "$dest/"

plugins="$(qtpaths6 --query QT_INSTALL_PLUGINS 2>/dev/null || true)"
[[ -z "$plugins" ]] && plugins="$prefix/share/qt6/plugins"
plugins="$(cygpath -u "$plugins")"
for sub in platforms styles imageformats iconengines tls; do
  [[ -d "$plugins/$sub" ]] || continue
  mkdir -p "$dest/$sub"
  case "$sub" in
    platforms) cp "$plugins/$sub/qwindows.dll" "$dest/$sub/" ;;
    imageformats) cp "$plugins/$sub"/{qsvg,qico}.dll "$dest/$sub/" 2>/dev/null || true ;;
    tls) cp "$plugins/$sub/qschannelbackend.dll" "$dest/$sub/" ;;  # HTTPS for update checks
    *) cp "$plugins/$sub"/*.dll "$dest/$sub/" 2>/dev/null || true ;;
  esac
done

# Copy DLL dependencies until the set stops growing.
while :; do
  added=0
  while IFS= read -r dll; do
    name="$(basename "$dll")"
    if [[ ! -f "$dest/$name" ]]; then
      cp "$dll" "$dest/"
      added=1
    fi
  done < <(find "$dest" -name '*.exe' -o -name '*.dll' | while read -r f; do
             ldd "$f" 2>/dev/null | awk -v p="$prefix" 'index($3, p) == 1 { print $3 }'
           done | sort -u)
  [[ $added -eq 0 ]] && break
done

cat > "$dest/README.txt" <<EOF
Chobi development build (not an official release).
Run chobi.exe. Source code and license: https://github.com/dev-taosif-net/chobi
Chobi is GPL-3.0-or-later; bundled libraries keep their own licenses (THIRD_PARTY_NOTICES.md).
EOF
cp "$(dirname "$0")/../../LICENSE" "$dest/LICENSE.txt"
cp "$(dirname "$0")/../../THIRD_PARTY_NOTICES.md" "$dest/"
echo "portable build in $dest ($(find "$dest" -name '*.dll' | wc -l) DLLs)"
