#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Install/Uninstall development build of Firebird ODBC Driver

.DESCRIPTION
    This script installs or uninstalls the locally built Firebird ODBC driver
    under a custom name "Firebird ODBC Driver SUT" (System Under Test).
    
    This allows side-by-side installation with the official driver.
    Works on both Windows and Linux.

.PARAMETER Install
    Install the driver

.PARAMETER Uninstall
    Uninstall the driver

.PARAMETER Platform
    Platform to install/uninstall (Win32, x64, ARM64). Defaults to x64.
    On Linux, this parameter is ignored.

.PARAMETER DriverName
    Custom driver name. Defaults to "Firebird ODBC Driver SUT"

.EXAMPLE
    .\dev.ps1 -Install
    .\dev.ps1 -Install -Platform x64
    .\dev.ps1 -Uninstall
    .\dev.ps1 -Install -DriverName "Firebird ODBC Driver Dev"
#>

[CmdletBinding()]
param(
    [Parameter(ParameterSetName='Install')]
    [switch]$Install,
    
    [Parameter(ParameterSetName='Uninstall')]
    [switch]$Uninstall,
    
    [Parameter()]
    [ValidateSet('Win32', 'x64', 'ARM64')]
    [string]$Platform = 'x64',
    
    [Parameter()]
    [string]$DriverName = 'Firebird ODBC Driver SUT'
)

$ErrorActionPreference = 'Stop'

# Detect OS
$IsWindowsOS = $IsWindows -or ($PSVersionTable.PSVersion.Major -le 5)
$IsLinuxOS = $IsLinux

if (-not $IsWindowsOS -and -not $IsLinuxOS) {
    Write-Error "Unsupported OS. Only Windows and Linux are supported."
    exit 1
}

if (-not $Install -and -not $Uninstall) {
    Write-Error "Please specify either -Install or -Uninstall"
    Write-Host ""
    Write-Host "Usage:"
    Write-Host "  .\dev.ps1 -Install [-Platform x64]"
    Write-Host "  .\dev.ps1 -Uninstall [-Platform x64]"
    exit 1
}

#region Windows Installation

function Install-WindowsDriver {
    param(
        [string]$Platform,
        [string]$DriverName
    )
    
    Write-Host "Installing ODBC driver on Windows..." -ForegroundColor Cyan
    Write-Host "Platform: $Platform" -ForegroundColor Gray
    Write-Host "Driver Name: $DriverName" -ForegroundColor Gray
    
    # Map platform to directory name
    $platformDir = $Platform
    
    # Locate built DLL
    $driverPath = Join-Path $PSScriptRoot "Builds\MsVc2022.win\$platformDir\Release\FirebirdODBC.dll"
    
    if (-not (Test-Path $driverPath)) {
        Write-Error "Driver not found at: $driverPath"
        Write-Host "Please build the driver first using: .\build.ps1" -ForegroundColor Yellow
        exit 1
    }
    
    Write-Host "Found driver: $driverPath" -ForegroundColor Green
    
    # Get system directory for installation
    $systemDir = [System.Environment]::GetFolderPath('System')
    $targetPath = Join-Path $systemDir "FirebirdODBC_SUT.dll"
    
    Write-Host "Copying driver to: $targetPath" -ForegroundColor Gray
    
    # Copy DLL to system directory (requires admin)
    try {
        Copy-Item -Path $driverPath -Destination $targetPath -Force
        Write-Host "✓ Driver copied successfully" -ForegroundColor Green
    }
    catch {
        Write-Error "Failed to copy driver. Run as Administrator."
        exit 1
    }
    
    # Register using odbcinst or direct registry manipulation
    # We'll use Add-OdbcDriver cmdlet if available, otherwise use odbcinst.exe
    
    if (Get-Command Add-OdbcDriver -ErrorAction SilentlyContinue) {
        Write-Host "Registering driver using Add-OdbcDriver..." -ForegroundColor Gray
        
        # Note: Add-OdbcDriver doesn't support custom names easily
        # We'll use odbcinst.exe instead
    }
    
    # Use odbcinst.exe for registration
    $odbcinstPath = Get-Command odbcinst.exe -ErrorAction SilentlyContinue
    
    if ($odbcinstPath) {
        Write-Host "Using odbcinst.exe for registration..." -ForegroundColor Gray
        
        # Create temporary INI file for registration
        $tempIni = [System.IO.Path]::GetTempFileName()
        $iniContent = @"
[$DriverName]
Driver=$targetPath
Setup=$targetPath
APILevel=1
ConnectFunctions=YYY
DriverODBCVer=03.80
FileUsage=0
SQLLevel=1
"@
        
        Set-Content -Path $tempIni -Value $iniContent
        
        # Register the driver
        $result = & odbcinst.exe -i -d -f $tempIni 2>&1
        
        Remove-Item -Path $tempIni -Force
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "✓ Driver registered successfully" -ForegroundColor Green
        }
        else {
            Write-Warning "odbcinst returned: $result"
            Write-Host "Attempting direct registry registration..." -ForegroundColor Yellow
            Register-WindowsDriverRegistry -DriverName $DriverName -DriverPath $targetPath
        }
    }
    else {
        Write-Host "odbcinst.exe not found, using registry registration..." -ForegroundColor Yellow
        Register-WindowsDriverRegistry -DriverName $DriverName -DriverPath $targetPath
    }
    
    Write-Host ""
    Write-Host "✓ Installation complete!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Test connection string:" -ForegroundColor Cyan
    Write-Host "Driver={$DriverName};Database=localhost:C:\path\to\database.fdb;UID=SYSDBA;PWD=masterkey" -ForegroundColor Gray
}

