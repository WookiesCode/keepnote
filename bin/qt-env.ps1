# Shared helpers for the Windows scripts. Dot-source this file.
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot

function Find-Tool([string[]]$Names) {
    foreach ($name in $Names) {
        $cmd = Get-Command $name -ErrorAction SilentlyContinue
        if ($cmd) { return $cmd.Source }
    }
    throw "None of [$($Names -join ', ')] found on PATH. Open a Qt command prompt (or add Qt's bin directory and your compiler to PATH)."
}

$QMake = Find-Tool @('qmake6', 'qmake')
# jom builds in parallel; nmake is MSVC's serial fallback; mingw32-make is for MinGW Qt kits.
$Make = Find-Tool @('jom', 'nmake', 'mingw32-make')

function Invoke-Checked([string]$Exe, [string[]]$Arguments) {
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Exe exited with code $LASTEXITCODE" }
}

function Invoke-Make {
    if ((Split-Path -Leaf $Make) -like 'mingw32-make*') {
        Invoke-Checked $Make @("-j$env:NUMBER_OF_PROCESSORS")
    } else {
        Invoke-Checked $Make @()
    }
}

# qmake puts Windows binaries in release\ unless debug_and_release is off; accept either.
function Find-Binary([string]$Dir, [string]$Name) {
    foreach ($candidate in @((Join-Path $Dir $Name), (Join-Path $Dir "release\$Name"))) {
        if (Test-Path $candidate) { return $candidate }
    }
    throw "Built binary $Name not found under $Dir"
}
