# Loads the GitHub PAT for this machine into the current PowerShell session.
# The token is NOT stored in the repo. It lives in:
#   %USERPROFILE%\.aqemu\github_pat.txt
# Rotate that file if the token was ever pasted into a chat, log, or commit.
# Usage (from a PowerShell prompt, in the repo):
#   . .\scripts\load_github_token.ps1

$TokenFile = Join-Path $env:USERPROFILE '.aqemu\github_pat.txt'
if (-not (Test-Path -LiteralPath $TokenFile)) {
    throw "Missing $TokenFile. Put a GitHub PAT with repo access in that file (one line, nothing else)."
}

$token = (Get-Content -LiteralPath $TokenFile -Raw).Trim()
if ([string]::IsNullOrWhiteSpace($token)) {
    throw "$TokenFile is empty."
}

$env:GH_TOKEN = $token
$env:GITHUB_TOKEN = $token
Remove-Variable token

Write-Host "GitHub token loaded from $TokenFile. It was not printed."
