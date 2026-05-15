# Awake Buddy

A Windows desktop application built with Qt 6 and C++.

## Prerequisites

- [Qt 6](https://www.qt.io/download) (e.g. Qt 6.10.1 with MinGW 64-bit)
- CMake 3.16+
- MinGW 64-bit (bundled with Qt)
- [Inno Setup 6](https://jrsoftware.org/isinfo.php) (for creating the installer)

## Building via Terminal

### 1. Configure

```powershell
$QtDir = "C:\Qt\6.10.1\mingw_64"
$MingwDir = "C:\Qt\Tools\mingw1310_64"
$env:PATH = "$MingwDir\bin;$QtDir\bin;$env:PATH"

cmake -S . -B build/Release `
    -G "MinGW Makefiles" `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH="$QtDir" `
    -DCMAKE_MAKE_PROGRAM="$MingwDir\bin\mingw32-make.exe"
```

### 2. Build

```powershell
cmake --build build/Release --config Release -- -j $env:NUMBER_OF_PROCESSORS
```

### 3. Deploy Qt Dependencies

```powershell
New-Item -ItemType Directory -Path deploy -Force | Out-Null
Copy-Item build/Release/Awake-Buddy.exe -Destination deploy/
& "$QtDir\bin\windeployqt.exe" --release --no-translations --no-opengl-sw deploy/Awake-Buddy.exe
```

The `deploy/` folder now contains a portable build with all required DLLs.

## Creating the Installer Setup

### Option A: Automated (recommended)

Run the deploy script which handles the full build, deployment, and installer creation:

```powershell
.\deploy.ps1
```

You can override default paths:

```powershell
.\deploy.ps1 -QtDir "C:\Qt\6.10.1\mingw_64" `
             -MingwDir "C:\Qt\Tools\mingw1310_64" `
             -CmakeDir "C:\Qt\Tools\CMake_64\bin" `
             -IsccPath "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"
```

### Option B: Manual

After building and deploying (steps above), run Inno Setup Compiler:

```powershell
& "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe" installer.iss
```

The installer will be created at `installer_output/AwakeBuddy-Setup.exe`.
