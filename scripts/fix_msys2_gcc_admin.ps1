# Run this ONCE as Administrator (right-click PowerShell → Run as administrator)
# Fixes MSYS2 MinGW gcc broken by a wrong pthread DLL in C:\WINDOWS.

$ErrorActionPreference = "Stop"
$src = "C:\WINDOWS\libwinpthread-1.dll"
$bak = "C:\WINDOWS\libwinpthread-1.dll.aqemu_bak"

if (Test-Path $bak) {
  Write-Host "Already fixed: $bak exists"
} elseif (Test-Path $src) {
  Rename-Item -LiteralPath $src -NewName "libwinpthread-1.dll.aqemu_bak"
  Write-Host "Renamed $src -> $bak"
} else {
  Write-Host "No conflicting DLL at $src"
}

# Sanity-check MSYS2 gcc
& C:\msys64\usr\bin\bash.exe --login -c 'export PATH=/mingw64/bin:$PATH; printf "%s\n" "int main(void){return 0;}" > /tmp/t.c; gcc -c /tmp/t.c -o /tmp/t.o && echo MSYS2_GCC_OK'
if ($LASTEXITCODE -ne 0) { throw "MSYS2 gcc still broken after rename" }
Write-Host "Done. You can now run: bash scripts/build_qemu_windows_msys.sh"
