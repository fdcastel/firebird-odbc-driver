# Firebird ODBC Driver Test Suite

Modern MSTest-based test suite for the Firebird ODBC driver.

## Status

**58 tests implemented** - 53 passing (91.4% success rate)

## Quick Start

### Prerequisites
- Visual Studio 2022 with C++ MSTest components
- Firebird database server (tested with Firebird 5.0)
- Firebird ODBC driver installed and registered

### Running Tests

1. **Set the connection string environment variable:**
```powershell
$env:FIREBIRD_ODBC_CONNECTION = "Driver={Firebird ODBC Driver};Database=C:\path\to\test.fdb;UID=SYSDBA;PWD=masterkey"
```

2. **Build the tests:**
```powershell
msbuild /p:Configuration=Release /p:Platform=x64 ./Tests/OdbcTests/OdbcTests.vcxproj
```

3. **Run all tests:**
```powershell
vstest.console.exe ./Tests/OdbcTests/x64/Release/OdbcTests.dll
```

4. **Run specific test class:**
```powershell
vstest.console.exe ./Tests/OdbcTests/x64/Release/OdbcTests.dll /TestCaseFilter:"ClassName=EnvAttrsTests"
```

## Test Coverage

### Implemented Test Classes

| Test Class | Tests | Pass | Description |
|------------|-------|------|-------------|
| **EnvAttrsTests** | 3 | 3 | Environment handle and attributes |
| **ConnectAttrsTests** | 3 | 3 | Connection management and attributes |
| **InfoTests** | 7 | 4 | SQLGetInfo capabilities (3 Unicode issues) |
| **StatementAttrsTests** | 9 | 8 | Statement lifecycle and attributes |
| **FetchBindingTests** | 5 | 5 | Data fetching and column binding |
| **GetDataTests** | 4 | 4 | SQLGetData functionality |
| **CatalogTests** | 7 | 7 | Schema metadata functions |
| **DiagnosticsTests** | 5 | 5 | Error diagnostics and SQLSTATE |
| **UnicodeTests** | 4 | 4 | Unicode API functions |
| **TransactionsTests** | 4 | 4 | Transaction control |
| **ErrorMappingTests** | 7 | 6 | Error code to SQLSTATE mapping |

### Test Areas Covered

✅ **Environment Management**
- Handle allocation/deallocation
- ODBC version setting
- Environment attributes

✅ **Connection Management**  
- SQLDriverConnect
- Connection attributes (autocommit, isolation)
- Connection pooling attributes

✅ **Statement Operations**
- Prepare/Execute/Free lifecycle
- Statement attributes
- Cursor types and scrollability
- Row array operations

✅ **Data Fetching**
- SQLFetch and SQLFetchScroll
- Column binding (SQLBindCol)
- SQLGetData
- NULL value handling
- Truncation handling

✅ **Metadata/Catalog Functions**
- SQLTables
- SQLColumns
- SQLPrimaryKeys
- SQLStatistics
- SQLSpecialColumns
- Privilege queries

✅ **Diagnostics**
- SQLGetDiagRec
- SQLGetDiagField
- SQLSTATE validation
- Error message retrieval

✅ **Unicode Support**
- SQLGetInfoW
- SQLGetConnectAttrW
- SQLGetDiagRecW
- Unicode string roundtrip

✅ **Transactions**
- SQLEndTran (commit/rollback)
- Autocommit mode
- Isolation levels

✅ **Error Handling**
- Invalid SQL syntax
- Table not found
- Invalid cursor state
- Invalid descriptor index
- Parameter type validation

## Known Issues

### 1. InfoTests Unicode Buffer Issues (3 failures)
**Tests:** DriverOdbcVersion, DbmsNameAndVersion, DriverName  
**Issue:** String data truncated to first character  
**Cause:** Using SQLCHAR instead of SQLWCHAR buffers  
**Impact:** Low - driver functionality works, just test assertions fail  
**Fix:** Update tests to use SQLWCHAR buffers

### 2. Cursor Scrollable Not Supported (1 failure)
**Test:** StatementAttrsTests::CursorScrollable  
**Issue:** Driver doesn't fully support SQL_ATTR_CURSOR_SCROLLABLE attribute  
**Impact:** Low - Optional feature, most apps use forward-only cursors  
**Status:** Documented driver limitation

### 3. SQLSTATE Code Difference (1 failure)
**Test:** ErrorMappingTests::InvalidSqlSyntax  
**Issue:** Firebird returns different SQLSTATE than expected  
**Impact:** Very low - Error is still correctly reported  
**Fix:** Update test to accept Firebird's SQLSTATE

## Project Structure

```
Tests/
├── OdbcTests/
│   ├── OdbcTests.vcxproj          # MSBuild project file
│   ├── pch.h/cpp                   # Precompiled headers
│   └── x64/Release/
│       └── OdbcTests.dll           # Test assembly
├── Fixtures/
│   ├── TestBase.h/cpp              # Base test helper class
│   └── (future: SchemaFixture for test data)
├── Cases/
│   ├── EnvAttrsTests.cpp
│   ├── ConnectAttrsTests.cpp
│   ├── InfoTests.cpp
│   ├── StatementAttrsTests.cpp
│   ├── FetchBindingTests.cpp
│   ├── GetDataTests.cpp
│   ├── CatalogTests.cpp
│   ├── DiagnosticsTests.cpp
│   ├── UnicodeTests.cpp
│   ├── TransactionsTests.cpp
│   └── ErrorMappingTests.cpp
└── README.md                       # This file
```

