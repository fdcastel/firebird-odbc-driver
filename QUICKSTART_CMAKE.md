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
$fb = New-FirebirdEnvironment -Version '5.0.2' -Path 'C:\fbodbc-tests\fb502' -Force
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
# Install and start Firebird
sudo apt-get install -y firebird3.0-server
sudo systemctl start firebird3.0

# Create test database
mkdir -p /tmp/fbodbc-tests
echo "CREATE DATABASE '/tmp/fbodbc-tests/TEST.FB.FDB' USER 'SYSDBA' PASSWORD 'masterkey';" > /tmp/create_db.sql
/usr/bin/isql-fb -user SYSDBA -password masterkey -i /tmp/create_db.sql

# Set connection string
export FIREBIRD_ODBC_CONNECTION='Driver={Firebird ODBC Driver};Database=/tmp/fbodbc-tests/TEST.FB.FDB;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8;CLIENT=/usr/lib/x86_64-linux-gnu/libfbclient.so'

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
