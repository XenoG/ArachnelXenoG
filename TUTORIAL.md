# Building Arachnel Locally — Step by Step

This guide walks you through building Arachnel from source on your own machine. It assumes you're starting from scratch with no development tools installed.

If you just want to run the app, grab a pre-built binary from [Releases](https://github.com/BadKiko/Arachnel/releases) instead.

---

## Table of Contents

- [Windows](#windows)
  - [What you need](#what-you-need-windows)
  - [Step 1: Install prerequisites automatically](#step-1-install-prerequisites-automatically)
  - [Step 2: Build and run](#step-2-build-and-run)
  - [Step 3: Create a release package](#step-3-create-a-release-package-windows)
  - [Manual setup (if bootstrap fails)](#manual-setup-if-bootstrap-fails)
- [Linux](#linux)
  - [What you need](#what-you-need-linux)
  - [Step 1: Install system packages](#step-1-install-system-packages)
  - [Step 2: Install Qt](#step-2-install-qt)
  - [Step 3: Build and run](#step-3-build-and-run-linux)
  - [Step 4: Create an AppImage](#step-4-create-an-appimage)
- [Troubleshooting](#troubleshooting)

---

## Windows

### What you need (Windows)

The build system uses:

| Tool | Version | Purpose |
|------|---------|---------|
| CMake | 3.20+ | Build system generator |
| Ninja | any | Fast parallel builds |
| MinGW | 13.1.0 | C++ compiler (GCC) |
| Qt | 6.11.1 | UI framework (mingw_64 kit) |
| Git | any | Source control + LFS |
| Python | 3.10+ | For aqtinstall (Qt downloader) |

You don't need to install these manually — the bootstrap script handles everything.

### Step 1: Install prerequisites automatically

1. Open PowerShell **as Administrator** (right-click → Run as administrator)

2. Clone the repo:
   ```powershell
   git clone https://github.com/BadKiko/Arachnel.git
   cd Arachnel
   ```

3. Run the bootstrap script:
   ```powershell
   .\bootstrap.ps1
   ```

   This installs CMake, Ninja, Python, aqtinstall, Qt 6.11.1, and MinGW 13. Everything goes into `D:\Qt` by default. It takes 10-20 minutes on a fresh machine (mostly downloading Qt).

4. **Close and reopen your terminal** after bootstrap finishes (PATH needs to refresh).

### Step 2: Build and run

From a normal (non-admin) PowerShell in the project folder:

```powershell
.\run.ps1
```

That's it. The script will:
- Find your Qt installation automatically
- Run CMake configure (first time takes a few minutes — downloads Boost and libtorrent)
- Build the app
- Launch it

The first build downloads dependencies (~500MB for Boost headers + libtorrent), so give it time. Subsequent builds are fast (incremental).

#### What `run.ps1` options are available?

```
.\run.ps1              → configure + build + run
.\run.ps1 --rebuild   → clean everything, then build + run
.\run.ps1 --run       → just run (skip build, exe must already exist)
.\run.ps1 --release   → build in Release mode (optimized, no debug info)
.\run.ps1 --package   → build Release + create dist-win folder + ZIP
.\run.ps1 --installer → build the Inno Setup installer (Arachnel-Setup.exe)
```

### Step 3: Create a release package (Windows)

To build a distributable package:

```powershell
$env:ARACHNEL_VERSION = "0.1.40"
$env:BUILD_TYPE = "Release"
$env:ARACHNEL_FAST_BUILD = "0"
.\run.ps1 --package
```

This produces:
- `dist-win/` — folder with the app + all Qt DLLs + runtime
- `Arachnel-win64-Release.zip` — ready to share

For the full installer (`.exe` with UI):
```powershell
.\run.ps1 --installer
```

This creates `Arachnel-Setup.exe` in the project root.

### Manual setup (if bootstrap fails)

If `bootstrap.ps1` doesn't work for your system, install these manually:

1. **CMake**: Download from https://cmake.org/download/ and add to PATH
2. **Ninja**: Download from https://ninja-build.org/ and add to PATH
3. **Python 3.12**: Download from https://python.org (check "Add to PATH" during install)
4. **Qt 6.11.1**:
   ```powershell
   pip install aqtinstall
   aqt install-qt windows desktop 6.11.1 win64_mingw --outputdir D:\Qt --modules qtshadertools qtmultimedia
   aqt install-tool windows desktop tools_mingw1310 --outputdir D:\Qt
   ```
5. **Git LFS**: 
   ```powershell
   git lfs install
   ```

Then set the environment so `run.ps1` can find Qt:
```powershell
$env:CMAKE_PREFIX_PATH = "D:\Qt\6.11.1\mingw_64"
.\run.ps1
```

---

## Linux

### What you need (Linux)

| Tool | Version | Purpose |
|------|---------|---------|
| CMake | 3.20+ | Build system generator |
| Ninja | any | Fast parallel builds |
| GCC | 11+ | C++20 compiler |
| Qt | 6.11.1 | UI framework |
| Git + LFS | any | Source control |

### Step 1: Install system packages

**Ubuntu 22.04+ / Debian 12+:**

```bash
sudo apt-get update
sudo apt-get install -y \
  git git-lfs cmake ninja-build g++ pkg-config \
  libgl1-mesa-dev libglib2.0-dev libxkbcommon-dev \
  libxcb-cursor-dev libxcb-icccm4-dev libxcb-image0-dev \
  libxcb-keysyms1-dev libxcb-render-util0-dev libxcb-shape0-dev \
  libxcb-xfixes0-dev libxcb-xinerama0-dev libxcb-xinput-dev \
  libdbus-1-dev libpulse-dev patchelf fuse libfuse2 file curl
```

**Fedora 38+:**

```bash
sudo dnf install -y \
  git git-lfs cmake ninja-build gcc-c++ pkg-config \
  mesa-libGL-devel glib2-devel libxkbcommon-devel \
  xcb-util-cursor-devel xcb-util-image-devel xcb-util-keysyms-devel \
  xcb-util-renderutil-devel libXfixes-devel dbus-devel pulseaudio-libs-devel \
  patchelf fuse fuse-libs file curl
```

**Arch Linux:**

```bash
sudo pacman -S --needed \
  git git-lfs cmake ninja gcc pkgconf \
  mesa glib2 libxkbcommon xcb-util-cursor xcb-util-image \
  xcb-util-keysyms xcb-util-renderutil libxfixes dbus libpulse \
  patchelf fuse2 file curl
```

### Step 2: Install Qt

The easiest way is using `aqtinstall`:

```bash
pip install aqtinstall

# Install Qt 6.11.1 with required modules
aqt install-qt linux desktop 6.11.1 linux_gcc_64 \
  --outputdir ~/Qt \
  --modules qtshadertools qtmultimedia
```

Then export the path:

```bash
export CMAKE_PREFIX_PATH="$HOME/Qt/6.11.1/gcc_64"
```

Add that line to your `~/.bashrc` or `~/.zshrc` so you don't have to repeat it.

Alternatively, if your distro ships Qt 6.8+, you can use system packages (e.g. `qt6-base-dev qt6-declarative-dev qt6-shadertools-dev qt6-multimedia-dev` on Ubuntu). But version 6.11.1 is recommended for full compatibility.

### Step 3: Build and run (Linux)

```bash
git clone https://github.com/BadKiko/Arachnel.git
cd Arachnel
git lfs pull

mkdir build && cd build

cmake .. \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH}" \
  -DARACHNEL_VERSION="dev"

cmake --build . --target arachnel_app -j$(nproc)
```

Run the app:

```bash
# Set up QML paths so the app can find Qcm.Material
export QT_QML_MATERIAL_IMPORT_PATH="$(pwd)/qml_modules"
export QML2_IMPORT_PATH="$(pwd)/qml_modules"

./arachnel_app
```

### Step 4: Create an AppImage

To create a portable AppImage that runs on any Linux distro:

```bash
cd /path/to/Arachnel   # project root

export ARACHNEL_VERSION="0.1.40"
export CMAKE_PREFIX_PATH="$HOME/Qt/6.11.1/gcc_64"

bash scripts/ci/package-appimage.sh
```

The output lands in `dist-linux/Arachnel-0.1.40-x86_64.AppImage`. You can share this file — it runs anywhere without installing dependencies:

```bash
chmod +x dist-linux/Arachnel-0.1.40-x86_64.AppImage
./dist-linux/Arachnel-0.1.40-x86_64.AppImage
```

---

## Troubleshooting

### "Qt kit not found" or "Qt6Config.cmake not found"

The build can't find your Qt installation. Set the path explicitly:

```powershell
# Windows
$env:CMAKE_PREFIX_PATH = "D:\Qt\6.11.1\mingw_64"
```

```bash
# Linux
export CMAKE_PREFIX_PATH="$HOME/Qt/6.11.1/gcc_64"
```

### "MinGW not found" (Windows)

MinGW must be installed alongside Qt. Verify it exists:

```powershell
Test-Path "D:\Qt\Tools\mingw1310_64\bin\g++.exe"
```

If missing, reinstall:
```powershell
aqt install-tool windows desktop tools_mingw1310 --outputdir D:\Qt
```

### First build takes forever

Normal. The first build downloads:
- Boost headers (~130MB compressed)
- libtorrent source (~5MB)
- QmlMaterial (~20MB)

These are cached in `build-win/_deps/` (Windows) or `build/_deps/` (Linux). Subsequent builds skip the download.

### "git lfs" errors or missing font files

The project uses Git LFS for binary assets. Make sure it's installed:

```bash
git lfs install
git lfs pull
```

### Build fails with "spirv-opt not found" or shader errors

On Windows, the build patches out Vulkan SDK requirements automatically. If you still see shader-related errors, the patch may not have applied. Try a clean rebuild:

```powershell
.\run.ps1 --rebuild
```

### App starts but shows blank/white screen

Usually means QML modules weren't found. Check that `qml_modules/` exists in your build directory and contains `Qcm/Material/`. On Linux, make sure the environment variables are set:

```bash
export QT_QML_MATERIAL_IMPORT_PATH="$(pwd)/qml_modules"
export QML2_IMPORT_PATH="$(pwd)/qml_modules"
```

### libtorrent shared DLL migration

If you previously built with static libtorrent and switch to shared (the default), you might see link errors. The build handles this automatically, but if it fails:

```powershell
.\run.ps1 --rebuild
```

### Using a compile cache (optional, speeds up rebuilds)

Install `sccache` for faster rebuilds:

```powershell
# Windows (via cargo or scoop)
scoop install sccache

# Linux
cargo install sccache
# or: apt install sccache
```

The build scripts detect and use it automatically.