## Configuration

### Environment Variables

- **FIREBIRD_ODBC_CONNECTION** (required)  
  Full ODBC connection string including driver, database path, credentials, and options.
  
  Example:
  ```
  Driver={Firebird ODBC Driver};Database=C:\data\test.fdb;UID=SYSDBA;PWD=masterkey;CHARSET=UTF8
  ```

### Connection String Options

Common connection string keywords:
- `Driver` - ODBC driver name (required)
- `Database` or `Dbname` - Path to database file (required)
- `UID` - Username
- `PWD` - Password  
- `CHARSET` - Character set (e.g., UTF8, NONE)
- `CLIENT` - Path to fbclient.dll (if not in system PATH)
- `ROLE` - SQL role name

## Advanced Usage

### Running Specific Tests

Filter by test name:
```powershell
vstest.console.exe OdbcTests.dll /TestCaseFilter:"Name~Transaction"
```

Filter by test class:
```powershell
vstest.console.exe OdbcTests.dll /TestCaseFilter:"ClassName=DiagnosticsTests"
```

### Viewing Detailed Output

```powershell
vstest.console.exe OdbcTests.dll --logger:"console;verbosity=detailed"
```

### Parallel Execution

```powershell
vstest.console.exe OdbcTests.dll /Parallel
```

### Generating Reports

```powershell
vstest.console.exe OdbcTests.dll --logger:"trx;LogFileName=results.trx"
```

## Development

### Adding New Tests

1. Create new .cpp file in `Tests/Cases/`
2. Include `pch.h` and `TestBase.h`
3. Define test class with `TEST_CLASS(ClassName)`
4. Implement tests with `TEST_METHOD(TestName)`
5. Use `TEST_METHOD_INITIALIZE` and `TEST_METHOD_CLEANUP` for setup/teardown
6. Add file to `OdbcTests.vcxproj`

Example:
```cpp
#include "pch.h"
#include "../Fixtures/TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace OdbcTests
{
    TEST_CLASS(MyTests)
    {
    private:
        TestBase* testBase;

    public:
        TEST_METHOD_INITIALIZE(SetUp)
        {
            testBase = new TestBase();
            testBase->SetUp();
        }

        TEST_METHOD_CLEANUP(TearDown)
        {
            if (testBase)
            {
                testBase->TearDown();
                delete testBase;
                testBase = nullptr;
            }
        }

        TEST_METHOD(MyFirstTest)
        {
            SQLRETURN rc = SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"SELECT 1 FROM RDB$DATABASE", SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Query failed");
            
            Logger::WriteMessage("✓ Test passed");
        }
    };
}
```

### TestBase Helper Functions

- `SetUp()` - Allocates handles, connects to database
- `TearDown()` - Disconnects and frees handles  
- `AssertSuccess(rc, msg)` - Asserts SQL_SUCCESS
- `AssertSuccessOrInfo(rc, msg)` - Asserts SQL_SUCCESS or SQL_SUCCESS_WITH_INFO
- `AssertSqlState(handle, type, expected)` - Validates SQLSTATE
- `GetDiagnostics(handle, type)` - Returns diagnostic messages as string

### Public Members

- `SQLHENV env` - Environment handle
- `SQLHDBC dbc` - Connection handle
- `SQLHSTMT stmt` - Statement handle

## Building for Other Platforms

### x86 (32-bit)
```powershell
msbuild /p:Configuration=Release /p:Platform=Win32 ./Tests/OdbcTests/OdbcTests.vcxproj
```

### ARM64
```powershell
msbuild /p:Configuration=Release /p:Platform=ARM64 ./Tests/OdbcTests/OdbcTests.vcxproj
```

## CI/CD Integration

### Example GitHub Actions Workflow

```yaml
- name: Build Tests
  run: |
    msbuild /p:Configuration=Release /p:Platform=x64 ./Tests/OdbcTests/OdbcTests.vcxproj

- name: Run Tests
  env:
    FIREBIRD_ODBC_CONNECTION: "Driver={Firebird ODBC Driver};Database=C:\test.fdb;UID=SYSDBA;PWD=masterkey"
  run: |
    vstest.console.exe ./Tests/OdbcTests/x64/Release/OdbcTests.dll --logger:"trx"
```

## References

- [ODBC Programmer's Reference](https://learn.microsoft.com/en-us/sql/odbc/reference/odbc-programmer-s-reference)
- [MSTest C++ Framework](https://learn.microsoft.com/en-us/visualstudio/test/microsoft-visualstudio-testtools-cppunittestframework-api-reference)
- [Firebird Documentation](https://firebirdsql.org/en/documentation/)
- [PLAN-NEW-TESTS.md](../PLAN-NEW-TESTS.md) - Detailed test plan and implementation notes
