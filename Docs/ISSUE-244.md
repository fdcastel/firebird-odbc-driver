## Unicode Support is Fundamentally Broken (Issue #244)

### Problem Summary
The driver's Unicode implementation has critical architectural flaws that make it non-functional on Linux/Unix systems and violates ODBC Unicode specifications on all platforms.

**GitHub Issue**: [#244 - Unicode support](https://github.com/FirebirdSQL/firebird-odbc-driver/issues/244)  
**Microsoft Spec**: [ODBC Unicode Data](https://learn.microsoft.com/en-us/sql/odbc/reference/develop-app/unicode-data)

### Root Causes

#### 1. **SQLWCHAR vs wchar_t Confusion**
**Problem**: The code incorrectly assumes `SQLWCHAR` and `wchar_t` are interchangeable.
- **ODBC Spec**: `SQLWCHAR` is ALWAYS 16-bit UTF-16 (UCS-2) on all platforms
- **Reality**: `wchar_t` is 2 bytes on Windows, 4 bytes on most Unix/Linux (UTF-32)
- **Location**: [MainUnicode.cpp](MainUnicode.cpp) - Throughout, especially ConvertingString class

**Evidence**:
```cpp
// Line 88 - MainUnicode.cpp - WRONG!
lengthString = length / sizeof(wchar_t);  // Assumes wchar_t == SQLWCHAR

// Line 138 - MainUnicode.cpp - WRONG! 
len = mbstowcs((wchar_t*)unicodeString, (const char*)byteString, lengthString);
// Casts SQLWCHAR* to wchar_t* - these are different types on Linux!
```

**Impact**: Complete data corruption on Linux where `sizeof(wchar_t) == 4` but `sizeof(SQLWCHAR) == 2`.

#### 2. **Use of Locale-Dependent mbstowcs/wcstombs**
**Problem**: Using `mbstowcs()`/`wcstombs()` for UTF-16 conversion.
- **Locations**: [MainUnicode.cpp](MainUnicode.cpp#L138), [MainUnicode.cpp](MainUnicode.cpp#L188)
- **Issue**: These functions use the current locale encoding, NOT UTF-16
- **ODBC Spec**: xxxW functions MUST use UTF-16 (UCS-2), regardless of locale

**Evidence**:
```cpp
// Line 138 - MainUnicode.cpp
len = mbstowcs((wchar_t*)unicodeString, (const char*)byteString, lengthString);
// This converts from current locale to wchar_t, NOT UTF-8 to UTF-16!
```

**Impact**: 
- Characters outside current locale fail to convert
- Makes driver unusable with iusql and unixODBC on Linux
- Violates ODBC spec requirement for locale-independent Unicode

#### 3. **Hardcoded sizeof(wchar_t) == 2 Assumption**
**Problem**: Multiple buffer size calculations assume 2-byte wchar_t.
- **Location**: [MainUnicode.cpp](MainUnicode.cpp#L146) - `*realLength = (TypeRealLen)(len * 2);`

**Evidence**:
```cpp
// Line 146 - Assumes all wchar_t are 2 bytes
*realLength = (TypeRealLen)(len * 2);
```

**Impact**: Wrong buffer sizes on Linux cause buffer overflows and data corruption.

#### 4. **Missing SQL_WCHAR Type Reporting**
**Problem**: SQLDescribeColW returns SQL_CHAR/SQL_VARCHAR instead of SQL_WCHAR/SQL_WVARCHAR.
- **Location**: [OdbcStatement.cpp](OdbcStatement.cpp#L1730) - `sqlDescribeCol` implementation
- **ODBC Spec**: Unicode functions should report Unicode SQL types

**Impact**: Applications using Unicode API get incorrect type information.

#### 5. **xxxW Functions Just Wrappers Around ANSI**
**Problem**: Unicode functions convert to ANSI, call ANSI implementation, convert back.
- **Issue Description** (from #244): "They are just front-end to ANSI functions that defeats their purpose"
- **Impact**: Cannot handle characters outside ANSI codepage

### Solution Architecture

#### Phase 1: Fix Type System (Critical - Required for any Unicode support)

**1.1 Define proper SQLWCHAR handling**
```cpp
// Create new UTF-16 conversion utilities (NOT using wchar_t)
namespace Utf16Convert {
    // Convert UTF-8 (from Firebird) to UTF-16 (for ODBC)
    size_t Utf8ToUtf16(const char* utf8, SQLWCHAR* utf16, size_t utf16BufferSize);
    
    // Convert UTF-16 (from ODBC) to UTF-8 (for Firebird)
    size_t Utf16ToUtf8(const SQLWCHAR* utf16, char* utf8, size_t utf8BufferSize);
    
    // Get required buffer size for conversions
    size_t Utf8ToUtf16Length(const char* utf8);
    size_t Utf16ToUtf8Length(const SQLWCHAR* utf16);
}
```

**1.2 Remove all wchar_t usage from Unicode paths**
- Replace all `(wchar_t*)` casts with proper SQLWCHAR handling
- Remove all `mbstowcs()`/`wcstombs()` calls
- Fix buffer length calculations to use `sizeof(SQLWCHAR)` not `sizeof(wchar_t)`

**Locations to fix**:
- [MainUnicode.cpp](MainUnicode.cpp#L88) - ConvertingString constructor
- [MainUnicode.cpp](MainUnicode.cpp#L129-L138) - ConvertingString destructor
- [MainUnicode.cpp](MainUnicode.cpp#L182-L201) - convUnicodeToString
- [MbsAndWcs.cpp](MbsAndWcs.cpp) - Complete rewrite for UTF-16

#### Phase 2: Fix Connection Character Set Handling

**2.1 Force UTF-8 connection for xxxW functions**
Per aafemt's recommendation in #244:
- `SQLConnect`/`SQLDriverConnect`: Use connection charset parameter (current behavior)
- `SQLConnectW`/`SQLDriverConnectW`: Force charset to UTF-8, ignore user parameter

**2.2 Implement proper charset conversion layer**
```
Application (UTF-16) 
    ↕ (Driver xxxW APIs - use Utf16Convert utilities)
Driver (UTF-8)
    ↕ (Firebird API - uses connection charset)
Firebird Server (database charset)
```

**Locations**:
- [MainUnicode.cpp](MainUnicode.cpp#L305) - SQLConnectW
- [MainUnicode.cpp](MainUnicode.cpp#L350) - SQLDriverConnectW
- [OdbcConnection.cpp](OdbcConnection.cpp) - Connection establishment

#### Phase 3: Fix Data Type Reporting

**3.1 SQLDescribeColW must return SQL_WCHAR types**
- When called from xxxW function: Return SQL_WCHAR/SQL_WVARCHAR/SQL_WLONGVARCHAR
- When called from ANSI function: Return SQL_CHAR/SQL_VARCHAR/SQL_LONGVARCHAR

**3.2 SQLColAttributeW same type mapping**
- Add context flag to track if called from Unicode or ANSI API
- Adjust type reporting accordingly

**Locations**:
- [OdbcStatement.cpp](OdbcStatement.cpp#L1730) - sqlDescribeCol
- [OdbcStatement.cpp](OdbcStatement.cpp) - sqlColAttribute
- [MainUnicode.cpp](MainUnicode.cpp#L312) - SQLDescribeColW wrapper

#### Phase 4: Add Missing Validations

**4.1 Validate even BufferLength (already in issue #9)**
**4.2 Validate SQLWCHAR alignment**
**4.3 Add proper NULL pointer handling**

### Test Coverage (See Tests/Cases/Issue244Tests.cpp)

Comprehensive test suite created to verify:
1. ✅ SQLWCHAR is always 2 bytes (platform-independent)
2. ❌ SQLDescribeColW returns SQL_WVARCHAR (currently fails)
3. ❌ String length calculations use bytes not characters (currently fails)
4. ❌ UTF-16 data roundtrip with non-ASCII (currently fails on Linux)
5. ❌ Odd BufferLength validation (currently not enforced)
6. ❌ Unicode DSN names don't crash (currently crashes on Linux)
7. ❌ No locale dependency (currently uses mbstowcs)
8. ❌ No sizeof(wchar_t) hardcoding (currently hardcoded)
9. ✅ SQL_C_WCHAR type indicators work
10. ❌ Unicode error messages (currently corrupted on Linux)

### Implementation Priority

**CRITICAL (P0) - Breaks Linux completely**:
1. Remove wchar_t usage, use only SQLWCHAR
2. Replace mbstowcs/wcstombs with UTF-16 ↔ UTF-8 conversion
3. Fix sizeof(wchar_t) hardcoding

**HIGH (P1) - Spec compliance**:
4. Force UTF-8 for xxxW connections
5. Return SQL_WCHAR types from xxxW functions
6. Validate even BufferLength

**MEDIUM (P2) - Completeness**:
7. Comprehensive Unicode test coverage
8. Documentation and examples

### References
- [GitHub Issue #244](https://github.com/FirebirdSQL/firebird-odbc-driver/issues/244)
- [ODBC Unicode Spec](https://learn.microsoft.com/en-us/sql/odbc/reference/develop-app/unicode-data)
- [ODBC Unicode Drivers](https://learn.microsoft.com/en-us/sql/odbc/reference/develop-app/unicode)
- [Jaybird FBEncodingsTest](https://github.com/FirebirdSQL/jaybird/blob/master/src/test/org/firebirdsql/jdbc/FBEncodingsTest.java) - Reference from mrotteveel

### Platform-Specific Notes

**Windows**:
- `wchar_t` is 2 bytes (UTF-16)
- `SQLWCHAR` is 2 bytes (UTF-16)
- Accidental "works" due to size match, but still wrong (uses locale)

**Linux/Unix**:
- `wchar_t` is 4 bytes (UTF-32)
- `SQLWCHAR` is 2 bytes (UTF-16)
- **COMPLETELY BROKEN** - iusql cannot connect, unixODBC fails

**macOS**:
- `wchar_t` is 4 bytes (UTF-32)
- `SQLWCHAR` is 2 bytes (UTF-16)
- **COMPLETELY BROKEN** - same as Linux

### Migration Path

For applications currently using the broken driver:
1. Applications using ANSI APIs (SQLConnect, etc.) - No change needed
2. Applications using Unicode APIs on Windows - May see behavior changes (correct charset handling)
3. Applications using Unicode APIs on Linux - Will finally work!
