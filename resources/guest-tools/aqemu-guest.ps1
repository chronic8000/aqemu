# Send files copied in Windows Explorer to AQEMU.
# powershell -STA -ExecutionPolicy Bypass -File aqemu-guest.ps1
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Windows.Forms
$portPath = "\\.\org.aqemu.files"
$enc = New-Object System.Text.UTF8Encoding $false
$last = ""
while ($true) {
    try {
        $fs = New-Object System.IO.FileStream($portPath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Write, [System.IO.FileShare]::ReadWrite)
        try {
            while ($true) {
                Start-Sleep -Milliseconds 800
                $paths = @()
                foreach ($p in [System.Windows.Forms.Clipboard]::GetFileDropList()) {
                    if (Test-Path -LiteralPath $p -PathType Leaf) { $paths += $p }
                }
                $sig = ($paths -join "|")
                if (-not $sig -or $sig -eq $last) { continue }
                $ready = @()
                foreach ($p in $paths) {
                    try {
                        $data = [System.IO.File]::ReadAllBytes($p)
                    } catch {
                        continue
                    }
                    if ($data.Length -gt 64MB) { continue }
                    $ready += ,@($p, $data)
                }
                if ($ready.Count -eq 0) { continue }
                $last = $sig
                $header = $enc.GetBytes("AQEMU1`nFROM-GUEST`n$($ready.Count)`n")
                $fs.Write($header, 0, $header.Length)
                foreach ($item in $ready) {
                    $name = [System.IO.Path]::GetFileName($item[0])
                    $data = $item[1]
                    $meta = $enc.GetBytes("$name`n$($data.Length)`n")
                    $fs.Write($meta, 0, $meta.Length)
                    $fs.Write($data, 0, $data.Length)
                }
                $fs.Flush()
            }
        } finally {
            $fs.Close()
        }
    } catch {
        Start-Sleep -Seconds 2
    }
}
