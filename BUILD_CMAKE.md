# Firebird ODBC Driver - CMake Build System

This project has been converted to use CMake as a modern, cross-platform build system.

## Prerequisites

### Windows
- CMake 3.15 or later
- Visual Studio 2019 or later (or MinGW)
- Windows SDK (for ODBC headers)

### Linux
- CMake 3.15 or later
- GCC or Clang
- UnixODBC development package: `sudo apt-get install unixodbc-dev`
- Firebird development package: `sudo apt-get install firebird-dev`

## Building

### Quick Start

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Run tests (optional)
cd build
ctest -C Release --output-on-failure
```

### Windows

```powershell
# Configure for Visual Studio
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Output will be in: build/Release/FirebirdODBC.dll
```

### Linux

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release -- -j$(nproc)

# Output will be in: build/libOdbcFb.so
```

## CMake Options

- `BUILD_SHARED_LIBS` - Build shared libraries (default: ON)
- `BUILD_TESTING` - Build tests (default: ON)
- `BUILD_SETUP` - Build setup/configuration library (Windows only, default: ON)

Example:
```bash
cmake -B build -DBUILD_TESTING=OFF -DBUILD_SETUP=OFF
```

## Testing

The project includes Google Test-based tests that verify ODBC functionality.

### Setting Up Tests

1. Set the environment variable `FIREBIRD_ODBC_CONNECTION` with your connection string:

**Windows (PowerShell):**
```powershell
$env:FIREBIRD_ODBC_CONNECTION='Driver={Firebird ODBC Driver};Database=/fbodbc-tests/TEST.FB50.FDB;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8;CLIENT=/fbodbc-tests/fb502/fbclient.dll'
```

**Linux (Bash):**
```bash
export FIREBIRD_ODBC_CONNECTION='Driver={Firebird ODBC Driver};Database=/fbodbc-tests/TEST.FB50.FDB;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8;CLIENT=/usr/lib/libfbclient.so'
```

2. Create a test database using PSFirebird (Windows) or isql-fb (Linux)

3. Run tests:
```bash
cd build
ctest -C Release --output-on-failure
```

Or run the test executable directly:
```bash
# Windows
./build/Release/firebird_odbc_tests.exe

# Linux
./build/tests/firebird_odbc_tests
```

## Installation

### Windows

```powershell
# Install to default location (requires admin)
cmake --install build --config Release

# Or copy manually
copy build\Release\FirebirdODBC.dll C:\Windows\System32\
```

Then register the driver using ODBC Data Source Administrator.

### Linux

```bash
# Install to default location
sudo cmake --install build

# Or install manually
sudo cp build/libOdbcFb.so /usr/local/lib/odbc/

# Register in /etc/odbcinst.ini
sudo bash -c 'cat >> /etc/odbcinst.ini << EOF
[Firebird ODBC Driver]
Description = Firebird ODBC Driver
Driver = /usr/local/lib/odbc/libOdbcFb.so
Setup = /usr/local/lib/odbc/libOdbcFb.so
FileUsage = 1
EOF'
```

## Project Structure

```
├── CMakeLists.txt              # Root CMake configuration
├── IscDbc/
│   └── CMakeLists.txt          # IscDbc library configuration
├── OdbcJdbcSetup/
│   └── CMakeLists.txt          # Setup library configuration (Windows)
├── tests/
│   ├── CMakeLists.txt          # Test configuration
│   ├── test_main.cpp           # Test entry point
│   └── test_connection.cpp     # Connection tests
├── .github/
│   └── workflows/
│       ├── build-and-test.yml  # CI workflow
│       └── release.yml         # Release workflow
```

## Continuous Integration

The project includes GitHub Actions workflows for:

1. **Build and Test** (`.github/workflows/build-and-test.yml`)
   - Runs on every push and pull request
   - Tests on Windows and Linux
   - Sets up Firebird database automatically
   - Runs all tests

2. **Release** (`.github/workflows/release.yml`)
   - Triggered when a tag like `v1.0.0` is pushed
   - Builds release binaries for Windows and Linux
   - Creates a GitHub release with artifacts

### Creating a Release

```bash
git tag v3.0.1
git push origin v3.0.1
```

This will automatically:
1. Build the driver for Windows and Linux
2. Create a GitHub release
3. Upload the binaries as release assets

## Migration from Old Build System

The old makefile-based build system is preserved in the `Builds/` directory. The new CMake system provides:

- Cross-platform support without manual configuration
- Automatic dependency detection
- Modern IDE integration (Visual Studio, CLion, etc.)
- Integrated testing with CTest
- Simplified CI/CD workflows

## Troubleshooting

### Windows: "Cannot open include file: 'windows.h'"
- Install Windows SDK through Visual Studio Installer

### Linux: "sql.h: No such file or directory"
- Install unixODBC: `sudo apt-get install unixodbc-dev`

### Tests fail with "Connection refused"
- Ensure Firebird is running
- Verify the connection string in `FIREBIRD_ODBC_CONNECTION`
- Check that the database file exists

### "Driver not found" error
- Register the ODBC driver (see Installation section)
- Verify driver path in odbcinst.ini

## Contributing

When contributing, please ensure:
1. Code builds successfully on both Windows and Linux
2. All tests pass
3. New features include appropriate tests
4. CMakeLists.txt files are updated if adding new source files

## License

See the project root for license information.
