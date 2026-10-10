# Windows PowerShell version. Run from anywhere:  .\tools\fetch_mavlink.ps1
$root = Split-Path -Parent $PSScriptRoot
New-Item -ItemType Directory -Force "$root\third_party\mavlink" | Out-Null
if (-not (Test-Path "$root\third_party\mavlink\c_library_v2")) {
    git clone --depth 1 https://github.com/mavlink/c_library_v2.git "$root\third_party\mavlink\c_library_v2"
}
Write-Host "MAVLink headers ready."
