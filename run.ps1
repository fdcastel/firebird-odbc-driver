[CmdletBinding()]
param(
    [ValidateSet("Build","Test","Clean")]
    [string]$Verb = "Test",

    [string]$Configuration = "Release",

    [string]$Platform = "x64",

    [string]$Solution = "Builds\MsVc2022.win\OdbcFb.sln"
)

#
# Configuration
#

# Firebird version to use for testing
$global:FirebirdVersion = '5.0.3'



#
# Functions
#

function Find-MSBuild {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $installPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
        if ($installPath) {
            $candidate = Join-Path $installPath "MSBuild\Current\Bin\MSBuild.exe"
            if (Test-Path $candidate) { return $candidate }
        }
    }

    $cmd = Get-Command msbuild -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $fallback = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
    if (Test-Path $fallback) { return $fallback }

    throw "MSBuild.exe not found. Install Visual Studio Build Tools or run this from a Developer PowerShell for VS 2022."
}

function Find-VSTest {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $installPath = & $vswhere -latest -products * -property installationPath
        if ($installPath) {
            $candidate = Join-Path $installPath "Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe"
            if (Test-Path $candidate) { return $candidate }
        }
    }

    throw "vstest.console.exe not found"
}

function Requires-ConnectionString {
    if ($env:FIREBIRD_ODBC_CONNECTION) {
        Write-Host "Using environment variable FIREBIRD_ODBC_CONNECTION."
    } else {
        Write-Host "Using temporary Firebird environment."

        Import-Module PSFirebird
        $fb = New-FirebirdEnvironment -Version $FirebirdVersion

        $tempPath = [System.IO.Path]::GetTempPath()
        $dbPath = Join-Path $tempPath "testdb.fdb"
        $db = New-FirebirdDatabase -Database $dbPath -Environment $fb -Force

        $clientLibrary = Join-Path $fb.Path 'fbclient.dll'
        $connectionString = "Driver={Firebird ODBC Driver};Database=$dbPath;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8;CLIENT=$clientLibrary"
        
        $env:FIREBIRD_ODBC_CONNECTION = $connectionString
        return $connectionString
    }        
}



#
# Main
#

$ErrorActionPreference = "Stop"

# Configure GitHub API authentication if token is available to avoid rate limits.
$githubToken = if ($env:GITHUB_TOKEN) { $env:GITHUB_TOKEN } elseif ($env:GH_TOKEN) { $env:GH_TOKEN } else { $null }
if ($githubToken) {
    $PSDefaultParameterValues['Invoke-RestMethod:Headers'] = @{ Authorization = "token $githubToken" }
}

$msbuild = Find-MSBuild

# Clean
if ($Verb -eq "Clean") {
    & $msbuild $Solution /t:Clean /p:Configuration=$Configuration /p:Platform=$Platform /m /v:minimal
    if ($LASTEXITCODE -ne 0) {
        throw "MSBuild failed with exit code $LASTEXITCODE"
    }
    exit 0
}

# Build
& $msbuild $Solution /p:Configuration=$Configuration /p:Platform=$Platform /m /v:minimal
if ($LASTEXITCODE -ne 0) {
    throw "MSBuild failed with exit code $LASTEXITCODE"
}

# Test
if ($Verb -ne "Test") {
    exit 0
}
Requires-ConnectionString

$testDll = "Tests\OdbcTests\$Platform\$Configuration\OdbcTests.dll"
"           Tests\OdbcTests\x64\Release\OdbcTests.dll"
"           Tests\OdbcTests\x64\Release\OdbcTests.dll"
if (-not (Test-Path $testDll)) {
    throw "Test DLL not found: $testDll"
}

$verbosity = if ($PSCmdlet.MyInvocation.BoundParameters["Verbose"]) { "detailed" } else { "normal" }
$vstest = Find-VSTest
& $vstest $testDll --logger:"console;verbosity=$verbosity"

$exitCode = $LASTEXITCODE
if ($exitCode -eq 0) {
    Write-Host "`n✓ All tests passed!" -ForegroundColor Green
} else {
    Write-Host "`n⚠ Some tests failed (exit code: $exitCode)" -ForegroundColor Yellow
}
exit $exitCode
