# deploy.ps1 — Build, package, and create installer for Awake Buddy
param(
    [string]$QtDir     = "C:\Qt\6.10.1\mingw_64",
    [string]$MingwDir  = "C:\Qt\Tools\mingw1310_64",
    [string]$CmakeDir  = "C:\Qt\Tools\CMake_64\bin",
    [string]$IsccPath  = "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"
)

$ErrorActionPreference = "Stop"
$ProjectDir = $PSScriptRoot
$BuildDir   = Join-Path $ProjectDir "build\Release"
$DeployDir  = Join-Path $ProjectDir "deploy"

# Add tools to PATH
$env:PATH = "$($MingwDir)\bin;$($QtDir)\bin;$($CmakeDir);$env:PATH"

Write-Host "=== Cleaning ===" -ForegroundColor Cyan
if (Test-Path $BuildDir)   { Remove-Item $BuildDir  -Recurse -Force }
if (Test-Path $DeployDir)  { Remove-Item $DeployDir -Recurse -Force }
New-Item -ItemType Directory -Path $BuildDir  | Out-Null
New-Item -ItemType Directory -Path $DeployDir | Out-Null

Write-Host "=== Configuring (Release) ===" -ForegroundColor Cyan
cmake -S $ProjectDir -B $BuildDir `
    -G "MinGW Makefiles" `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH="$QtDir" `
    -DCMAKE_MAKE_PROGRAM="$MingwDir\bin\mingw32-make.exe"

if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

Write-Host "=== Building ===" -ForegroundColor Cyan
cmake --build $BuildDir --config Release -- -j $env:NUMBER_OF_PROCESSORS
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

Write-Host "=== Deploying Qt dependencies ===" -ForegroundColor Cyan
$ExePath = Join-Path $BuildDir "Awake-Buddy.exe"
Copy-Item $ExePath -Destination $DeployDir

& "$QtDir\bin\windeployqt.exe" --release --no-translations --no-opengl-sw `
    (Join-Path $DeployDir "Awake-Buddy.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed" }

Write-Host "=== Exporting app icon ===" -ForegroundColor Cyan
$IconPng = Join-Path $ProjectDir "awakebuddy.png"
$IconIco = Join-Path $ProjectDir "awakebuddy.ico"
$DeployExe = Join-Path $DeployDir "Awake-Buddy.exe"

# Run with full path for icon output
Start-Process -FilePath $DeployExe -ArgumentList "--export-icon", "`"$IconPng`"" -Wait -NoNewWindow
if (-not (Test-Path $IconPng)) { throw "Icon export failed - PNG not created" }

# Convert PNG to ICO using .NET (BMP format for compatibility with Inno Setup)
Add-Type -AssemblyName System.Drawing
$bmp = [System.Drawing.Bitmap]::FromFile($IconPng)
$icoStream = [System.IO.File]::Create($IconIco)
$writer = New-Object System.IO.BinaryWriter($icoStream)
$sizes = @(256, 48, 32, 16)
$writer.Write([uint16]0)      # reserved
$writer.Write([uint16]1)      # type: icon
$writer.Write([uint16]$sizes.Count)  # count

# Build image data for each size first
$imgDatas = @()
foreach ($sz in $sizes) {
    $resized = New-Object System.Drawing.Bitmap($bmp, $sz, $sz)
    if ($sz -eq 256) {
        # 256x256 uses PNG compression
        $ms = New-Object System.IO.MemoryStream
        $resized.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
        $imgDatas += ,$ms.ToArray()
        $ms.Dispose()
    } else {
        # Smaller sizes use BMP/DIB format for maximum compatibility
        $ms = New-Object System.IO.MemoryStream
        $dibWriter = New-Object System.IO.BinaryWriter($ms)
        # BITMAPINFOHEADER
        $dibWriter.Write([uint32]40)        # header size
        $dibWriter.Write([int32]$sz)        # width
        $dibWriter.Write([int32]($sz * 2))  # height (doubled for XOR+AND mask)
        $dibWriter.Write([uint16]1)         # planes
        $dibWriter.Write([uint16]32)        # bpp
        $dibWriter.Write([uint32]0)         # compression (BI_RGB)
        $pixelDataSize = $sz * $sz * 4
        $andMaskRowBytes = [Math]::Ceiling($sz / 8)
        if ($andMaskRowBytes % 4 -ne 0) { $andMaskRowBytes = $andMaskRowBytes + (4 - $andMaskRowBytes % 4) }
        $andMaskSize = $andMaskRowBytes * $sz
        $dibWriter.Write([uint32]($pixelDataSize + $andMaskSize))  # image size
        $dibWriter.Write([int32]0)          # X ppm
        $dibWriter.Write([int32]0)          # Y ppm
        $dibWriter.Write([uint32]0)         # colors used
        $dibWriter.Write([uint32]0)         # important colors
        # Pixel data (BGRA, bottom-up)
        for ($y = $sz - 1; $y -ge 0; $y--) {
            for ($x = 0; $x -lt $sz; $x++) {
                $pixel = $resized.GetPixel($x, $y)
                $dibWriter.Write([byte]$pixel.B)
                $dibWriter.Write([byte]$pixel.G)
                $dibWriter.Write([byte]$pixel.R)
                $dibWriter.Write([byte]$pixel.A)
            }
        }
        # AND mask (all zeros = fully opaque, alpha channel handles transparency)
        $zeroRow = New-Object byte[] $andMaskRowBytes
        for ($y = 0; $y -lt $sz; $y++) { $dibWriter.Write($zeroRow) }
        $dibWriter.Flush()
        $imgDatas += ,$ms.ToArray()
        $ms.Dispose()
    }
    $resized.Dispose()
}

# Write directory entries
$dataOffset = 6 + ($sizes.Count * 16)
for ($i = 0; $i -lt $sizes.Count; $i++) {
    $sz = $sizes[$i]
    $w = if ($sz -eq 256) { 0 } else { $sz }
    $h = $w
    $writer.Write([byte]$w)
    $writer.Write([byte]$h)
    $writer.Write([byte]0)     # palette
    $writer.Write([byte]0)     # reserved
    $writer.Write([uint16]1)   # planes
    $writer.Write([uint16]32)  # bpp
    $writer.Write([uint32]$imgDatas[$i].Length)
    $writer.Write([uint32]$dataOffset)
    $dataOffset += $imgDatas[$i].Length
}
# Write image data
foreach ($d in $imgDatas) { $writer.Write($d) }
$writer.Close()
$icoStream.Close()
$bmp.Dispose()
Remove-Item $IconPng
Write-Host "Icon saved: $IconIco"

Write-Host "=== Creating installer ===" -ForegroundColor Cyan
if (-not (Test-Path $IsccPath)) {
    Write-Host "Inno Setup not found at: $IsccPath" -ForegroundColor Yellow
    Write-Host "Skipping installer. Portable build is in: $DeployDir" -ForegroundColor Yellow
    exit 0
}

& $IsccPath (Join-Path $ProjectDir "installer.iss")
if ($LASTEXITCODE -ne 0) { throw "Inno Setup failed" }

Write-Host ""
Write-Host "=== Done ===" -ForegroundColor Green
Write-Host "Installer: $(Join-Path $ProjectDir 'installer_output\AwakeBuddy-Setup.exe')"
Write-Host "Portable:  $DeployDir"
