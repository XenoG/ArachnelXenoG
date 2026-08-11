#Requires -Version 5.1
<#
.SYNOPSIS
    Installs all Arachnel build prerequisites on a fresh Windows machine.

.DESCRIPTION
    Installs: CMake, Ninja, Python 3, aqtinstall, Qt 6.11.1 (mingw_64 kit),
    and MinGW 13.1.0 toolchain. Everything lands under D:\Qt by default.

    After running this script, run.ps1 will auto-discover all tools.

.NOTES
    Requires winget and an internet connection.
    Run from an elevated (Administrator) PowerShell for winget installs,
    or accept UAC prompts.
#>

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$QT_VERSION = "6.11.1"
$QT_INSTALL_DIR = "D:\Qt"
$QT_ARCH = "win64_mingw"
$QT_MODULES = @("qtshadertools", "qtmultimedia")
$MINGW_TOOL = "tools_mingw1310"

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Arachnel Build Prerequisites Installer" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "This will install:"
Write-Host "  - CMake (via winget)"
Write-Host "  - Ninja (via winget)"
Write-Host "  - Python 3 (via winget)"
Write-Host "  - aqtinstall (via pip)"
Write-Host "  - Qt $QT_VERSION mingw_64 kit (to $QT_INSTALL_DIR)"
Write-Host "  - MinGW 13.1.0 toolchain (to $QT_INSTALL_DIR\Tools)"
Write-Host ""

# --- Step 1: CMake ---
Write-Host "[1/6] Installing CMake..." -ForegroundColor Yellow
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmake) {
    Write-Host "  CMake already installed: $($cmake.Source)" -ForegroundColor Green
} else {
    winget install Kitware.CMake --source winget --accept-package-agreements --accept-source-agreements --silent
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "winget install CMake returned $LASTEXITCODE (may still be OK if already installed)"
    }
    # Refresh PATH
    $env:Path = [Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [Environment]::GetEnvironmentVariable("Path", "User")
    $cmake = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmake) {
        Write-Host "  CMake installed: $($cmake.Source)" -ForegroundColor Green
    } else {
        Write-Host "  CMake installed but not on PATH yet. Restart your terminal after this script." -ForegroundColor Yellow
    }
}

# --- Step 2: Ninja ---
Write-Host "[2/6] Installing Ninja..." -ForegroundColor Yellow
$ninja = Get-Command ninja -ErrorAction SilentlyContinue
if ($ninja) {
    Write-Host "  Ninja already installed: $($ninja.Source)" -ForegroundColor Green
} else {
    winget install Ninja-build.Ninja --source winget --accept-package-agreements --accept-source-agreements --silent
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "winget install Ninja returned $LASTEXITCODE"
    }
    $env:Path = [Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [Environment]::GetEnvironmentVariable("Path", "User")
    $ninja = Get-Command ninja -ErrorAction SilentlyContinue
    if ($ninja) {
        Write-Host "  Ninja installed: $($ninja.Source)" -ForegroundColor Green
    } else {
        Write-Host "  Ninja installed but not on PATH yet. Restart your terminal after this script." -ForegroundColor Yellow
    }
}

# --- Step 3: Python ---
Write-Host "[3/6] Installing Python 3..." -ForegroundColor Yellow
$hasPython = $false
try {
    $pyOut = & python --version 2>&1 | Out-String
    if ($pyOut -match "Python 3\.(\d+)") {
        Write-Host "  Python already installed: $($pyOut.Trim())" -ForegroundColor Green
        $hasPython = $true
    }
} catch {}
if (-not $hasPython) {
    # Disable the Windows Store app execution alias for python (causes false detection)
    winget install Python.Python.3.12 --source winget --accept-package-agreements --accept-source-agreements --silent
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "winget install Python returned $LASTEXITCODE"
    }
    $env:Path = [Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [Environment]::GetEnvironmentVariable("Path", "User")
    # Python installs to a user-local or program files path; find it
    $pyPaths = @(
        "$env:LOCALAPPDATA\Programs\Python\Python312\python.exe",
        "$env:LOCALAPPDATA\Programs\Python\Python311\python.exe",
        "${env:ProgramFiles}\Python312\python.exe",
        "${env:ProgramFiles}\Python311\python.exe"
    )
    $foundPy = $pyPaths | Where-Object { Test-Path $_ } | Select-Object -First 1
    if ($foundPy) {
        $pyDir = Split-Path -Parent $foundPy
        $env:Path = "$pyDir;$pyDir\Scripts;$env:Path"
        Write-Host "  Python installed: $(& $foundPy --version 2>&1)" -ForegroundColor Green
    } else {
        # Try the generic command again
        try {
            $pyOut = & python --version 2>&1 | Out-String
            if ($pyOut -match "Python 3") {
                Write-Host "  Python installed: $($pyOut.Trim())" -ForegroundColor Green
            } else {
                throw "not found"
            }
        } catch {
            throw "Python installation failed. Please install Python 3.12+ manually and re-run."
        }
    }
}

# --- Step 4: aqtinstall ---
Write-Host "[4/6] Installing aqtinstall (pip)..." -ForegroundColor Yellow
$aqt = Get-Command aqt -ErrorAction SilentlyContinue
if ($aqt) {
    Write-Host "  aqtinstall already available: $($aqt.Source)" -ForegroundColor Green
} else {
    & python -m pip install --upgrade pip 2>$null
    & python -m pip install aqtinstall
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to install aqtinstall via pip"
    }
    # pip installs scripts to Python\Scripts which should be on PATH
    $env:Path = [Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [Environment]::GetEnvironmentVariable("Path", "User")
    # Also add the user Scripts dir
    $pyDir = Split-Path -Parent (Get-Command python).Source
    $scriptsDir = Join-Path $pyDir "Scripts"
    if (Test-Path $scriptsDir) {
        $env:Path = "$scriptsDir;$env:Path"
    }
    $aqt = Get-Command aqt -ErrorAction SilentlyContinue
    if ($aqt) {
        Write-Host "  aqtinstall installed: $($aqt.Source)" -ForegroundColor Green
    } else {
        Write-Host "  aqtinstall installed but may need PATH refresh. Trying python -m aqt..." -ForegroundColor Yellow
    }
}

