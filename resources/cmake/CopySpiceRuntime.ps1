# Copy spice-client-glib and transitive MSYS2/ucrt64 DLLs next to aqemu.exe.
param(
	[Parameter(Mandatory = $true)][string]$BinDir,
	[Parameter(Mandatory = $true)][string]$DestDir,
	[Parameter(Mandatory = $true)][string]$SeedDll
)

$ErrorActionPreference = "Stop"
if (-not (Test-Path $DestDir)) {
	New-Item -ItemType Directory -Path $DestDir | Out-Null
}

$objdump = Join-Path $BinDir "objdump.exe"
if (-not (Test-Path $objdump)) {
	# Fallback: copy seed + a few known deps only
	$fallback = @(
		$SeedDll,
		"libglib-2.0-0.dll", "libgobject-2.0-0.dll", "libgio-2.0-0.dll",
		"libgmodule-2.0-0.dll", "libintl-8.dll", "libiconv-2.dll",
		"libpcre2-8-0.dll", "libffi-8.dll", "libpixman-1-0.dll",
		"zlib1.dll", "libjpeg-8.dll", "libwinpthread-1.dll", "libgcc_s_seh-1.dll"
	)
	foreach ($dll in $fallback) {
		$src = Join-Path $BinDir $dll
		if (Test-Path $src) {
			Copy-Item -Force $src (Join-Path $DestDir $dll)
		}
	}
	exit 0
}

$seen = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$queue = [System.Collections.Generic.Queue[string]]::new()
[void]$queue.Enqueue($SeedDll)

$systemPrefixes = @(
	"ADVAPI32", "KERNEL32", "USER32", "WS2_32", "COMCTL32", "GDI32", "OLE32",
	"SHELL32", "IMM32", "SETUPAPI", "WINMM", "CRYPT32", "bcrypt", "NSI",
	"IPHLPAPI", "DNSAPI", "Secur32", "SHLWAPI", "OLEAUT32", "RPCRT4",
	"WINHTTP", "api-ms-win", "ntdll", "msvcrt", "ucrtbase", "vcruntime"
)

while ($queue.Count -gt 0) {
	$dll = $queue.Dequeue()
	if (-not $seen.Add($dll)) { continue }
	$path = Join-Path $BinDir $dll
	if (-not (Test-Path $path)) { continue }
	Copy-Item -Force $path (Join-Path $DestDir $dll)
	$imports = & $objdump -x $path 2>$null |
		Select-String "DLL Name:\s+(\S+)" |
		ForEach-Object { $_.Matches.Groups[1].Value }
	foreach ($imp in $imports) {
		$skip = $false
		foreach ($p in $systemPrefixes) {
			if ($imp -like "$p*") { $skip = $true; break }
		}
		if ($skip) { continue }
		if (Test-Path (Join-Path $BinDir $imp)) {
			[void]$queue.Enqueue($imp)
		}
	}
}

Write-Host "Copied $($seen.Count) SPICE runtime DLLs to $DestDir"
