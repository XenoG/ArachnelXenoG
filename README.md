<div align="center">

<img src="resources/icons/arachnel-github.svg" width="128" alt="Arachnel logo" />

<h1>Arachnel</h1>

<p>
  <a href="https://hosted.weblate.org/engage/arachnel/">
    <img src="https://hosted.weblate.org/widget/arachnel/application/svg-badge.svg" alt="Translation status">
  </a>
</p>

<br>

<img src="docs/readme-carousel.svg" width="960" alt="Arachnel UI previews">

</div>

<br>

<img src="images/demo.gif" width="800" alt="Arachnel demo — catalog, downloads, library">

## What is Arachnel?

Game launcher for Windows and Linux. Pick a game, download it, hit Play. On Windows it just runs. On Linux it grabs what it needs (Proton and that kind of stuff) so you don't have to mess with the OS.

## Quickest way to test

### Download a release (no build required)

Go to [Releases](https://github.com/BadKiko/Arachnel/releases) and grab the latest build:

- **Windows** — `Arachnel-*-Setup.exe` (installer, just run it)
- **Linux** — `Arachnel-*-x86_64.AppImage` (make executable and run)

```bash
# Linux example
chmod +x Arachnel-*-x86_64.AppImage
./Arachnel-*-x86_64.AppImage
```

That's it. No dependencies, no build tools.

### Trigger a CI build yourself

If you want to build from the latest commit without setting up a local environment:

1. Fork the repo (or have push access)
2. Go to **Actions** → **Release** → **Run workflow**
3. Enter a version (e.g. `0.1.40`) and check "pre-release"
4. CI builds both platforms and publishes a GitHub Release with the binaries

Or from the command line:

```bash
gh workflow run release.yml -f version="0.1.40" -f prerelease=true
```

### Build locally

If you want to build from source on your machine:

**Prerequisites (Windows):**

Run the bootstrap script (installs CMake, Ninja, Qt, MinGW):

```powershell
.\bootstrap.ps1
```

Then build:

```powershell
.\run.ps1
```

For a release package:

```powershell
$env:ARACHNEL_VERSION = "0.1.40"
$env:BUILD_TYPE = "Release"
$env:ARACHNEL_FAST_BUILD = "0"
.\run.ps1 --package
```

**Prerequisites (Linux):**

Install system deps (Ubuntu/Debian):

```bash
sudo apt-get install -y git git-lfs cmake ninja-build \
  qt6-base-dev qt6-declarative-dev qt6-shadertools-dev qt6-multimedia-dev \
  libgl1-mesa-dev libxkbcommon-dev
```

Then build:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

## Links

- [Releases](https://github.com/BadKiko/Arachnel/releases) — download builds
- [Issues](https://github.com/BadKiko/Arachnel/issues) — bugs and feature requests
- [TUTORIAL.md](TUTORIAL.md) — detailed local build guide for beginners
- [FAQ.md](FAQ.md) — common build errors and how to fix them
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — codebase overview
