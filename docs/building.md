# Building Chobi

Requirements: a C++17 compiler, CMake ≥ 3.21, Ninja (recommended), pkg-config, Qt ≥ 6.5
(Core, Gui, Widgets, OpenGL, OpenGLWidgets, Test), libmpv ≥ 0.38 (client API 2.x). `ffmpeg` and
`bash` are needed only to generate test media.

## Linux

### Ubuntu 26.04 / Debian testing

```bash
sudo apt install build-essential cmake ninja-build pkg-config git \
                 qt6-base-dev qt6-wayland libmpv-dev ffmpeg fonts-noto-core
```

### Fedora

```bash
sudo dnf install gcc-c++ cmake ninja-build pkgconf-pkg-config qt6-qtbase-devel \
                 qt6-qtwayland mpv-libs-devel ffmpeg-free google-noto-sans-bengali-fonts
```
(RPM Fusion's `mpv-libs-devel`/`ffmpeg` give wider codec coverage than `ffmpeg-free`.)

### Arch

```bash
sudo pacman -S base-devel cmake ninja qt6-base qt6-wayland mpv ffmpeg noto-fonts
```

### Build, test, run, install

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/src/chobi video.mkv

sudo cmake --install build           # /usr/local: binary, .desktop (Open With), icon, metainfo
```

Options: `-DCHOBI_BUILD_TESTS=OFF`, `-DCHOBI_SANITIZE=ON` (ASan + UBSan),
`-DCHOBI_ENABLE_UPDATER=OFF` (distribution packages: updates come from the package manager).
Builds without a release key in `packaging/update-signing-key.pub` never check for updates.

Runtime environment variables: `CHOBI_SUBTITLE_LANGS=bn,en` (subtitle language preference,
default: system UI languages, then English), `CHOBI_MPV_LOG=1` (mpv log to stderr),
`CHOBI_BENCH=1` (startup timeline).

## Windows (MSYS2 UCRT64)

1. Install [MSYS2](https://www.msys2.org) and open the **UCRT64** shell.
2. Install the toolchain and libraries:
   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
     mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf \
     mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-mpv mingw-w64-ucrt-x86_64-ffmpeg git
   ```
3. Build and test:
   ```bash
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ctest --test-dir build --output-on-failure
   ```
4. Make a portable folder you can copy to any Windows 10/11 PC:
   ```bash
   bash packaging/windows/make-portable.sh build dist/chobi-windows-x64
   ```
   `dist/chobi-windows-x64/chobi.exe` runs without MSYS2 installed.

CI does exactly this on every push and uploads the folder as the `chobi-windows-x64-dev`
artifact. Tagged releases additionally build the installer from `packaging/windows/chobi.iss`
(see [releasing.md](releasing.md)).

## Packaging

| Channel | How | Self-update |
|---|---|---|
| Linux AppImage | `packaging/linux/build-appimage.sh` (CI builds it on Debian 13) | ✅ |
| Windows installer | `packaging/windows/chobi.iss` (Inno Setup, per user) | ✅ |
| Windows portable zip | `packaging/windows/make-portable.sh` | download link |
| `cmake --install` / distro packages | standard | via the package manager |
| Flatpak (Flathub) | planned | via Flathub |

### AppImage runtime requirements

The AppImage bundles Qt, libmpv, FFmpeg and their dependencies, but by design uses the system's
graphics and desktop libraries: OpenGL/EGL (libglvnd), libva, the Vulkan loader, libdrm, gbm,
Wayland client, fontconfig, FreeType, HarfBuzz, FriBidi, ALSA, PipeWire, X11/xcb (incl. SM/ICE),
libxkbcommon, D-Bus and libusb. Every desktop installation has these. The release workflow checks
that the AppImage starts on Ubuntu 26.04, Fedora 43 and Arch with only those installed.
