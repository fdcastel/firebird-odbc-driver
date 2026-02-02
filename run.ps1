[CmdletBinding()]
param(
    [ValidateSet("Build","Test","Clean")]
    [string]$Verb = "Test",

    [string]$Configuration = "Release",

    [string]$Platform = "x64",

    [string]$Solution = "Builds\MsVc2022.win\OdbcFb.sln"
)

$ErrorActionPreference = "Stop"

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
    if (-not $env:FIREBIRD_ODBC_CONNECTION) {
        Write-Warning "Environment variable FIREBIRD_ODBC_CONNECTION is not set. Skipping test execution."
        exit 0
    }
}



#
# Main
#

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
