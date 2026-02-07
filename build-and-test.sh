#!/bin/bash
# Firebird ODBC Driver - Build and Test Script for Linux

set -e

BUILD_TYPE="${1:-Release}"
SKIP_BUILD=false
SKIP_TESTS=false

# Parse arguments
for arg in "$@"; do
    case $arg in
        --skip-build)
            SKIP_BUILD=true
            shift
            ;;
        --skip-tests)
            SKIP_TESTS=true
            shift
            ;;
    esac
done

echo "=== Firebird ODBC Driver Build and Test ==="
echo ""

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "✗ CMake not found. Please install CMake 3.15 or later."
    exit 1
fi
echo "✓ CMake found"

# Install dependencies
echo ""
echo "Installing dependencies..."
sudo apt-get update -qq
sudo apt-get install -y unixodbc unixodbc-dev firebird3.0-server firebird3.0-common firebird-dev
echo "✓ Dependencies installed"

# Build
if [ "$SKIP_BUILD" = false ]; then
    echo ""
    echo "Building project..."
    
    echo "Configuring CMake..."
    cmake -B build -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DBUILD_TESTING=ON
    
    echo "Building..."
    cmake --build build --config $BUILD_TYPE -- -j$(nproc)
    
    echo "✓ Build completed successfully"
    
    # Check if driver was built
    if [ -f "build/libOdbcFb.so" ]; then
        echo "✓ Driver built at: build/libOdbcFb.so"
    else
        echo "✗ Driver not found at expected location: build/libOdbcFb.so"
        exit 1
    fi
fi

# Setup tests
if [ "$SKIP_TESTS" = false ]; then
    echo ""
    echo "Setting up test environment..."
    
    # Install PowerShell if not present
    if ! command -v pwsh &> /dev/null; then
        echo "Installing PowerShell..."
        wget -q https://packages.microsoft.com/config/ubuntu/$(lsb_release -rs)/packages-microsoft-prod.deb
        sudo dpkg -i packages-microsoft-prod.deb
        sudo apt-get update
        sudo apt-get install -y powershell
        echo "✓ PowerShell installed"
    fi
    
    # Setup Firebird using PSFirebird
    echo "Setting up Firebird environment with PSFirebird..."
    pwsh -Command "
        Set-PSRepository -Name PSGallery -InstallationPolicy Trusted
        Install-Module -Name PSFirebird -Force -Scope CurrentUser
        
        \$fbVersion = '5.0.2'
        \$envPath = '/fbodbc-tests/fb502'
        \$dbPath = '/fbodbc-tests/TEST.FB50.FDB'
        
        New-Item -ItemType Directory -Path '/fbodbc-tests' -Force | Out-Null
        
        Import-Module PSFirebird
        \$fb = New-FirebirdEnvironment -Version \$fbVersion -Path \$envPath -Force
        Write-Host '✓ Firebird environment created at:' \$envPath
        
        \$db = New-FirebirdDatabase -Database \$dbPath -Environment \$fb -Force
        Write-Host '✓ Test database created at:' \$dbPath
    "
    
    if [ -f "/fbodbc-tests/TEST.FB50.FDB" ]; then
        echo "✓ Test database created"
    else
        echo "✗ Failed to create test database"
        exit 1
    fi
    
    # Find Firebird client library from PSFirebird environment
    FB_CLIENT=$(find /fbodbc-tests/fb502 -name "libfbclient.so*" 2>/dev/null | head -n 1)
    if [ -z "$FB_CLIENT" ]; then
        echo "✗ Could not find fbclient library in PSFirebird environment"
        exit 1
    fi
    echo "✓ Firebird client library: $FB_CLIENT"
    
    # Set connection string
    export FIREBIRD_ODBC_CONNECTION="Driver={Firebird ODBC Driver};Database=/fbodbc-tests/TEST.FB50.FDB;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8;CLIENT=$FB_CLIENT"
    echo "✓ Connection string set"
    
    # Register ODBC driver
    echo ""
    echo "Registering ODBC driver..."
    
    sudo mkdir -p /usr/local/lib/odbc
    sudo cp build/libOdbcFb.so /usr/local/lib/odbc/
    
    cat > /tmp/odbcinst.ini << EOF
[Firebird ODBC Driver]
Description = Firebird ODBC Driver
Driver = /usr/local/lib/odbc/libOdbcFb.so
Setup = /usr/local/lib/odbc/libOdbcFb.so
FileUsage = 1
EOF
    
    sudo cp /tmp/odbcinst.ini /etc/odbcinst.ini
    
    # Verify registration
    if odbcinst -q -d | grep -q "Firebird"; then
        echo "✓ ODBC driver registered"
    else
        echo "⚠ Warning: Driver registration could not be verified"
    fi
    
    # Run tests
    echo ""
    echo "Running tests..."
    
    cd build
    ctest -C $BUILD_TYPE --output-on-failure --verbose
    
    if [ $? -eq 0 ]; then
        echo ""
        echo "✓ All tests passed!"
    else
        echo ""
        echo "✗ Some tests failed"
        exit 1
    fi
    cd ..
fi

echo ""
echo "=== Complete ==="
echo ""
echo "Driver location: build/libOdbcFb.so"
echo "Test database: /fbodbc-tests/TEST.FB50.FDB"
echo ""
echo "To manually run tests:"
echo "  cd build"
echo "  ctest -C $BUILD_TYPE"
echo ""
