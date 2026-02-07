# Firebird ODBC Driver - Build and Test Script for Windows
# This script builds the driver, sets up a test database, and runs tests

param(
    [switch]$SkipBuild,
    [switch]$SkipTests,
    [string]$BuildType = "Release"
)

$ErrorActionPreference = "Stop"

Write-Host "=== Firebird ODBC Driver Build and Test ===" -ForegroundColor Cyan
Write-Host ""

# Check for CMake
try {
    $cmakeVersion = cmake --version
    Write-Host "✓ CMake found" -ForegroundColor Green
} catch {
    Write-Host "✗ CMake not found. Please install CMake 3.15 or later." -ForegroundColor Red
    exit 1
}

# Build
if (-not $SkipBuild) {
    Write-Host ""
    Write-Host "Building project..." -ForegroundColor Cyan
    
    # Configure
    Write-Host "Configuring CMake..." -ForegroundColor Yellow
    cmake -B build -DCMAKE_BUILD_TYPE=$BuildType -DBUILD_TESTING=ON
    if ($LASTEXITCODE -ne 0) {
        Write-Host "✗ CMake configuration failed" -ForegroundColor Red
        exit 1
    }
    
    # Build
    Write-Host "Building..." -ForegroundColor Yellow
    cmake --build build --config $BuildType
    if ($LASTEXITCODE -ne 0) {
        Write-Host "✗ Build failed" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "✓ Build completed successfully" -ForegroundColor Green
    
    # Check if driver was built
    $driverPath = "build\$BuildType\FirebirdODBC.dll"
    if (Test-Path $driverPath) {
        Write-Host "✓ Driver built at: $driverPath" -ForegroundColor Green
    } else {
        Write-Host "✗ Driver not found at expected location: $driverPath" -ForegroundColor Red
        exit 1
    }
}

# Setup tests
if (-not $SkipTests) {
    Write-Host ""
    Write-Host "Setting up test environment..." -ForegroundColor Cyan
    
    # Check for PSFirebird
    Write-Host "Checking for PSFirebird module..." -ForegroundColor Yellow
    if (-not (Get-Module -ListAvailable -Name PSFirebird)) {
        Write-Host "PSFirebird not found. Installing..." -ForegroundColor Yellow
        
        Set-PSRepository -Name PSGallery -InstallationPolicy Trusted
        Install-Module -Name PowerShellGet -Force -AllowClobber -Scope CurrentUser -ErrorAction SilentlyContinue
        
        try {
            Install-PackageProvider -Name NuGet -MinimumVersion 2.8.5.201 -Force -Scope CurrentUser -ForceBootstrap -ErrorAction Stop
        } catch {
            Write-Host "NuGet provider install warning: $($_.Exception.Message)" -ForegroundColor Yellow
        }
        
        Install-Module -Name PSFirebird -Force -AllowClobber -Scope CurrentUser -Repository PSGallery
        Write-Host "✓ PSFirebird installed" -ForegroundColor Green
    } else {
        Write-Host "✓ PSFirebird already installed" -ForegroundColor Green
    }
    
    # Create test database
    Write-Host "Creating test database..." -ForegroundColor Yellow
    
    $fbVersion = '5.0.2'
    $envPath = 'C:\fbodbc-tests\fb502'
    $dbPath = '/fbodbc-tests/TEST.FB50.FDB'
    
    New-Item -ItemType Directory -Path 'C:\fbodbc-tests' -Force | Out-Null
    
    Import-Module PSFirebird
    
    try {
        $fb = New-FirebirdEnvironment -Version $fbVersion -Path $envPath -Force
        Write-Host "✓ Firebird environment created at: $envPath" -ForegroundColor Green
        
        $db = New-FirebirdDatabase -Database $dbPath -Environment $fb -Force
        Write-Host "✓ Test database created at: $dbPath" -ForegroundColor Green
    } catch {
        Write-Host "✗ Failed to create Firebird environment/database: $($_.Exception.Message)" -ForegroundColor Red
        exit 1
    }
    
    # Set connection string
    $connStr = "Driver={Firebird ODBC Driver};Database=/fbodbc-tests/TEST.FB50.FDB;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8;CLIENT=C:/fbodbc-tests/fb502/fbclient.dll"
    $env:FIREBIRD_ODBC_CONNECTION = $connStr
    Write-Host "✓ Connection string set" -ForegroundColor Green
    
    # Register ODBC driver (requires admin privileges)
    Write-Host ""
    Write-Host "Registering ODBC driver..." -ForegroundColor Yellow
    Write-Host "Note: This requires administrator privileges" -ForegroundColor Yellow
    
    $driverPath = Join-Path (Get-Location) "build\$BuildType\FirebirdODBC.dll"
    $driverPath = $driverPath -replace '\\', '\\'
    
    try {
        $regPath = "HKLM:\SOFTWARE\ODBC\ODBCINST.INI\Firebird ODBC Driver"
        if (!(Test-Path $regPath)) {
            New-Item -Path "HKLM:\SOFTWARE\ODBC\ODBCINST.INI" -Name "Firebird ODBC Driver" -Force | Out-Null
        }
        
        Set-ItemProperty -Path $regPath -Name "Driver" -Value $driverPath -Type String -Force
        Set-ItemProperty -Path $regPath -Name "Setup" -Value $driverPath -Type String -Force
        Set-ItemProperty -Path $regPath -Name "APILevel" -Value "1" -Type String -Force
        Set-ItemProperty -Path $regPath -Name "ConnectFunctions" -Value "YYY" -Type String -Force
        Set-ItemProperty -Path $regPath -Name "DriverODBCVer" -Value "03.51" -Type String -Force
        Set-ItemProperty -Path $regPath -Name "FileUsage" -Value "0" -Type String -Force
        Set-ItemProperty -Path $regPath -Name "SQLLevel" -Value "1" -Type String -Force
        
        $driversPath = "HKLM:\SOFTWARE\ODBC\ODBCINST.INI\ODBC Drivers"
        if (!(Test-Path $driversPath)) {
            New-Item -Path "HKLM:\SOFTWARE\ODBC\ODBCINST.INI" -Name "ODBC Drivers" -Force | Out-Null
        }
        Set-ItemProperty -Path $driversPath -Name "Firebird ODBC Driver" -Value "Installed" -Type String -Force
        
        Write-Host "✓ ODBC driver registered" -ForegroundColor Green
    } catch {
        Write-Host "✗ Failed to register ODBC driver: $($_.Exception.Message)" -ForegroundColor Red
        Write-Host "  You may need to run this script as Administrator" -ForegroundColor Yellow
        Write-Host "  Tests may fail if driver is not registered" -ForegroundColor Yellow
    }
    
    # Run tests
    Write-Host ""
    Write-Host "Running tests..." -ForegroundColor Cyan
    
    Push-Location build
    try {
        ctest -C $BuildType --output-on-failure --verbose
        if ($LASTEXITCODE -eq 0) {
            Write-Host ""
            Write-Host "✓ All tests passed!" -ForegroundColor Green
        } else {
            Write-Host ""
            Write-Host "✗ Some tests failed" -ForegroundColor Red
            exit 1
        }
    } finally {
        Pop-Location
    }
}

Write-Host ""
Write-Host "=== Complete ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Driver location: build\$BuildType\FirebirdODBC.dll" -ForegroundColor Green
Write-Host "Test database: /fbodbc-tests/TEST.FB50.FDB" -ForegroundColor Green
Write-Host ""
Write-Host "To manually run tests:" -ForegroundColor Yellow
Write-Host "  cd build" -ForegroundColor White
Write-Host "  ctest -C $BuildType" -ForegroundColor White
Write-Host ""
