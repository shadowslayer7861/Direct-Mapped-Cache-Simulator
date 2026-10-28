param(
    [ValidateSet('build', 'test', 'run', 'clean')]
    [string]$Action = 'build'
)

$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot

$BuildDir = Join-Path $PSScriptRoot 'build'
$ExecutableExtension = if ($env:OS -eq 'Windows_NT') { '.exe' } else { '' }
$CommonArguments = @(
    '-std=c11'
    '-Wall'
    '-Wextra'
    '-Wpedantic'
    '-O2'
    '-Iinclude'
)

function Invoke-Gcc {
    param([string[]]$Arguments)

    & gcc @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "gcc failed with exit code $LASTEXITCODE."
    }
}

function Get-SourceFiles {
    param([bool]$IncludeMain)

    $sources = @(Get-ChildItem -Path 'src' -Filter '*.c' -File |
        Sort-Object Name |
        ForEach-Object { Join-Path 'src' $_.Name })
    if (-not $IncludeMain) {
        $sources = @($sources | Where-Object { $_ -ne (Join-Path 'src' 'main.c') })
    }
    return ,$sources
}

function Build-Program {
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
    $Output = Join-Path $BuildDir "cachesim$ExecutableExtension"
    $Sources = Get-SourceFiles -IncludeMain $true
    $Arguments = $CommonArguments + $Sources + @('-o', $Output)
    Invoke-Gcc -Arguments $Arguments
    return $Output
}

function Build-Tests {
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
    $LibrarySources = Get-SourceFiles -IncludeMain $false
    $testFiles = @(Get-ChildItem -Path 'tests' -Filter 'test_*.c' -File |
        Sort-Object Name)
    foreach ($testFile in $testFiles) {
        $testName = [IO.Path]::GetFileNameWithoutExtension($testFile.Name)
        $Output = Join-Path $BuildDir "$testName$ExecutableExtension"
        $Arguments = $CommonArguments + @(Join-Path 'tests' $testFile.Name) +
            $LibrarySources + @('-o', $Output)
        Invoke-Gcc -Arguments $Arguments
    }
}

function Run-Tests {
    $testFiles = @(Get-ChildItem -Path 'tests' -Filter 'test_*.c' -File |
        Sort-Object Name)
    foreach ($testFile in $testFiles) {
        $testName = [IO.Path]::GetFileNameWithoutExtension($testFile.Name)
        $TestExecutable = Join-Path $BuildDir "$testName$ExecutableExtension"
        & $TestExecutable
        if ($LASTEXITCODE -ne 0) {
            throw "Test '$testName' failed with exit code $LASTEXITCODE."
        }
    }
}

switch ($Action) {
    'build' {
        Build-Program | Out-Null
    }
    'test' {
        Build-Tests
        Run-Tests
    }
    'run' {
        $Executable = Build-Program
        & $Executable
        if ($LASTEXITCODE -ne 0) {
            throw "Program failed with exit code $LASTEXITCODE."
        }
    }
    'clean' {
        if (Test-Path -LiteralPath $BuildDir) {
            Remove-Item -LiteralPath $BuildDir -Recurse -Force
        }
    }
}
