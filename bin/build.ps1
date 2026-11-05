# Windows equivalent of bin/build.
. (Join-Path $PSScriptRoot 'qt-env.ps1')
$BuildDir = Join-Path $Root 'build'
New-Item -ItemType Directory -Force $BuildDir | Out-Null
Push-Location $BuildDir
try {
    Invoke-Checked $QMake @((Join-Path $Root 'keepnote.pro'))
    Invoke-Make
    Write-Host "Built $(Find-Binary $BuildDir 'keepnote.exe')"
} finally { Pop-Location }
