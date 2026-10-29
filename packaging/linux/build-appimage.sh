#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Builds Chobi-<version>-x86_64.AppImage from a configured+built CMake tree.
# Usage: build-appimage.sh BUILD_DIR OUT_DIR
#
# Build on the oldest distribution you want to support: the AppImage needs at least that
# glibc. CI uses Debian 13 "trixie" (glibc 2.41, Qt 6.8, mpv 0.40).
#
# The GPU/driver stack (libva, Vulkan loader, libdrm, gbm, Mesa, libGL/EGL/OpenGL) is never
# bundled: it must match the user's kernel and drivers. Like every AppImage, Chobi also relies on
# linuxdeploy's excludelist of libraries every desktop has (fontconfig, freetype, HarfBuzz,
# FriBidi, ALSA, PipeWire, X11/xcb, libxkbcommon, D-Bus, libusb); docs/building.md lists them.
set -euo pipefail

build="$(cd "${1:?usage: build-appimage.sh BUILD_DIR OUT_DIR}" && pwd)"
out="${2:?usage: build-appimage.sh BUILD_DIR OUT_DIR}"
mkdir -p "$out"
out="$(cd "$out" && pwd)"
tools="${CHOBI_TOOLS_DIR:-$build/appimage-tools}"

# Pinned tool releases, verified by SHA-256.
LINUXDEPLOY_VERSION=1-alpha-20251107-1
LINUXDEPLOY_SHA256=c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d
PLUGIN_QT_VERSION=1-alpha-20250213-1
PLUGIN_QT_SHA256=15106be885c1c48a021198e7e1e9a48ce9d02a86dd0a1848f00bdbf3c1c92724

fetch() {  # fetch URL SHA256 DEST
  local url="$1" sha="$2" dest="$3"
  if [[ ! -f "$dest" ]] || ! echo "$sha  $dest" | sha256sum -c --quiet 2>/dev/null; then
    curl -fsSL --retry 3 -o "$dest" "$url"
    echo "$sha  $dest" | sha256sum -c --quiet
  fi
  chmod +x "$dest"
}

mkdir -p "$tools"
fetch "https://github.com/linuxdeploy/linuxdeploy/releases/download/$LINUXDEPLOY_VERSION/linuxdeploy-x86_64.AppImage" \
  "$LINUXDEPLOY_SHA256" "$tools/linuxdeploy-x86_64.AppImage"
fetch "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/$PLUGIN_QT_VERSION/linuxdeploy-plugin-qt-x86_64.AppImage" \
  "$PLUGIN_QT_SHA256" "$tools/linuxdeploy-plugin-qt-x86_64.AppImage"

version="$("$build/src/chobi" --version | awk '{print $2}')"
app_id=io.github.dev_taosif_net.Chobi
appdir="$build/AppDir"
rm -rf "$appdir"
DESTDIR="$appdir" cmake --install "$build" --prefix /usr >/dev/null

export PATH="$tools:$PATH"
export APPIMAGE_EXTRACT_AND_RUN=1  # containers have no FUSE
export QMAKE="${QMAKE:-$(command -v qmake6 || echo /usr/lib/qt6/bin/qmake)}"
export EXTRA_PLATFORM_PLUGINS="libqwayland-egl.so;libqwayland-generic.so"
export EXTRA_QT_MODULES="waylandclient"
export LDAI_OUTPUT="$out/Chobi-$version-x86_64.AppImage"
export OUTPUT="$LDAI_OUTPUT"

excludes=()
for lib in 'libva.so*' 'libva-*.so*' 'libvulkan.so*' 'libdrm.so*' 'libdrm_*.so*' 'libgbm.so*' \
           'libGL*.so*' 'libEGL*.so*' 'libOpenGL.so*' 'libglapi.so*'; do
  excludes+=(--exclude-library "$lib")
done

# linuxdeploy's excludelist also skips libjack (it must match a JACK server). Chobi never uses
# JACK, but FFmpeg links it, and many desktops don't have it: bundle it so the app always starts.
includes=()
for lib in /usr/lib/x86_64-linux-gnu/libjack.so.0; do
  [[ -f "$lib" ]] && includes+=(--library "$lib")
done

# Pass 1: application, Qt modules and their plugins.
linuxdeploy-x86_64.AppImage \
  --appdir "$appdir" \
  --plugin qt \
  "${includes[@]}" \
  --desktop-file "$appdir/usr/share/applications/$app_id.desktop" \
  --icon-file "$appdir/usr/share/icons/hicolor/scalable/apps/$app_id.svg" \
  "${excludes[@]}"

# The Qt plugin bundles QtWaylandClient but not the plugin folders Wayland needs at runtime
# (without xdg-shell Qt silently falls back to XWayland). Also add the XDG portal theme for
# native file dialogs on GNOME/KDE without pulling GTK into the AppImage.
qt_plugins="$("$QMAKE" -query QT_INSTALL_PLUGINS)"
extra=()
for dir in wayland-shell-integration wayland-decoration-client wayland-graphics-integration-client; do
  if [[ -d "$qt_plugins/$dir" ]]; then
    mkdir -p "$appdir/usr/plugins/$dir"
    cp "$qt_plugins/$dir"/*.so "$appdir/usr/plugins/$dir/"
    extra+=(--deploy-deps-only "$appdir/usr/plugins/$dir")
  fi
done
if [[ -f "$qt_plugins/platformthemes/libqxdgdesktopportal.so" ]]; then
  mkdir -p "$appdir/usr/plugins/platformthemes"
  cp "$qt_plugins/platformthemes/libqxdgdesktopportal.so" "$appdir/usr/plugins/platformthemes/"
  extra+=(--deploy-deps-only "$appdir/usr/plugins/platformthemes")
fi
mkdir -p "$appdir/apprun-hooks"
cat > "$appdir/apprun-hooks/chobi-env.sh" <<'HOOK'
# Native file dialogs via xdg-desktop-portal unless the user chose a theme.
export QT_QPA_PLATFORMTHEME="${QT_QPA_PLATFORMTHEME:-xdgdesktopportal}"
HOOK

# Pass 2: dependencies of the extra plugins, then the AppImage itself.
linuxdeploy-x86_64.AppImage \
  --appdir "$appdir" \
  "${extra[@]}" \
  --output appimage \
  "${excludes[@]}"

chmod +x "$LDAI_OUTPUT"
echo "built $LDAI_OUTPUT"
