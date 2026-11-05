# Windows equivalent of bin/test.
. (Join-Path $PSScriptRoot 'qt-env.ps1')
$BuildDir = Join-Path $Root 'build\tests'
New-Item -ItemType Directory -Force $BuildDir | Out-Null
Push-Location $BuildDir
try {
    Invoke-Checked $QMake @((Join-Path $Root 'tests\tests.pro'))
    Invoke-Make
    $env:QT_QPA_PLATFORM = 'offscreen'
    $Tests = Find-Binary $BuildDir 'keepnote_tests.exe'
    $Junit = Join-Path $BuildDir 'results.xml'
    Remove-Item -Force $Junit -ErrorAction SilentlyContinue
    Write-Host "Running $Tests"
    & $Tests -o '-,txt' -o "$Junit,junitxml"
    $code = $LASTEXITCODE
    Write-Host "keepnote_tests exited with code $code"
    if ($code -ne 0) { throw "Tests failed (exit code $code)" }
    # Guard against a silent no-op run: require a results file with at least one test case.
    if (-not (Test-Path $Junit)) { throw "No test results were written to $Junit" }
    [xml]$results = Get-Content $Junit
    $suite = $results.testsuite
    Write-Host "Results: $($suite.tests) tests, $($suite.failures) failures, $($suite.errors) errors"
    if ([int]$suite.tests -eq 0 -or [int]$suite.failures -ne 0 -or [int]$suite.errors -ne 0) { throw 'Test results show failures or no tests' }
} finally { Pop-Location }
