# Builds Keepnote and bundles it with its Qt runtime into dist\keepnote-windows.zip.
. (Join-Path $PSScriptRoot 'qt-env.ps1')
& (Join-Path $PSScriptRoot 'build.ps1')
$Exe = Find-Binary (Join-Path $Root 'build') 'keepnote.exe'
$Stage = Join-Path $Root 'dist\keepnote'
Remove-Item -Recurse -Force $Stage -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $Stage | Out-Null
Copy-Item $Exe $Stage
$Deploy = Find-Tool @('windeployqt6', 'windeployqt')
Invoke-Checked $Deploy @('--release', '--qmldir', (Join-Path $Root 'src'), '--no-translations', (Join-Path $Stage 'keepnote.exe'))
$Zip = Join-Path $Root 'dist\keepnote-windows.zip'
Remove-Item -Force $Zip -ErrorAction SilentlyContinue
Compress-Archive -Path (Join-Path $Stage '*') -DestinationPath $Zip
Write-Host "Packaged $Zip"
