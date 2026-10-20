# Copies the Release x64 build and notices into dist\ and zips them.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$bin  = Join-Path $root 'build\bin\x64\Release'
$out  = Join-Path $root 'dist\playerP2-x64'

foreach ($f in 'playerP2.exe', 'libmpv-2.dll') {
    if (-not (Test-Path (Join-Path $bin $f))) { throw "Missing $f in $bin. Build Release x64 first." }
}

if (Test-Path $out) { Remove-Item $out -Recurse -Force }
New-Item -ItemType Directory -Path $out | Out-Null

Copy-Item (Join-Path $bin 'playerP2.exe'), (Join-Path $bin 'libmpv-2.dll') $out
Copy-Item (Join-Path $root 'docs\THIRD_PARTY_NOTICES.md') $out
Copy-Item (Join-Path $root 'README.md') $out

$zip = "$out.zip"
if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path (Join-Path $out '*') -DestinationPath $zip
Write-Host "Created $zip"
