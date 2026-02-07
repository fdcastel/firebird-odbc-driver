# Quick Start Guide - CMake Build

## Build the Project

### Windows (PowerShell)
```powershell
# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# The driver will be at: build\Release\FirebirdODBC.dll
```

### Linux
```bash
# Install dependencies
sudo apt-get install -y unixodbc-dev firebird-dev

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -- -j$(nproc)

# The driver will be at: build/libOdbcFb.so
```

## Run Tests

### Windows (PowerShell)
```powershell
# Set up test database using PSFirebird
Set-PSRepository -Name PSGallery -InstallationPolicy Trusted
Install-Module -Name PSFirebird -Force -Scope CurrentUser

# Create test database
$fb = New-FirebirdEnvironment -Version '5.0.2' -Path '/fbodbc-tests/fb502' -Force
$db = New-FirebirdDatabase -Database '/fbodbc-tests/TEST.FB50.FDB' -Environment $fb -Force

# Set connection string
$env:FIREBIRD_ODBC_CONNECTION='Driver={Firebird ODBC Driver};Database=/fbodbc-tests/TEST.FB50.FDB;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8;CLIENT=C:/fbodbc-tests/fb502/fbclient.dll'

# Register driver (requires admin)
# ... see BUILD_CMAKE.md for details

# Run tests
cd build
ctest -C Release --output-on-failure
```

### Linux
```bash
# Install PowerShell (if not already installed)
wget -q https://packages.microsoft.com/config/ubuntu/$(lsb_release -rs)/packages-microsoft-prod.deb
sudo dpkg -i packages-microsoft-prod.deb
sudo apt-get update
sudo apt-get install -y powershell

# Setup Firebird using PSFirebird (embedded mode - no server needed)
pwsh -Command "
  Set-PSRepository -Name PSGallery -InstallationPolicy Trusted
  Install-Module -Name PSFirebird -Force -Scope CurrentUser
  Import-Module PSFirebird
  
  \$fb = New-FirebirdEnvironment -Version '5.0.2' -Path '/fbodbc-tests/fb502' -Force
  \$db = New-FirebirdDatabase -Database '/fbodbc-tests/TEST.FB50.FDB' -Environment \$fb -Force
"

# Find the client library from PSFirebird
FB_CLIENT=$(find /fbodbc-tests/fb502 -name "libfbclient.so*" | head -n 1)

# Set connection string (using embedded Firebird)
export FIREBIRD_ODBC_CONNECTION="Driver={Firebird ODBC Driver};Database=/fbodbc-tests/TEST.FB50.FDB;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8;CLIENT=$FB_CLIENT"

# Register driver
sudo mkdir -p /usr/local/lib/odbc
sudo cp build/libOdbcFb.so /usr/local/lib/odbc/

echo "[Firebird ODBC Driver]
Driver = /usr/local/lib/odbc/libOdbcFb.so
Setup = /usr/local/lib/odbc/libOdbcFb.so
FileUsage = 1" | sudo tee /etc/odbcinst.ini

# Run tests
cd build
ctest --output-on-failure
```

## Full Documentation

See [BUILD_CMAKE.md](BUILD_CMAKE.md) for complete documentation.