# --- Step 5: Qt 6.11.1 ---
Write-Host "[5/6] Installing Qt $QT_VERSION ($QT_ARCH)..." -ForegroundColor Yellow
$qtKitDir = Join-Path $QT_INSTALL_DIR "$QT_VERSION\mingw_64"
if (Test-Path -LiteralPath (Join-Path $qtKitDir "lib\cmake\Qt6\Qt6Config.cmake")) {
    Write-Host "  Qt $QT_VERSION already installed at $qtKitDir" -ForegroundColor Green
} else {
    Write-Host "  Downloading Qt $QT_VERSION to $QT_INSTALL_DIR (this may take 5-10 minutes)..."
    New-Item -ItemType Directory -Force -Path $QT_INSTALL_DIR | Out-Null

    $modulesArg = ($QT_MODULES -join " ")
    $aqtCmd = Get-Command aqt -ErrorAction SilentlyContinue
    if ($aqtCmd) {
        & aqt install-qt windows desktop $QT_VERSION $QT_ARCH --outputdir $QT_INSTALL_DIR --modules $QT_MODULES
    } else {
        & python -m aqt install-qt windows desktop $QT_VERSION $QT_ARCH --outputdir $QT_INSTALL_DIR --modules $QT_MODULES
    }
    if ($LASTEXITCODE -ne 0) {
        throw "Qt installation failed (exit code $LASTEXITCODE). Check network connectivity and try again."
    }

    if (Test-Path -LiteralPath (Join-Path $qtKitDir "lib\cmake\Qt6\Qt6Config.cmake")) {
        Write-Host "  Qt $QT_VERSION installed at $qtKitDir" -ForegroundColor Green
    } else {
        throw "Qt install completed but Qt6Config.cmake not found at expected path: $qtKitDir"
    }
}

# --- Step 6: MinGW 13 toolchain ---
Write-Host "[6/6] Installing MinGW 13 toolchain..." -ForegroundColor Yellow
$mingwDir = Join-Path $QT_INSTALL_DIR "Tools\mingw1310_64"
if (Test-Path -LiteralPath (Join-Path $mingwDir "bin\g++.exe")) {
    Write-Host "  MinGW 13 already installed at $mingwDir" -ForegroundColor Green
} else {
    Write-Host "  Downloading MinGW 13 toolchain..."
    New-Item -ItemType Directory -Force -Path (Join-Path $QT_INSTALL_DIR "Tools") | Out-Null

    $aqtCmd = Get-Command aqt -ErrorAction SilentlyContinue
    if ($aqtCmd) {
        & aqt install-tool windows desktop $MINGW_TOOL --outputdir $QT_INSTALL_DIR
    } else {
        & python -m aqt install-tool windows desktop $MINGW_TOOL --outputdir $QT_INSTALL_DIR
    }
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "aqt install-tool for MinGW returned $LASTEXITCODE. Trying alternative download..."
        # Fallback: download MinGW from GitHub releases
        $mingwUrl = "https://github.com/niXman/mingw-builds-binaries/releases/download/13.1.0-rt_v11-rev1/x86_64-13.1.0-release-posix-seh-ucrt-rt_v11-rev1.7z"
        $mingwArchive = Join-Path $env:TEMP "mingw13.7z"
        Write-Host "  Downloading MinGW from GitHub..."
        Invoke-WebRequest -Uri $mingwUrl -OutFile $mingwArchive -UseBasicParsing
        # Extract with 7z if available, or tar
        $7z = Get-Command 7z -ErrorAction SilentlyContinue
        if ($7z) {
            & 7z x $mingwArchive -o"$(Join-Path $QT_INSTALL_DIR 'Tools')" -y
        } else {
            throw "MinGW download succeeded but 7z is not available to extract. Install 7-Zip or use Qt Online Installer."
        }
    }

    if (Test-Path -LiteralPath (Join-Path $mingwDir "bin\g++.exe")) {
        Write-Host "  MinGW 13 installed at $mingwDir" -ForegroundColor Green
    } else {
        # Check if it landed with a different folder name
        $toolsDir = Join-Path $QT_INSTALL_DIR "Tools"
        $found = Get-ChildItem -LiteralPath $toolsDir -Directory -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -match "mingw" -and (Test-Path (Join-Path $_.FullName "bin\g++.exe")) } |
            Select-Object -First 1
        if ($found) {
            Write-Host "  MinGW installed at $($found.FullName)" -ForegroundColor Green
            Write-Host "  (run.ps1 will auto-discover it)" -ForegroundColor Gray
        } else {
            Write-Warning "MinGW installation could not be verified. You may need to install it manually via Qt Online Installer."
        }
    }
}

# --- Done ---
Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host " All prerequisites installed!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Qt kit:    $qtKitDir"
Write-Host "MinGW:     $mingwDir"
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. Close and reopen your terminal (to refresh PATH)"
Write-Host "  2. Run: .\run.ps1"
Write-Host ""
Write-Host "For a release build:"
Write-Host '  $env:ARACHNEL_VERSION = "0.1.40"'
Write-Host '  $env:BUILD_TYPE = "Release"'
Write-Host '  $env:ARACHNEL_FAST_BUILD = "0"'
Write-Host "  .\run.ps1 --package"
Write-Host ""
