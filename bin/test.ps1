# Windows equivalent of bin/test.
. (Join-Path $PSScriptRoot 'qt-env.ps1')
$BuildDir = Join-Path $Root 'build\tests'
New-Item -ItemType Directory -Force $BuildDir | Out-Null
Push-Location $BuildDir
try {
    Invoke-Checked $QMake @((Join-Path $Root 'tests\tests.pro'))
    Invoke-Make
    $env:QT_QPA_PLATFORM = 'offscreen'
    Invoke-Checked (Find-Binary $BuildDir 'keepnote_tests.exe') @()
} finally { Pop-Location }
