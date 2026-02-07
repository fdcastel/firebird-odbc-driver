# Firebird ODBC Driver

An ODBC driver for [Firebird](https://firebirdsql.org/) databases. Supports Firebird 3.0, 4.0, and 5.0 on Windows and Linux (x64, ARM64).

## Features

- Full ODBC 3.51 API compliance
- Unicode support (ANSI and Wide-character ODBC entry points)
- Connection string and DSN-based connections
- Prepared statements with parameter binding
- Stored procedure execution (EXECUTE PROCEDURE)
- Catalog functions (SQLTables, SQLColumns, SQLPrimaryKeys, etc.)
- Transaction control (manual and auto-commit modes)
- Statement-level savepoint isolation (failed statements don't corrupt the transaction)
- BLOB read/write support
- Scrollable cursors (static)
- Thread-safe (connection-level locking)

## Quick Start

### Connection String

Connect using `SQLDriverConnect` with a connection string:

```
Driver={Firebird ODBC Driver};Database=localhost:C:\mydb\employee.fdb;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8
```

### Connection Parameters

| Parameter | Alias | Description | Default |
|-----------|-------|-------------|---------|
| `Driver` | — | ODBC driver name (must be `{Firebird ODBC Driver}`) | — |
| `DSN` | — | Data Source Name (alternative to Driver) | — |
| `Database` | `Dbname` | Database path (`host:path` or just `path` for local) | — |
| `UID` | `User` | Username | `SYSDBA` |
| `PWD` | `Password` | Password | — |
| `Role` | — | SQL role for the connection | — |
| `CHARSET` | `CharacterSet` | Character set for the connection | — |
| `Client` | — | Path to `fbclient.dll` / `libfbclient.so` | System default |
| `Dialect` | — | SQL dialect (1 or 3) | `3` |
| `ReadOnly` | — | Read-only transaction mode (`Y`/`N`) | `N` |
| `NoWait` | — | No-wait transaction mode (`Y`/`N`) | `N` |
| `LockTimeout` | — | Lock timeout in seconds for WAIT transactions | `0` (infinite) |
| `Quoted` | `QuotedIdentifier` | Enable quoted identifiers (`Y`/`N`) | `Y` |
| `Sensitive` | `SensitiveIdentifier` | Case-sensitive identifiers (`Y`/`N`) | `N` |
| `AutoQuoted` | `AutoQuotedIdentifier` | Automatically quote identifiers (`Y`/`N`) | `N` |
| `UseSchema` | `UseSchemaIdentifier` | Schema handling (0=none, 1=remove, 2=full) | `0` |
| `SafeThread` | — | Thread safety mode | `Y` |
| `PageSize` | — | Default page size for CREATE DATABASE | `4096` |
| `EnableWireCompression` | — | Enable wire compression (`Y`/`N`) | `N` |

### Connection String Examples

```
# Remote server with explicit client library
Driver={Firebird ODBC Driver};Database=myserver:C:\databases\mydb.fdb;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8;CLIENT=C:\Firebird\fbclient.dll

# Local database on Linux
Driver={Firebird ODBC Driver};Database=/var/db/mydb.fdb;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8

# Read-only connection with lock timeout
Driver={Firebird ODBC Driver};Database=myserver:/data/mydb.fdb;UID=SYSDBA;PWD=masterkey;ReadOnly=Y;LockTimeout=10
```

---

## Building from Source

### Prerequisites

| Requirement | Version |
|-------------|---------|
| C++ compiler | C++17 capable (MSVC 2019+, GCC 7+, Clang 5+) |
| CMake | 3.15 or later |
| ODBC headers | Windows SDK (included) or unixODBC-dev (Linux) |

No Firebird installation is needed to build — the Firebird client headers are included in the repository (`FBClient.Headers/`). At runtime, the driver loads `fbclient.dll` / `libfbclient.so` dynamically.

### Windows

```powershell
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON

# Build
cmake --build build --config Release

# The driver DLL is at: build\Release\FirebirdODBC.dll
```

### Linux

```bash
# Install ODBC development package
sudo apt-get install unixodbc-dev cmake g++

# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON

# Build
cmake --build build -- -j$(nproc)

# The driver shared object is at: build/libOdbcFb.so
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTING` | `ON` | Build the test suite |
| `BUILD_SHARED_LIBS` | `ON` | Build as shared library (DLL/SO) |
| `BUILD_SETUP` | `OFF` | Build the Windows setup/configuration dialog |

---

## Installing the Driver

### Windows

Register the driver in the ODBC Driver Manager:

```powershell
# Register using the ODBC installer API (run as Administrator)
$driverPath = "C:\path\to\FirebirdODBC.dll"

$regPath = "HKLM:\SOFTWARE\ODBC\ODBCINST.INI\Firebird ODBC Driver"
New-Item -Path "HKLM:\SOFTWARE\ODBC\ODBCINST.INI" -Name "Firebird ODBC Driver" -Force
Set-ItemProperty -Path $regPath -Name "Driver" -Value $driverPath -Type String
Set-ItemProperty -Path $regPath -Name "Setup" -Value $driverPath -Type String
Set-ItemProperty -Path $regPath -Name "DriverODBCVer" -Value "03.51" -Type String

$driversPath = "HKLM:\SOFTWARE\ODBC\ODBCINST.INI\ODBC Drivers"
Set-ItemProperty -Path $driversPath -Name "Firebird ODBC Driver" -Value "Installed" -Type String
```

### Linux

Register the driver in `/etc/odbcinst.ini`:

```ini
[Firebird ODBC Driver]
Description = Firebird ODBC Driver
Driver = /usr/local/lib/odbc/libOdbcFb.so
Setup = /usr/local/lib/odbc/libOdbcFb.so
FileUsage = 1
```

Then copy the built library:

```bash
sudo mkdir -p /usr/local/lib/odbc
sudo cp build/libOdbcFb.so /usr/local/lib/odbc/

# Verify
odbcinst -q -d
```

---

## Running the Tests

Tests use [Google Test](https://github.com/google/googletest/) and are fetched automatically via CMake.

### Without a database (unit tests only)

```bash
ctest --test-dir build --output-on-failure -C Release
```

### With a Firebird database (full integration tests)

Set the `FIREBIRD_ODBC_CONNECTION` environment variable to enable integration tests. Tests that require a database connection will be skipped (not failed) when this variable is not set.

```powershell
# PowerShell (Windows)
$env:FIREBIRD_ODBC_CONNECTION='Driver={Firebird ODBC Driver};Database=/fbodbc-tests/TEST.FB50.FDB;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8;CLIENT=C:\Firebird\fbclient.dll'
ctest --test-dir build --output-on-failure -C Release
```

```bash
# Bash (Linux)
export FIREBIRD_ODBC_CONNECTION='Driver={Firebird ODBC Driver};Database=/fbodbc-tests/TEST.FB50.FDB;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8;CLIENT=/usr/lib/libfbclient.so'
ctest --test-dir build --output-on-failure
```

### CI

The project includes GitHub Actions workflows (`.github/workflows/build-and-test.yml`) that automatically:

- Build on Windows x64 and Linux x64
- Set up a Firebird 5.0 test database using [PSFirebird](https://github.com/fdcastel/PSFirebird)
- Register the ODBC driver
- Run all tests (unit + integration)

---

## Project Structure

```
├── CMakeLists.txt          # Top-level CMake build configuration
├── Main.cpp                # ODBC ANSI entry points (SQLConnect, SQLExecDirect, etc.)
├── MainUnicode.cpp         # ODBC Unicode (W) entry points (SQLConnectW, etc.)
├── OdbcConnection.cpp/h    # ODBC connection handle implementation
├── OdbcStatement.cpp/h     # ODBC statement handle implementation
├── OdbcEnv.cpp/h           # ODBC environment handle implementation
├── OdbcDesc.cpp/h          # ODBC descriptor handle implementation
├── OdbcConvert.cpp/h       # Data type conversions between C and SQL types
├── OdbcError.cpp/h         # Diagnostic record management
├── OdbcObject.cpp/h        # Base class for all ODBC handle objects
├── SafeEnvThread.cpp/h     # Thread-safety (connection-level mutexes)
├── IscDbc/                 # Firebird client interface layer (IscConnection, IscStatement, etc.)
├── FBClient.Headers/       # Firebird client API headers (no Firebird install needed to build)
├── Headers/                # ODBC API headers for cross-platform builds
├── tests/                  # Google Test-based test suite
├── Docs/                   # Documentation and project roadmap
├── .github/workflows/      # CI/CD pipelines
└── Builds/                 # Legacy build files (Makefiles for various compilers)
```

---

## License

This project is licensed under the [Initial Developer's Public License Version 1.0 (IDPL)](Install/IDPLicense.txt).

## History

This driver has been in active development since 2001, originally created by James A. Starkey for IBPhoenix, with significant contributions from Vladimir Tsvigun, Carlos G. Alvarez, Robert Milharcic, and others. The CMake build system and CI pipeline were added in 2026.