function Register-WindowsDriverRegistry {
    param(
        [string]$DriverName,
        [string]$DriverPath
    )
    
    # Determine architecture
    if ([Environment]::Is64BitOperatingSystem) {
        $odbcKey = "HKLM:\SOFTWARE\ODBC\ODBCINST.INI"
    }
    else {
        $odbcKey = "HKLM:\SOFTWARE\ODBC\ODBCINST.INI"
    }
    
    # Create driver key
    $driverKey = Join-Path $odbcKey $DriverName
    
    if (-not (Test-Path $driverKey)) {
        New-Item -Path $driverKey -Force | Out-Null
    }
    
    # Set driver properties
    Set-ItemProperty -Path $driverKey -Name "Driver" -Value $DriverPath
    Set-ItemProperty -Path $driverKey -Name "Setup" -Value $DriverPath
    Set-ItemProperty -Path $driverKey -Name "APILevel" -Value "1"
    Set-ItemProperty -Path $driverKey -Name "ConnectFunctions" -Value "YYY"
    Set-ItemProperty -Path $driverKey -Name "DriverODBCVer" -Value "03.80"
    Set-ItemProperty -Path $driverKey -Name "FileUsage" -Value "0"
    Set-ItemProperty -Path $driverKey -Name "SQLLevel" -Value "1"
    
    # Add to drivers list
    $driversKey = Join-Path $odbcKey "ODBC Drivers"
    if (-not (Test-Path $driversKey)) {
        New-Item -Path $driversKey -Force | Out-Null
    }
    
    Set-ItemProperty -Path $driversKey -Name $DriverName -Value "Installed"
    
    Write-Host "✓ Driver registered in registry" -ForegroundColor Green
}

function Uninstall-WindowsDriver {
    param(
        [string]$DriverName
    )
    
    Write-Host "Uninstalling ODBC driver on Windows..." -ForegroundColor Cyan
    Write-Host "Driver Name: $DriverName" -ForegroundColor Gray
    
    # Remove from registry
    $odbcKey = "HKLM:\SOFTWARE\ODBC\ODBCINST.INI"
    $driverKey = Join-Path $odbcKey $DriverName
    
    if (Test-Path $driverKey) {
        Remove-Item -Path $driverKey -Recurse -Force
        Write-Host "✓ Driver removed from registry" -ForegroundColor Green
    }
    
    # Remove from drivers list
    $driversKey = Join-Path $odbcKey "ODBC Drivers"
    if (Test-Path $driversKey) {
        Remove-ItemProperty -Path $driversKey -Name $DriverName -ErrorAction SilentlyContinue
    }
    
    # Remove DLL
    $systemDir = [System.Environment]::GetFolderPath('System')
    $targetPath = Join-Path $systemDir "FirebirdODBC_SUT.dll"
    
    if (Test-Path $targetPath) {
        try {
            Remove-Item -Path $targetPath -Force
            Write-Host "✓ Driver DLL removed" -ForegroundColor Green
        }
        catch {
            Write-Warning "Could not remove $targetPath - may be in use"
            Write-Host "You may need to reboot to complete uninstallation" -ForegroundColor Yellow
        }
    }
    
    Write-Host ""
    Write-Host "✓ Uninstallation complete!" -ForegroundColor Green
}

#endregion

#region Linux Installation

