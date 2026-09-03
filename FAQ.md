# FAQ — Common Build Errors and Solutions

## CMake / Configure Phase

### "Qt kit not found" or "Could NOT find a package configuration file provided by Qt6"

CMake can't find your Qt installation.

**Fix:** Set the path explicitly before running `run.ps1`:

```powershell
# Windows — adjust the version/path to match your install
$env:CMAKE_PREFIX_PATH = "D:\Qt\6.11.1\mingw_64"
```

```bash
# Linux
export CMAKE_PREFIX_PATH="$HOME/Qt/6.11.1/gcc_64"
```

If you installed Qt via the Online Installer, check `D:\Qt` or `C:\Qt` for a folder like `6.x.y\mingw_64`.

---

### "MinGW not found" / "g++.exe not found"

The build needs MinGW 13 (GCC compiler) and it's either not installed or not on PATH.

**Fix (Windows):**

```powershell
# Install via aqtinstall
aqt install-tool windows desktop tools_mingw1310 --outputdir D:\Qt
```

Or rerun `.\bootstrap.ps1` which handles this.

---

### "Ninja not found" / "CMAKE_MAKE_PROGRAM is not set"

Ninja (the build tool) isn't installed or isn't on PATH.

**Fix:**

```powershell
# Windows (winget)
winget install Ninja-build.Ninja

# Or via chocolatey
choco install ninja
```

```bash
# Linux
sudo apt install ninja-build
```

After installing, restart your terminal so PATH refreshes.

---

### "cmake is not recognized" / "cmake: command not found"

CMake isn't installed or isn't on PATH.

**Fix:**

```powershell
# Windows
winget install Kitware.CMake
```

```bash
# Linux
sudo apt install cmake
```

Restart terminal after install.

---

### CMake picks the wrong Qt version (e.g. 6.8.3 instead of 6.11.1)

If you have multiple Qt versions installed, CMake might find an older one first.

**Fix:** Explicitly set the prefix path to the version you want:

```powershell
$env:CMAKE_PREFIX_PATH = "D:\Qt\6.11.1\mingw_64"
.\run.ps1 --rebuild
```

The `--rebuild` flag ensures a clean configure with the new path.

---

### "Could NOT find WrapVulkanHeaders"

This is a warning, not an error. The Vulkan SDK is optional. The build continues without it and shaders still compile fine.

**No fix needed.** Just ignore it.

---

## Build Phase (Compilation)

### "cc1plus.exe: out of memory allocating X bytes"

The compiler ran out of memory. This happens on machines with limited RAM when running too many parallel compile jobs.

**Fix:** Reduce parallelism:

```powershell
$env:NUMBER_OF_PROCESSORS = "2"
.\run.ps1
```

Or directly:

```powershell
cmake --build build-win --target arachnel_app -j 2
```

Close other heavy programs (browser, IDE) during compilation.

---

### "error: 'SomeType' does not name a type" or "was not declared in this scope"

Usually means a header include is missing or the source file list in `src/CMakeLists.txt` is incomplete.

**Fix:**
1. Make sure you pulled the latest code: `git pull`
2. Clean rebuild: `.\run.ps1 --rebuild`
3. If the error mentions a file you modified, double-check your includes at the top of that file.

---

### Build fails in QmlMaterial (qmltyperegistrar, shader compilation)

The QmlMaterial dependency (UI framework) can fail if:
- Git LFS didn't pull font files
- Qt version is too old (needs 6.8+)
- Shader tools are missing

**Fix:**

```bash
git lfs install
git lfs pull
```

If using Qt < 6.8, upgrade. The project requires Qt 6.8 minimum.

---

### "spirv-opt not found" or shader-related errors

The Windows build patches out Vulkan SDK requirements automatically. If you still see these:

**Fix:**

```powershell
.\run.ps1 --rebuild
```

A clean rebuild re-applies the patch to QmlMaterial's CMakeLists.txt.

---

## Runtime Errors

### App starts but shows a blank/white screen

QML modules weren't found at runtime.

**Fix:** Make sure `qml_modules/` exists in your build directory and contains `Qcm/Material/`. On dev builds, `run.ps1` sets the environment automatically. If running the exe directly:

```powershell
$env:QT_QML_MATERIAL_IMPORT_PATH = "D:\MoneyMaking\build-win\qml_modules"
$env:QML2_IMPORT_PATH = "D:\MoneyMaking\build-win\qml_modules"
.\build-win\arachnel_app.exe
```

---

### "SteamCMD download failed" or "SteamCMD not found"

The bundled Steam plugin auto-downloads SteamCMD on first use. If that fails:

**Fix (manual install):**

Windows:
1. Download from https://steamcdn-a.akamaihd.net/client/installer/steamcmd.zip
2. Extract to `%APPDATA%\Arachnel\steamcmd\`
3. Restart the app

Linux:
```bash
mkdir -p ~/.local/share/Arachnel/steamcmd
cd ~/.local/share/Arachnel/steamcmd
curl -sqL https://steamcdn-a.akamaihd.net/client/installer/steamcmd_linux.tar.gz | tar xzf -
```

Or install from your package manager (`sudo apt install steamcmd` on Debian/Ubuntu).

---

### "Failed to start HTTP download"

Network issue. Check:
1. Internet connection is working
2. Firewall isn't blocking the app
3. The download URL is still valid (source might be down)

---

### App crashes on startup (Windows)

Check `%APPDATA%\Arachnel\crash.log` for details. Common causes:
- Missing Qt DLLs — use `run.ps1` which deploys them automatically
- Corrupted settings — delete `%APPDATA%\Arachnel\settings.json` and restart

---

## Git / Source Control

### "git lfs" errors or "Smudge error" or font files are 130 bytes

Binary assets (fonts, icons) are stored in Git LFS. If they show as tiny text files:

**Fix:**

```bash
git lfs install
git lfs pull
```

If LFS wasn't installed before cloning, you may need to re-clone:

```bash
git lfs install
git clone https://github.com/BadKiko/Arachnel.git
```

---

### "Generator mismatch" on rebuild

If you switch between MinGW and MSVC (or vice versa), the build directory has stale state.

**Fix:**

```powershell
.\run.ps1 --rebuild
```

This deletes `build-win/` and starts fresh.

---

## Linux-Specific

### AppImage won't start: "FUSE not found" or "cannot execute binary file"

AppImages need FUSE to mount themselves.

**Fix:**

```bash
# Ubuntu/Debian
sudo apt install fuse libfuse2

# Then make executable and run
chmod +x Arachnel-*.AppImage
./Arachnel-*.AppImage
```

---

### "libQt6*.so not found" when running the built binary directly

The runtime can't find Qt shared libraries.

**Fix:** Either:
- Use the AppImage (bundles everything)
- Set `LD_LIBRARY_PATH` to your Qt lib dir:

```bash
export LD_LIBRARY_PATH="$HOME/Qt/6.11.1/gcc_64/lib:$LD_LIBRARY_PATH"
./arachnel_app
```

---

## Still stuck?

Open an issue at https://github.com/BadKiko/Arachnel/issues with:
1. Your OS and version
2. Qt version installed
3. The full error output (last 30 lines minimum)
4. What you ran (exact command)
