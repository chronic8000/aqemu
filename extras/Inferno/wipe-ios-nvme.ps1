# Wipe Inferno NVMe raw images after a partial/failed idevicerestore.
# AQEMU recreates them on next Power On (same sizes as Apple_SoC_Support.cpp)
# and seeds a placeholder GPT on root (AQEMU_SEED). This script can seed
# immediately so an older aqemu.exe still presents EFI PART to ramrod.
param(
    [Parameter(Mandatory = $true)]
    [string]$VmXml
)

$ErrorActionPreference = 'Stop'
$xml = Resolve-Path -LiteralPath $VmXml
$base = [System.IO.Path]::GetFileNameWithoutExtension($xml.Path)
$dir = Join-Path (Split-Path -Parent $xml.Path) "${base}_inferno"

$names = @(
    'sep_nvram', 'sep_ssc', 'root', 'firmware', 'syscfg',
    'ctrl_bits', 'nvram', 'effaceable', 'panic_log'
)

if (-not (Test-Path -LiteralPath $dir)) {
    Write-Host "Nothing to wipe (folder missing): $dir"
    exit 0
}

Write-Host "Will delete Inferno NVMe images under:"
Write-Host "  $dir"
Write-Host ""
foreach ($n in $names) {
    $p = Join-Path $dir $n
    if (Test-Path -LiteralPath $p) {
        $sz = (Get-Item -LiteralPath $p).Length
        Write-Host "  $n ($([math]::Round($sz / 1MB, 1)) MiB)"
    }
}
Write-Host ""
$confirm = Read-Host 'Type YES to delete'
if ($confirm -ne 'YES') {
    Write-Host 'Cancelled.'
    exit 1
}

foreach ($n in $names) {
    $p = Join-Path $dir $n
    if (Test-Path -LiteralPath $p) {
        Remove-Item -LiteralPath $p -Force
        Write-Host "Removed $n"
    }
}
$seed = Join-Path $PSScriptRoot 'seed-root-gpt.py'
if (Test-Path -LiteralPath $seed) {
    Write-Host 'Seeding placeholder GPT on root (AQEMU_SEED)...'
    python $seed --vm-xml $xml.Path
} else {
    Write-Host 'seed-root-gpt.py not next to this script; next AQEMU Power On will seed GPT.'
}
Write-Host 'Done. Power Off iOS, Stop companion, Start companion, Power On iOS, Restore.'