function Install-LinuxDriver {
    param(
        [string]$DriverName
    )
    
    Write-Host "Installing ODBC driver on Linux..." -ForegroundColor Cyan
    Write-Host "Driver Name: $DriverName" -ForegroundColor Gray
    
    # Check if odbcinst is available
    $odbcinst = Get-Command odbcinst -ErrorAction SilentlyContinue
    if (-not $odbcinst) {
        Write-Error "odbcinst not found. Please install unixODBC: sudo apt-get install unixodbc unixodbc-dev"
        exit 1
    }
    
    # Locate built .so file
    $buildDir = Join-Path $PSScriptRoot "Builds/Gcc.lin"
    $driverPath = $null
    
    # Find the Release directory
    $releaseDirs = Get-ChildItem -Path $buildDir -Directory -Filter "Release*" -ErrorAction SilentlyContinue
    
    if ($releaseDirs) {
        $releaseDir = $releaseDirs[0]
        $driverPath = Join-Path $releaseDir.FullName "libOdbcFb.so"
    }
    
    if (-not $driverPath -or -not (Test-Path $driverPath)) {
        Write-Error "Driver not found. Please build the driver first."
        Write-Host "Expected location: $buildDir/Release_*/libOdbcFb.so" -ForegroundColor Yellow
        exit 1
    }
    
    Write-Host "Found driver: $driverPath" -ForegroundColor Green
    
    # Determine installation directory
    $installDir = if (Test-Path "/usr/lib/x86_64-linux-gnu") {
        "/usr/lib/x86_64-linux-gnu"
    }
    elseif (Test-Path "/usr/lib64") {
        "/usr/lib64"
    }
    elseif (Test-Path "/usr/lib/unixODBC") {
        "/usr/lib/unixODBC"
    }
    else {
        "/usr/lib"
    }
    
    $targetPath = Join-Path $installDir "libOdbcFb_SUT.so"
    
    Write-Host "Copying driver to: $targetPath" -ForegroundColor Gray
    
    # Copy with sudo
    $result = sudo cp $driverPath $targetPath 2>&1
    
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to copy driver. Error: $result"
        exit 1
    }
    
    Write-Host "✓ Driver copied successfully" -ForegroundColor Green
    
    # Create driver configuration
    $tempIni = [System.IO.Path]::GetTempFileName()
    $iniContent = @"
[$DriverName]
Description = Firebird ODBC driver (System Under Test)
Driver = $targetPath
Setup = $targetPath
Threading = 0
FileUsage = 0
"@
    
    Set-Content -Path $tempIni -Value $iniContent
    
    Write-Host "Registering driver with odbcinst..." -ForegroundColor Gray
    
    # Register the driver
    $result = odbcinst -i -d -f $tempIni 2>&1
    
    Remove-Item -Path $tempIni -Force
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ Driver registered successfully" -ForegroundColor Green
    }
    else {
        Write-Error "Failed to register driver. Error: $result"
        exit 1
    }
    
    Write-Host ""
    Write-Host "✓ Installation complete!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Test connection string:" -ForegroundColor Cyan
    Write-Host "Driver={$DriverName};Database=/path/to/database.fdb;UID=SYSDBA;PWD=masterkey" -ForegroundColor Gray
}

function Uninstall-LinuxDriver {
    param(
        [string]$DriverName
    )
    
    Write-Host "Uninstalling ODBC driver on Linux..." -ForegroundColor Cyan
    Write-Host "Driver Name: $DriverName" -ForegroundColor Gray
    
    # Check if odbcinst is available
    $odbcinst = Get-Command odbcinst -ErrorAction SilentlyContinue
    if (-not $odbcinst) {
        Write-Error "odbcinst not found."
        exit 1
    }
    
    # Unregister the driver
    Write-Host "Unregistering driver..." -ForegroundColor Gray
    $result = odbcinst -u -d -n $DriverName 2>&1
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ Driver unregistered successfully" -ForegroundColor Green
    }
    else {
        Write-Warning "Driver may not have been registered. Error: $result"
    }
    
    # Remove the .so file
    $possibleDirs = @(
        "/usr/lib/x86_64-linux-gnu",
        "/usr/lib64",
        "/usr/lib/unixODBC",
        "/usr/lib"
    )
    
    foreach ($dir in $possibleDirs) {
        $targetPath = Join-Path $dir "libOdbcFb_SUT.so"
        if (Test-Path $targetPath) {
            Write-Host "Removing: $targetPath" -ForegroundColor Gray
            sudo rm -f $targetPath
            if ($LASTEXITCODE -eq 0) {
                Write-Host "✓ Driver file removed" -ForegroundColor Green
            }
        }
    }
    
    Write-Host ""
    Write-Host "✓ Uninstallation complete!" -ForegroundColor Green
}

#endregion

#region Main Execution

if ($IsWindowsOS) {
    if ($Install) {
        Install-WindowsDriver -Platform $Platform -DriverName $DriverName
    }
    else {
        Uninstall-WindowsDriver -DriverName $DriverName
    }
}
elseif ($IsLinuxOS) {
    if ($Install) {
        Install-LinuxDriver -DriverName $DriverName
    }
    else {
        Uninstall-LinuxDriver -DriverName $DriverName
    }
}

#endregion
