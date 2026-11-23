# Build FUNKY-TRAIN from Windows. The SDK build script expects python3; on
# Windows python3 can be a Store alias, so point it at the real python.exe.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$shimDir = Join-Path $root 'tools\shim'
$shim = Join-Path $shimDir 'python3.exe'

$python = (Get-Command python -ErrorAction SilentlyContinue).Source
if (-not $python) {
  throw 'python was not found on PATH. Install Python 3 first.'
}
New-Item -ItemType Directory -Force $shimDir | Out-Null
Copy-Item $python $shim -Force

$zig = (Get-Command zig -ErrorAction SilentlyContinue).Source
if (-not $zig) {
  throw 'zig was not found on PATH. Install Zig 0.13.0 first.'
}

$env:PATH = "$shimDir;$env:PATH"
$env:ZIG = $zig
Push-Location (Join-Path $root 'sdk')
try {
  sh tools/build.sh ../firmware/funky-train
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} finally {
  Pop-Location
}
