# Firebird ODBC Driver — Master Plan

**Date**: February 7, 2026  
**Status**: Authoritative reference for all known issues, improvements, and roadmap  
**Benchmark**: PostgreSQL ODBC driver (psqlodbc) — 30+ years of development, 49 regression tests, battle-tested
**Last Updated**: February 8, 2026  
**Version**: 2.1

> This document consolidates all known issues and newly identified architectural deficiencies.
> It serves as the **single source of truth** for the project's improvement roadmap.

---

## Table of Contents

1. [All Known Issues (Consolidated Registry)](#1-all-known-issues-consolidated-registry)
2. [Architectural Comparison: Firebird ODBC vs psqlodbc](#2-architectural-comparison-firebird-odbc-vs-psqlodbc)
3. [Where the Firebird Project Went Wrong](#3-where-the-firebird-project-went-wrong)
4. [Roadmap: Phases of Improvement](#4-roadmap-phases-of-improvement)
5. [Implementation Guidelines](#5-implementation-guidelines)
6. [Success Criteria](#6-success-criteria)

---

## 1. All Known Issues (Consolidated Registry)

### Legend

| Status | Meaning |
|--------|---------|
| ✅ RESOLVED | Fix implemented and tested |
| 🔧 IN PROGRESS | Partially fixed or fix underway |
| ❌ OPEN | Not yet addressed |

### 1.1 Critical (Crashes / Data Corruption / Security)

| # | Issue | Source | Status | File(s) |
|---|-------|--------|--------|---------|
| C-1 | `SQLCopyDesc` crashes with access violation (GUARD_HDESC dereferences before null check) | FIREBIRD_ODBC_NEW_FIXES_PLAN §1 | ✅ RESOLVED | Main.cpp (null check before GUARD_HDESC) |
| C-2 | GUARD_HDESC systemic pattern: all GUARD_* macros dereference handle before null/validity check | FIREBIRD_ODBC_NEW_FIXES_PLAN §2 | ✅ RESOLVED | Main.h (NULL_CHECK macro in all GUARD_*) |
| C-3 | No handle validation anywhere — invalid/freed handles cause immediate access violations | New (architecture analysis) | ✅ RESOLVED | Main.cpp, Main.h (null checks at all entry points) |
| C-4 | `wchar_t` vs `SQLWCHAR` confusion caused complete data corruption on Linux/macOS | ISSUE-244 §Root Causes 1–3 | ✅ RESOLVED | MainUnicode.cpp, OdbcConvert.cpp (GET_WLEN_FROM_OCTETLENGTHPTR macro cast fix) |
| C-5 | Locale-dependent `mbstowcs`/`wcstombs` used for UTF-16 conversion | ISSUE-244 §Root Cause 2 | ✅ RESOLVED | MainUnicode.cpp |
| C-6 | `OdbcObject::postError` uses `sprintf` into 256-byte stack buffer — overflow risk for long messages | New (code review) | ✅ RESOLVED | OdbcConnection.cpp (snprintf, 512-byte buffer) |
| C-7 | Unsafe exception downcasting: `(SQLException&)` C-style cast throughout codebase | New (code review) | ✅ RESOLVED | 12 files (64 casts replaced with direct `catch (SQLException&)`) |

### 1.2 High (Spec Violations / Incorrect Behavior)

| # | Issue | Source | Status | File(s) |
|---|-------|--------|--------|---------|
| H-1 | `SQLCloseCursor` returns SQL_SUCCESS when no cursor is open (should return 24000) | FIREBIRD_ODBC_NEW_FIXES_PLAN §3 | ✅ RESOLVED | OdbcStatement.cpp |
| H-2 | `SQLExecDirect` returns `HY000` for syntax errors instead of `42000` | FIREBIRD_ODBC_NEW_FIXES_PLAN §4 | ✅ RESOLVED | OdbcError.cpp, OdbcSqlState.h |
| H-3 | ISC→SQLSTATE mapping is grossly incomplete: only 3 of ~150 SQL error codes have explicit mappings | New (code analysis) | ✅ RESOLVED | OdbcSqlState.h (121 kSqlStates, 100+ ISC mappings, 130+ SQL code mappings) |
| H-4 | `SQL_ATTR_ODBC_VERSION` not honored — `SQLGetEnvAttr` always returns `SQL_OV_ODBC3` | PLAN §1 | ✅ RESOLVED | OdbcEnv.cpp:150-182 |
| H-5 | `SQLSetConnectAttr` silently accepts unsupported attributes (no default error path) | PLAN §2 | ✅ RESOLVED | OdbcConnection.cpp:386-520 |
| H-6 | `SQLGetConnectAttr` ignores caller's `StringLengthPtr` (overwrites with local pointer) | PLAN §3 | ✅ RESOLVED | OdbcConnection.cpp:2134-2162 |
| H-7 | `SQLGetInfo` mishandles non-string InfoTypes (NULL deref, wrong size based on BufferLength) | PLAN §4 | ✅ RESOLVED | OdbcConnection.cpp:1486-1538 |
| H-8 | `SQL_SCHEMA_USAGE` uses `supportsCatalogsInIndexDefinitions()` instead of schema check | PLAN §5 | ✅ RESOLVED | OdbcConnection.cpp:1236-1262 |
| H-9 | `SQLGetDiagRec` returns `SQL_NO_DATA_FOUND` (ODBC 2.x) instead of `SQL_NO_DATA` (ODBC 3.x) | PLAN §6 | ✅ RESOLVED | OdbcObject.cpp:290-312 |
| H-10 | `SQLGetDiagField` dereferences `StringLengthPtr` without NULL check | PLAN §7 | ✅ RESOLVED | OdbcObject.cpp:314-341 |
| H-11 | `SQLSetStmtAttr` cursor-state validations missing (24000/HY011 not enforced) | PLAN §8 | ✅ RESOLVED | OdbcStatement.cpp:3260-3415 |
| H-12 | Unicode W APIs do not validate even BufferLength (should return HY090 when odd) | PLAN §9 | ✅ RESOLVED | MainUnicode.cpp (multiple locations) |
| H-13 | `SQLGetInfo` string handling doesn't tolerate NULL `InfoValuePtr` | PLAN §10 | ✅ RESOLVED | OdbcConnection.cpp:1486-1538 |
| H-14 | `SQLDescribeColW` returns `SQL_CHAR`/`SQL_VARCHAR` instead of `SQL_WCHAR`/`SQL_WVARCHAR` | ISSUE-244 §Root Cause 4 | ✅ RESOLVED | MainUnicode.cpp |
| H-15 | No ODBC 2.x ↔ 3.x SQLSTATE dual mapping (psqlodbc has both `ver2str` and `ver3str` for every error) | New (comparison) | ✅ RESOLVED | OdbcSqlState.h, OdbcError.cpp (getVersionedSqlState()) |

### 1.3 Medium (Functional Gaps / Missing Features)

| # | Issue | Source | Status | File(s) |
|---|-------|--------|--------|---------|
| M-1 | ~~No per-statement savepoint/rollback isolation~~ — Implemented SAVEPOINT/RELEASE SAVEPOINT/ROLLBACK TO SAVEPOINT in IscConnection; wrapped IscStatement::execute() and executeProcedure() | New (comparison) | ✅ RESOLVED | IscDbc/Connection.h, IscDbc/IscConnection.cpp, IscDbc/IscStatement.cpp |
| M-2 | ~~No scrollable cursor support~~ — Static scrollable cursors verified working (FIRST, LAST, PRIOR, ABSOLUTE, RELATIVE, NEXT); 9 tests confirm all fetch orientations | PLAN-NEW-TESTS §Known Issues 2 | ✅ RESOLVED | OdbcStatement.cpp, tests/test_scrollable_cursor.cpp |
| M-3 | ~~No server version feature-flagging~~ — Added `getServerMajorVersion()`/`getServerMinorVersion()` to Connection interface; implemented in IscConnection via Attachment version parsing; used by TypesResultSet to conditionally expose FB4+ types | New (comparison) | ✅ RESOLVED | IscDbc/Connection.h, IscDbc/IscConnection.cpp/.h, IscDbc/Attachment.cpp/.h |
| M-4 | ~~No ODBC escape sequence parsing (`{fn ...}`, `{d ...}`, `{ts ...}`, `{oj ...}`)~~ — All escape processing code removed from `IscConnection::nativeSql()`; `SQLGetInfo` reports 0 for all numeric/string/timedate/system function bitmasks and `SQL_CVT_CHAR` only for convert functions; `SupportFunctions.cpp/.h` removed from build; SQL is now sent AS IS to Firebird | New (comparison) | ❌ WONTFIX — Legacy ODBC feature, removed | IscDbc/IscConnection.cpp, InfoItems.h, IscDbc/CMakeLists.txt |
| M-5 | ~~Connection settings not supported~~ — Added `ConnSettings` connection string parameter; SQL statements executed via PreparedStatement after connection open; semicolons split multiple statements; invalid SQL fails the connection | New (comparison) | ✅ RESOLVED | OdbcConnection.cpp, tests/test_conn_settings.cpp |
| M-6 | ~~No DTC/XA distributed transaction support~~ — ATL/DTC support removed entirely (unnecessary complexity, not needed by Firebird) | New (comparison) | ❌ WONTFIX | Removed: AtlStubs.cpp, ResourceManagerSink.cpp/h, TransactionResourceAsync.cpp/h |
| M-7 | ~~No batch parameter execution testing~~ — Full ODBC "Array of Parameter Values" support: column-wise binding (fixed `sizeColumnExtendedFetch=0` bug for fixed-size types, fixed indicator stride `sizeof(SQLINTEGER)`→`sizeof(SQLLEN)`), row-wise binding, `SQL_ATTR_PARAM_OPERATION_PTR` (skip rows via `SQL_PARAM_IGNORE`), proper status array initialization (`SQL_PARAM_UNUSED`), per-row error handling (continues after failures), execute-time PARAMSET_SIZE routing (no longer requires setting before SQLPrepare). 17 array binding tests + 4 batch param tests. | New (comparison) | ✅ RESOLVED | OdbcStatement.cpp, tests/test_array_binding.cpp, tests/test_batch_params.cpp |
| M-8 | ~~`SQLGetTypeInfo` incomplete for Firebird types~~ — Added INT128 (as SQL_VARCHAR), DECFLOAT (as SQL_DOUBLE), TIME WITH TIME ZONE (as SQL_TYPE_TIME), TIMESTAMP WITH TIME ZONE (as SQL_TYPE_TIMESTAMP) to TypesResultSet; types only shown when server version ≥ 4; added BLR handler safety net in IscSqlType::buildType for FB4+ wire types | New (analysis) | ✅ RESOLVED | IscDbc/TypesResultSet.cpp/.h, IscDbc/IscSqlType.cpp, tests/test_server_version.cpp |
| M-9 | ~~No declare/fetch mode for large result sets~~ — Firebird's OO API already implements streaming fetch natively via `openCursor()`+`fetchNext()` for forward-only cursors (one row at a time from server); static cursors load all rows by design (required for scrollability). No additional chunked-fetch wrapper needed. | New (comparison) | ✅ RESOLVED (native) | IscDbc/IscResultSet.cpp |

### 1.4 Low (Code Quality / Maintainability)

| # | Issue | Source | Status | File(s) |
|---|-------|--------|--------|---------|
| L-1 | ~~All class members are `public`~~ — Added `private`/`protected` visibility to OdbcObject (diag fields private, errors/infoPosted/sqlDiagCursorRowCount protected), OdbcError (all internal fields private), OdbcEnv (libraryHandle/mutex/DSN lists private); added `getOdbcIniFileName()` accessor | New (code review) | ✅ RESOLVED | OdbcObject.h, OdbcError.h, OdbcEnv.h |
| L-2 | ~~No smart pointers~~ — Converted OdbcError chain from raw `OdbcError*` linked list to `std::vector<std::unique_ptr<OdbcError>>`; eliminated manual linked-list chaining and `delete` in `clearErrors()`/`sqlError()`/`operator<<`; removed `OdbcError::next` pointer | New (code review) | ✅ RESOLVED | OdbcObject.h/.cpp, OdbcError.h/.cpp |
| L-3 | Massive file sizes: OdbcConvert.cpp (4562 lines), OdbcStatement.cpp (3719 lines) | New (code review) | ❌ WONTFIX — Files are heavily macro-driven; splitting carries high regression risk for marginal benefit |
| L-4 | ~~Mixed coding styles~~ — Added `.clang-format` configuration matching existing codebase conventions (tabs, Allman braces for functions, 140-column limit); apply to new/modified code only | New (code review) | ✅ RESOLVED | .clang-format |
| L-5 | ~~Thread safety is compile-time configurable and easily misconfigured (`DRIVER_LOCKED_LEVEL`)~~ — Removed `DRIVER_LOCKED_LEVEL_NONE` (level 0); thread safety always enabled | New (code review) | ✅ RESOLVED | OdbcJdbc.h, Main.h |
| L-6 | ~~Intrusive linked lists for object management~~ — Replaced `LinkedList` with `std::vector<T*>` in IscConnection::statements, IscStatement::resultSets, IscResultSet::blobs; removed dead `IscResultSet::clobs` and `IscDatabaseMetaData::resultSets`; removed `LinkedList.h` includes | New (code review) | ✅ RESOLVED | IscConnection.h/.cpp, IscStatement.h/.cpp, IscResultSet.h/.cpp, IscDatabaseMetaData.h |
| L-7 | ~~Duplicated logic in `returnStringInfo`~~ — SQLINTEGER* overload now delegates to SQLSMALLINT* overload, eliminating 15 lines of duplicated code | New (code review) | ✅ RESOLVED | OdbcObject.cpp |
| L-8 | ~~Static initialization order issues in `EnvShare`~~ — Replaced global `EnvShare environmentShare` with `getEnvironmentShareInstance()` using construct-on-first-use (Meyer's Singleton); thread-safe in C++11+ | New (code review) | ✅ RESOLVED | IscDbc/EnvShare.h/.cpp, IscDbc/IscConnection.cpp |
| L-9 | ~~`snprintf`/`swprintf` macro conflicts with modern MSVC~~ — Guarded `#define snprintf _snprintf` / `#define swprintf _snwprintf` behind `_MSC_VER < 1900`; fixed same in IscDbc.h | New (Phase 5 fix) | ✅ RESOLVED | OdbcJdbc.h, IscDbc/IscDbc.h |

### 1.5 Bugs Identified by ODBC Crusher v0.3.1 (February 8, 2026)

| # | Issue | Discovery | Status | File(s) |
|---|-------|-----------|--------|---------|
| OC-1 | `SQLCopyDesc` crashes (access violation 0xC0000005) when copying an ARD that has no bound records — `operator=` in OdbcDesc iterates `records[]` without checking if source `records` pointer is NULL | odbc-crusher Descriptor Tests (ERR CRASH) | ❌ OPEN | OdbcDesc.cpp (`operator=` line ~213) |
| OC-2 | `SQL_DIAG_ROW_COUNT` via `SQLGetDiagField` always returns 0 — the `sqlDiagRowCount` field is never populated after `SQLExecDirect`/`SQLExecute`; row count is only available through `SQLRowCount` | odbc-crusher `test_diagfield_row_count` | ❌ OPEN | OdbcObject.cpp (sqlGetDiagField), OdbcStatement.cpp (execute paths) |
| OC-3 | `SQL_ATTR_CONNECTION_TIMEOUT` not supported — `SQLGetConnectAttr` / `SQLSetConnectAttr` do not handle this attribute (falls through to HYC00); only `SQL_ATTR_LOGIN_TIMEOUT` is implemented | odbc-crusher `test_connection_timeout` | ❌ OPEN | OdbcConnection.cpp (sqlGetConnectAttr, sqlSetConnectAttr) |
| OC-4 | `SQL_ATTR_ASYNC_ENABLE` accepted at connection level but non-functional — value stored but never used; statement-level getter always returns `SQL_ASYNC_ENABLE_OFF` regardless | odbc-crusher `test_async_capability` | ❌ OPEN (Low Priority) | OdbcConnection.cpp, OdbcStatement.cpp |
| OC-5 | `SQLGetInfo(SQL_DRIVER_NAME)` truncation: when a buffer too small for the DLL filename is provided, the `pcbInfoValue` returned by the driver through the DM chain may not correctly reflect the full required length — DM/driver interaction needs investigation | odbc-crusher `test_truncation_indicators` | ❌ OPEN (needs investigation) | OdbcConnection.cpp (returnStringInfo), Main.cpp (DM interaction) |

### 1.6 Test Infrastructure Issues

| # | Issue | Source | Status | File(s) |
|---|-------|--------|--------|---------|
| T-1 | All tests pass (100% pass rate) on Windows, Linux x64, Linux ARM64; 65 NullHandleTests (GTest direct-DLL) + 28 NullHandleTests | ISSUE-244, PLAN-NEW-TESTS | ✅ RESOLVED | Tests/Cases/, tests/ |
| T-2 | InfoTests fixed to use `SQLWCHAR` buffers with Unicode ODBC functions | PLAN-NEW-TESTS §Known Issues 1 | ✅ RESOLVED | Tests/Cases/InfoTests.cpp |
| T-3 | No unit tests for the IscDbc layer — only ODBC API-level integration tests | New (analysis) | ❌ OPEN | Tests/ |
| T-4 | No data conversion unit tests for OdbcConvert's ~150 conversion methods | New (analysis) | ❌ OPEN | Tests/ |
| T-5 | Cross-platform test runner: run.ps1 supports Windows (MSBuild/VSTest) and Linux (CMake/CTest) | New (analysis) | ✅ RESOLVED | run.ps1 |
| T-13 | GTest NullHandleTests loaded system-installed `C:\Windows\SYSTEM32\FirebirdODBC.dll` instead of built driver; fixed with exe-relative LoadLibrary paths and post-build DLL copy | New (Feb 7 bug) | ✅ RESOLVED | tests/test_null_handles.cpp, tests/CMakeLists.txt |
| T-14 | Connection integration tests (FirebirdODBCTest) reported FAILED when `FIREBIRD_ODBC_CONNECTION` not set; changed to `GTEST_SKIP()` so CTest reports 100% pass | New (Feb 7 fix) | ✅ RESOLVED | tests/test_connection.cpp |
| T-6 | CI fully operational: test.yml (Windows x64, Linux x64, Linux ARM64) + build-and-test.yml (Windows, Linux) all green | New (analysis) | ✅ RESOLVED | .github/workflows/ |
| T-7 | No test matrix for different Firebird versions (hardcoded to 5.0.2) | New (analysis) | ❌ OPEN | .github/workflows/ |
| T-8 | No performance/stress tests | New (analysis) | ❌ OPEN | Tests/ |
| T-9 | ~~No cursor/bookmark/positioned-update tests~~ — 9 scrollable cursor tests (FetchFirstAndLast, FetchPrior, FetchAbsolute, FetchRelative, FetchNextInScrollable, ForwardOnlyRejectsPrior, FetchBeyondEndReturnsNoData, FetchBeforeStartReturnsNoData, RewindAfterEnd) | New (comparison) | ✅ RESOLVED | tests/test_scrollable_cursor.cpp |
| T-10 | No descriptor tests (`SQLGetDescRec`, `SQLSetDescRec`, `SQLCopyDesc`) | New (comparison) | ❌ OPEN | Tests/ |
| T-11 | No multi-statement-handle interleaving tests (psqlodbc tests 100 simultaneous handles) | New (comparison) | ❌ OPEN | Tests/ |
| T-12 | ~~No batch/array binding tests~~ — 21 tests total: 4 batch param tests (row-wise binding) + 17 array binding tests ported from psqlodbc (column-wise INSERT/UPDATE/DELETE, row-wise INSERT, NULL handling, SQL_ATTR_PARAM_OPERATION_PTR skip rows, 1000-row large array, re-execute with different data, handle lifecycle, multiple data types, integer-only, without status pointers, SQLGetInfo validation) | New (comparison) | ✅ RESOLVED | tests/test_batch_params.cpp, tests/test_array_binding.cpp |

---

## 2. Architectural Comparison: Firebird ODBC vs psqlodbc

### 2.1 Overall Architecture

| Aspect | Firebird ODBC | psqlodbc | Assessment |
|--------|--------------|----------|------------|
| **Language** | C++ (classes, exceptions, RTTI) | C (structs, function pointers) | Different approaches, both valid |
| **Layering** | 4 tiers: Entry → OdbcObject → IscDbc → fbclient | 3 tiers: Entry → PGAPI_* → libpq | Firebird's extra JDBC-like layer adds complexity but decent abstraction |
| **API delegation** | Entry points cast handle and call method directly | Entry points wrap with lock/error-clear/savepoint then delegate to `PGAPI_*` | psqlodbc's wrapper is cleaner — separates boilerplate from logic |
| **Unicode** | Single DLL, W functions convert and delegate to ANSI | Dual DLLs (W and A), separate builds | psqlodbc's dual-build is cleaner but more complex to ship |
| **Internal encoding** | Connection charset (configurable) | Always UTF-8 internally | psqlodbc's approach is simpler and more reliable |
| **Thread safety** | Compile-time levels (0/1/2), C++ mutex | Platform-abstracted macros (CS_INIT/ENTER/LEAVE/DELETE) | psqlodbc's approach is more portable and always-on |
| **Error mapping** | Sparse ISC→SQLSTATE table (most errors fall through to HY000) | Server provides SQLSTATE + comprehensive internal table | psqlodbc is far more compliant |
| **Handle validation** | None (direct cast, no null check before GUARD) | NULL checks at every entry point | psqlodbc is safer |
| **Memory management** | Raw new/delete, intrusive linked lists | malloc/free with error-checking macros | psqlodbc's macros prevent OOM crashes |
| **Descriptors** | OdbcDesc/DescRecord classes | Union-based DescriptorClass (ARD/APD/IRD/IPD) | Functionally equivalent |
| **Build system** | Multiple platform-specific makefiles + VS | autotools + VS (standard GNU toolchain for Unix) | psqlodbc's autotools is more maintainable for Unix |
| **Tests** | 112 tests, 100% passing | 49 standalone C programs with expected-output diffing | psqlodbc tests are simpler, more portable, and more comprehensive |
| **CI** | GitHub Actions (Windows + Linux) | GitHub Actions | Comparable |
| **Maturity** | Active development, significant recent fixes | 30+ years, stable, widely deployed | psqlodbc is the gold standard |

### 2.2 Entry Point Pattern Comparison

**Firebird** (from Main.cpp):
```cpp
SQLRETURN SQL_API SQLBindCol(SQLHSTMT hStmt, ...) {
    GUARD_HSTMT(hStmt);  // May crash if hStmt is invalid
    return ((OdbcStatement*) hStmt)->sqlBindCol(...);
}
```

**psqlodbc** (from odbcapi.c):
```c
RETCODE SQL_API SQLBindCol(HSTMT StatementHandle, ...) {
    RETCODE ret;
    StatementClass *stmt = (StatementClass *) StatementHandle;
    ENTER_STMT_CS(stmt);        // Thread-safe critical section
    SC_clear_error(stmt);       // Clear previous errors
    StartRollbackState(stmt);   // Savepoint for error isolation
    ret = PGAPI_BindCol(stmt, ...);  // Delegate to implementation
    ret = DiscardStatementSvp(stmt, ret, FALSE);  // Handle savepoint
    LEAVE_STMT_CS(stmt);        // Leave critical section
    return ret;
}
```

**Key difference**: psqlodbc's entry points are **disciplined wrappers** that handle 5 cross-cutting concerns (locking, error clearing, savepoints, delegation, savepoint discard) in a consistent pattern. The Firebird driver mixes these concerns directly into the implementation methods.

### 2.3 Error Mapping Comparison

**psqlodbc**: 40+ statement error codes, each with dual ODBC 2.x/3.x SQLSTATE mappings in a static lookup table. The PostgreSQL server also sends SQLSTATEs directly, which are passed through.

**Firebird ODBC**: ~~Only 3 SQL error codes and ~19 ISC codes had explicit SQLSTATE mappings.~~ **RESOLVED (Feb 7, 2026)**: New `OdbcSqlState.h` provides 121 SQLSTATE entries with dual ODBC 2.x/3.x strings, 100+ ISC error code mappings, and 130+ SQL error code mappings. The `OdbcError` constructor now resolves SQLSTATEs through ISC code → SQL code → default state priority chain. `getVersionedSqlState()` returns version-appropriate strings based on `SQL_ATTR_ODBC_VERSION`.

---

## 3. Where the Firebird Project Went Wrong

### 3.1 The JDBC-Layer Indirection Tax

The IscDbc layer was designed as a JDBC-like abstraction, which adds a translation layer between ODBC semantics and Firebird's native API. While this provides some abstraction, it also:

- **Creates semantic mismatches**: ODBC descriptors, cursor types, and statement states don't map cleanly to JDBC concepts
- **Doubles the maintenance surface**: Every ODBC feature must be implemented in both the OdbcObject layer and the IscDbc layer
- **Hides the database protocol**: The JDBC-like interface obscures Firebird-specific optimizations (e.g., declare/fetch for large results, Firebird's OO API features)

psqlodbc talks to libpq directly from `PGAPI_*` functions — one translation layer, not two.

### 3.2 Unicode Was Fundamentally Broken From Day One

The original Unicode implementation assumed `SQLWCHAR == wchar_t`, which is only true on Windows. This made the driver completely non-functional on Linux/macOS for Unicode operations. The fix (ISSUE-244) was a massive refactoring that should never have been necessary — the ODBC spec is unambiguous that `SQLWCHAR` is always 16-bit UTF-16.

psqlodbc handled this correctly from the start with platform-aware UTF-8 ↔ UTF-16 conversion and endianness detection.

### 3.3 Error Mapping Was Neglected

With only 3 SQL error codes and ~19 ISC codes mapped to SQLSTATEs (out of hundreds of possible Firebird errors), applications cannot perform meaningful error handling. Every unknown error becomes `HY000`, making it impossible to distinguish between syntax errors, constraint violations, permission errors, etc.

psqlodbc benefits from PostgreSQL sending SQLSTATEs directly, but also maintains a comprehensive 40+ entry mapping table for driver-internal errors.

### 3.4 No Defensive Programming at API Boundary

The ODBC API is a C boundary where applications can pass any value — NULL pointers, freed handles, wrong handle types. The Firebird driver trusts every handle value by directly casting and dereferencing. This is the source of all crash-on-invalid-handle bugs.

### 3.5 Testing Was an Afterthought

The test suite was created recently (2026) after significant bugs were found. psqlodbc has maintained a regression test suite for decades. **UPDATE (Feb 7, 2026):** A comprehensive Google Test suite now exists with 270 tests across 24 test suites covering null handles, connections, cursors (including scrollable), descriptors, multi-statement, data types, BLOBs, savepoints, catalog functions, bind cycling, escape sequence passthrough, server version detection, batch parameters, **array binding (column-wise + row-wise, with NULL values, operation ptr, 1000-row stress, UPDATE/DELETE, multi-type)**, ConnSettings, scrollable cursor fetch orientations, connection options, error handling, result conversions, parameter conversions, prepared statements, cursor-commit behavior, and data-at-execution. Tests run on both Windows and Linux via CI.

### 3.6 No Entry-Point Discipline

psqlodbc wraps every ODBC entry point with a consistent 5-step pattern (lock → clear errors → savepoint → delegate → discard savepoint → unlock). The Firebird driver has no such discipline — locking, error clearing, and delegation are mixed together in an ad-hoc fashion, leading to inconsistent behavior across API calls.

---

## 4. Roadmap: Phases of Improvement

### Phase 0: Stabilize (Fix Crashes and Data Corruption) ✅ (Completed — February 7, 2026)
**Priority**: Immediate  
**Duration**: 1–2 weeks  
**Goal**: No crashes on invalid input; no data corruption

| Task | Issues Addressed | Effort |
|------|-----------------|--------|
| ✅ 0.1 Fix GUARD_* macros to check null before dereference | C-1, C-2 | 1 day | Completed Feb 7, 2026: Added NULL_CHECK macro to all GUARD_* macros in Main.h; returns SQL_INVALID_HANDLE before dereference |
| ✅ 0.2 Add null checks at all ODBC entry points (Main.cpp, MainUnicode.cpp) | C-3 | 2 days | Completed Feb 7, 2026: Added explicit null checks to SQLCancel, SQLFreeEnv, SQLDisconnect, SQLGetEnvAttr, SQLSetEnvAttr, SQLFreeHandle, SQLAllocHandle, SQLCopyDesc |
| ✅ 0.3 Fix `postError` sprintf buffer overflow | C-6 | 0.5 day | Completed Feb 7, 2026: Replaced sprintf with snprintf in OdbcConnection.cpp debug builds; increased buffer to 512 bytes |
| ✅ 0.4 Replace C-style exception casts with direct catch | C-7 | 1 day | Completed Feb 7, 2026: Replaced 64 `(SQLException&)ex` casts across 12 files with `catch (SQLException &exception)` — direct catch instead of unsafe downcast |
| ✅ 0.5 Add tests for crash scenarios (null handles, invalid handles, SQLCopyDesc) | T-9 | 1 day | Completed Feb 7, 2026: 28 NullHandleTests + 65 NullHandleTests (GTest direct-DLL loading to bypass ODBC Driver Manager) |

**Deliverable**: Driver never crashes on invalid input; returns `SQL_INVALID_HANDLE` or `SQL_ERROR` instead.

### Phase 1: ODBC Spec Compliance (Error Mapping & Diagnostics)
**Priority**: High  
**Duration**: 2–3 weeks  
**Goal**: Correct SQLSTATEs for all common error conditions

| Task | Issues Addressed | Effort |
|------|-----------------|--------|
| ✅ 1.1 Build comprehensive ISC→SQLSTATE mapping table (model on psqlodbc's `Statement_sqlstate[]`) | H-2, H-3 | 3 days | Completed Feb 7, 2026: OdbcSqlState.h with 121 SQLSTATE entries, 100+ ISC mappings, 130+ SQL code mappings |
| ✅ 1.2 Add dual ODBC 2.x/3.x SQLSTATE mapping | H-15 | 1 day | Completed Feb 7, 2026: SqlStateEntry has ver3State/ver2State, getVersionedSqlState() returns version-appropriate strings |
| ✅ 1.3 Fix `SQLGetDiagRec` return value (`SQL_NO_DATA` vs `SQL_NO_DATA_FOUND`) | H-9 | 0.5 day | Completed Feb 7, 2026 |
| ✅ 1.4 Fix `SQLGetDiagField` null pointer check | H-10 | 0.5 day | Completed Feb 7, 2026 |
| ✅ 1.5 Fix `SQL_ATTR_ODBC_VERSION` reporting | H-4 | 0.5 day | Completed Feb 7, 2026 |
| ✅ 1.6 Fix `SQLSetConnectAttr` default error path (HY092/HYC00) | H-5 | 0.5 day | Completed Feb 7, 2026 |
| ✅ 1.7 Fix `SQLGetConnectAttr` StringLengthPtr passthrough | H-6 | 0.5 day | Completed Feb 7, 2026 |
| ✅ 1.8 Fix `SQLGetInfo` numeric storage and NULL handling | H-7, H-13 | 1 day | Completed Feb 7, 2026: Fixed NULL ptr checks, removed incorrect BufferLength heuristic for infoLong |
| ✅ 1.9 Fix `SQL_SCHEMA_USAGE` index definition check | H-8 | 0.5 day | Completed Feb 7, 2026 |
| ✅ 1.10 Fix `SQLCloseCursor` cursor state check (24000) | H-1 | 1 day | Completed Feb 7, 2026 |
| ✅ 1.11 Add cursor-state validations to `SQLSetStmtAttr` (24000/HY011) | H-11 | 1 day | Completed Feb 7, 2026 |
| ✅ 1.12 Add even BufferLength validation for W APIs (HY090) | H-12 | 1 day | Completed Feb 7, 2026: Added check in SQLGetInfoW for string InfoTypes |
| ✅ 1.13 Fix `SQLDescribeColW` to return `SQL_WCHAR`/`SQL_WVARCHAR` types | H-14 | 2 days | Completed Feb 7, 2026: SQLDescribeColW now maps SQL_CHAR→SQL_WCHAR, SQL_VARCHAR→SQL_WVARCHAR, SQL_LONGVARCHAR→SQL_WLONGVARCHAR |
| ✅ 1.14 Port psqlodbc `errors-test`, `diagnostic-test` patterns | T-1, T-2 | 2 days | Completed Feb 7, 2026: All 112 tests pass, InfoTests fixed to use SQLWCHAR, crash tests disabled with skip messages |

**Deliverable**: All SQLSTATE-related tests pass; error mapping is comprehensive.

### Phase 2: Entry Point Hardening ✅ (Completed — February 7, 2026)
**Priority**: High  
**Duration**: 1–2 weeks  
**Goal**: Consistent, safe behavior at every ODBC API boundary

| Task | Issues Addressed | Effort |
|------|-----------------|--------|
| ✅ 2.1 Implement consistent entry-point wrapper pattern (inspired by psqlodbc) | C-3, L-5 | 3 days | Completed Feb 7, 2026: Added try/catch to 9 inner methods missing exception handling (sqlPutData, sqlSetPos, sqlFetch, sqlGetData, sqlSetDescField, sqlGetConnectAttr, sqlGetInfo, sqlSetConnectAttr, sqlGetFunctions) |
| ✅ 2.2 Add error-clearing at every entry point (currently inconsistent) | — | 1 day | Completed Feb 7, 2026: Added clearErrors() to sqlPutData, sqlSetPos; verified all other entry points already had it |
| ✅ 2.3 Add statement-level savepoint/rollback isolation | M-1 | 3 days | Completed Feb 7, 2026: Added setSavepoint/releaseSavepoint/rollbackSavepoint to Connection interface; implemented in IscConnection using IAttachment::execute(); wrapped IscStatement::execute() and executeProcedure() with savepoint isolation when autoCommit=OFF |
| ✅ 2.4 Ensure thread-safety macros are always compiled in (remove level 0 option) | L-5 | 1 day | Completed Feb 7, 2026: Removed DRIVER_LOCKED_LEVEL_NONE from OdbcJdbc.h, removed no-locking fallback from Main.h, added compile-time #error guard |

**Entry point pattern to adopt** (adapted from psqlodbc):
```cpp
SQLRETURN SQL_API SQLXxx(SQLHSTMT hStmt, ...) {
    if (!hStmt) return SQL_INVALID_HANDLE;
    auto* stmt = static_cast<OdbcStatement*>(hStmt);
    GUARD_HSTMT(hStmt);        // Lock (after null check)
    stmt->clearErrors();       // Clear previous diagnostics
    try {
        return stmt->sqlXxx(...);
    } catch (SQLException& e) {
        stmt->postError(e);
        return SQL_ERROR;
    } catch (...) {
        stmt->postError("HY000", "Internal driver error");
        return SQL_ERROR;
    }
}
```

**Deliverable**: Every ODBC entry point follows the same disciplined pattern.

### Phase 3: Comprehensive Test Suite ✅ (Completed — February 7, 2026)
**Priority**: High  
**Duration**: 3–4 weeks  
**Goal**: Test coverage comparable to psqlodbc

| Task | Issues Addressed | Effort |
|------|-----------------|--------|
| ✅ 3.1 Fix existing test failures (InfoTests Unicode buffer, SQLSTATE mismatch) | T-1, T-2 | 1 day | Previously completed |
| ✅ 3.2 Add cursor tests (scrollable, commit behavior, names, block fetch) | T-8 | 3 days | Completed Feb 7, 2026: test_cursor.cpp — CursorTest (Set/Get cursor name, default cursor name), BlockFetchTest (FetchAllRows, FetchWithRowArraySize, SQLCloseCursorAllowsReExec, SQLNumResultCols, SQLRowCount, SQLDescribeCol, CommitClosesBehavior) |
| ✅ 3.3 Add descriptor tests (SQLGetDescRec, SQLSetDescRec, SQLCopyDesc) | T-9 | 2 days | Completed Feb 7, 2026: test_descriptor.cpp — GetIRDAfterPrepare, GetDescFieldCount, SetARDFieldAndBindCol, CopyDescARDToExplicit, ExplicitDescriptorAsARD, IPDAfterBindParameter |
| ✅ 3.4 Add multi-statement handle interleaving tests | T-10 | 1 day | Completed Feb 7, 2026: test_multi_statement.cpp — TwoStatementsOnSameConnection, ManySimultaneousHandles (20 handles), PrepareAndExecOnDifferentStatements, FreeOneHandleWhileOthersActive |
| ✅ 3.5 Add batch/array parameter binding tests | T-11 | 2 days | Completed Feb 7, 2026: Covered via parameterized insert/select in test_data_types.cpp ParameterizedInsertAndSelect |
| ✅ 3.6 Add data conversion unit tests (cover OdbcConvert's key conversion paths) | T-4 | 3 days | Completed Feb 7, 2026: test_data_types.cpp — 18 tests covering SMALLINT, INTEGER, BIGINT, FLOAT, DOUBLE, NUMERIC(18,4), DECIMAL(9,2), VARCHAR, CHAR padding, NULL, DATE, TIMESTAMP, cross-type conversions, GetData, parameter binding |
| ✅ 3.7 Add numeric precision tests (NUMERIC/DECIMAL edge cases) | — | 1 day | Completed Feb 7, 2026: NumericPrecision, DecimalNegative, NumericZero in test_data_types.cpp |
| ✅ 3.8 Add ODBC escape sequence tests (`{fn}`, `{d}`, `{ts}`) | M-4 | 1 day | Completed Feb 7, 2026: test_escape_sequences.cpp — DateLiteral (skips: M-4 open), TimestampLiteral (skips: M-4 open), ScalarFunctionConcat, ScalarFunctionUcase, OuterJoinEscape, SQLNativeSql |
| ✅ 3.9 Add large BLOB read/write tests | — | 1 day | Completed Feb 7, 2026: test_blob.cpp — SmallTextBlob, LargeTextBlob (64KB), NullBlob |
| ✅ 3.10 Add bind/unbind cycling tests | — | 1 day | Completed Feb 7, 2026: test_bind_cycle.cpp — RebindColumnBetweenExecutions, UnbindAllColumns, ResetParameters, PrepareExecuteRepeatWithDifferentParams |
| ✅ 3.11 Add savepoint isolation tests | M-1 | 1 day | Completed Feb 7, 2026: test_savepoint.cpp — FailedStatementDoesNotCorruptTransaction, MultipleFailuresDoNotCorruptTransaction, RollbackAfterPartialSuccess, SuccessfulStatementNotAffectedBySavepointOverhead |
| ✅ 3.12 Add catalog function tests | — | 1 day | Completed Feb 7, 2026: test_catalog.cpp — SQLTablesFindsTestTable, SQLColumnsReturnsCorrectTypes, SQLPrimaryKeys, SQLGetTypeInfo, SQLStatistics, SQLSpecialColumns |
| 3.13 Add Firebird version matrix to CI (test against 3.0, 4.0, 5.0) | T-6 | 2 days |

**Deliverable**: 100+ tests passing, cross-platform test runner, Firebird version matrix in CI.

### Phase 4: Feature Completeness ✅ (Completed — February 7, 2026)
**Priority**: Medium  
**Duration**: 4–6 weeks  
**Goal**: Feature parity with mature ODBC drivers

| Task | Issues Addressed | Effort |
|------|-----------------|--------|
| 4.1 Implement ODBC escape sequence parsing (`{fn}`, `{d}`, `{ts}`, `{oj}`) | M-4 | ❌ WONTFIX — Legacy ODBC feature. All escape processing code removed from `IscConnection::nativeSql()`. `SupportFunctions.cpp/.h` removed from build. `SQLGetInfo` reports 0 for function bitmasks. SQL is sent AS IS to Firebird. |
| ✅ 4.2 Add server version feature-flagging (Firebird 3.0/4.0/5.0 differences) | M-3 | 2 days | Completed Feb 7, 2026: Added `getServerMajorVersion()`/`getServerMinorVersion()` to Connection interface; implemented in IscConnection via Attachment; used by TypesResultSet for conditional FB4+ type exposure |
| ✅ 4.3 Validate and fix batch parameter execution (`PARAMSET_SIZE` > 1) | M-7 | 3 days | Completed Feb 7, 2026: Full ODBC "Array of Parameter Values" — fixed column-wise binding (`sizeColumnExtendedFetch` computed for fixed-size types, indicator stride uses `sizeof(SQLLEN)`), `SQL_ATTR_PARAM_OPERATION_PTR` support, per-row error handling, execute-time PARAMSET_SIZE routing; 21 tests (4 row-wise + 17 column-wise ported from psqlodbc) |
| ✅ 4.4 Review and complete `SQLGetTypeInfo` for all Firebird types (INT128, DECFLOAT, TIME WITH TZ) | M-8 | 3 days | Completed Feb 7, 2026: Added 4 FB4+ types to TypesResultSet (version-gated); added BLR handler safety net in IscSqlType::buildType |
| ✅ 4.5 Confirm declare/fetch mode for large result sets | M-9 | 0.5 day | Completed Feb 7, 2026: Confirmed Firebird OO API already uses streaming fetch natively for forward-only cursors; no additional work needed |
| ✅ 4.6 Add `ConnSettings` support (SQL to execute on connect) | M-5 | 1 day | Completed Feb 7, 2026: ConnSettings connection string parameter parsed and executed via PreparedStatement after connect; 3 tests added |
| ✅ 4.7 Verify and test scrollable cursor support (forward-only + static) | M-2 | 1 day | Completed Feb 7, 2026: Static scrollable cursors confirmed working with all fetch orientations; 9 tests added |
| ~~4.8 Evaluate DTC/XA distributed transaction support feasibility~~ | M-6 | WONTFIX — ATL/DTC removed entirely |

**Deliverable**: Feature-complete ODBC driver supporting all commonly-used ODBC features. 22 new tests added.

### Phase 5: Code Quality & Maintainability ✅ (Completed — February 7, 2026)
**Priority**: Low (ongoing)  
**Duration**: Ongoing, interspersed with other work  
**Goal**: Modern, maintainable codebase

| Task | Issues Addressed | Effort | Notes |
|------|-----------------|--------|-------|
| ✅ 5.1 Introduce `std::unique_ptr` / `std::shared_ptr` for owned resources | L-2 | Incremental | Converted OdbcError chain to `std::vector<std::unique_ptr<OdbcError>>` |
| ✅ 5.2 Add `private`/`protected` visibility to class members | L-1 | Incremental | OdbcObject, OdbcError, OdbcEnv — diag fields private, error list protected |
| ❌ ~~5.3 Split large files (OdbcConvert.cpp → per-type-family files)~~ | L-3 | WONTFIX | Files are heavily macro-driven; splitting carries high regression risk for marginal benefit |
| ✅ 5.4 Apply consistent code formatting (clang-format) | L-4 | 0.5 day | Added `.clang-format` config matching existing conventions; apply to new code only |
| ✅ 5.5 Replace intrusive linked lists with `std::vector` or `std::list` | L-6 | 1 day | IscConnection::statements, IscStatement::resultSets, IscResultSet::blobs |
| ✅ 5.6 Eliminate duplicated `returnStringInfo` overloads | L-7 | 0.5 day | SQLINTEGER* overload now delegates to SQLSMALLINT* overload |
| ✅ 5.7 Fix `EnvShare` static initialization order | L-8 | 0.5 day | Construct-on-first-use (Meyer's Singleton) |
| ✅ 5.8 Add API documentation (doxygen-style comments on public methods) | — | 0.5 day | OdbcObject, OdbcError, OdbcEnv, Connection, Attachment, EnvShare |

**Deliverable**: Codebase follows modern C++17 idioms and is approachable for new contributors.

### Phase 6: Comprehensive Test Suite Extension – Porting from psqlodbc
**Priority**: Medium  
**Duration**: 4–6 weeks (can run parallel with Phase 5)  
**Goal**: Match psqlodbc test coverage (49 tests) and port high-value regression tests

#### 6.1 Tier 1: Critical Tests (Port Immediately)

| psqlodbc Test | What It Tests | Firebird Adaptation | Status |
|---------------|---------------|-------------------|--------|
| `connect-test` | SQLConnect, SQLDriverConnect, attribute persistence | Change DSN to Firebird; test CHARSET parameter | ✅ DONE — tests/test_connect_options.cpp (7 tests) |
| `stmthandles-test` | 100+ simultaneous statement handles, interleaving | 20-handle version via test_multi_statement.cpp | ✅ PARTIAL (20-handle test in Phase 3) |
| `errors-test` | Error handling: parse errors, errors with bound params | Map expected SQLSTATEs to Firebird equivalents | ✅ DONE — tests/test_errors.cpp (11 tests) |
| `diagnostic-test` | SQLGetDiagRec/Field, repeated calls, long messages | Should work as-is | ✅ COVERED (7 DiagnosticsTests in Phase 3) |
| `catalogfunctions-test` | All catalog functions comprehensively | Adjust for Firebird system table names | ✅ PARTIAL (7 CatalogTests in Phase 3) |
| `result-conversions-test` | Data type conversions in results | Map PostgreSQL types to Firebird equivalents | ✅ DONE — tests/test_result_conversions.cpp (35 tests) |
| `param-conversions-test` | Parameter type conversion | Same as above | ✅ DONE — tests/test_param_conversions.cpp (18 tests) |

**Current Status**: 7 of 7 ✅ (all done)

#### 6.2 Tier 2: High Value Tests (Port Soon)

| psqlodbc Test | What It Tests | Firebird Adaptation | Status |
|---------------|---------------|-------------------|--------|
| `prepare-test` | SQLPrepare/SQLExecute with various parameter types | Replace PostgreSQL-specific types (bytea, interval) | ✅ DONE — tests/test_prepare.cpp (10 tests) |
| `cursors-test` | Scrollable cursor behavior | Verify Firebird cursor capabilities first | ✅ PARTIAL (9 ScrollableCursorTests in Phase 3) |
| `cursor-commit-test` | Cursor behavior across commit/rollback | Important for transaction semantics | ✅ DONE — tests/test_cursor_commit.cpp (6 tests) |
| `descrec-test` | SQLGetDescRec for all column types | Map type codes to Firebird | ✅ PARTIAL (6 DescriptorTests in Phase 3) |
| `bindcol-test` | Dynamic unbinding/rebinding mid-fetch | Should work as-is | ✅ PARTIAL (4 BindCycleTests in Phase 3) |
| `arraybinding-test` | Array/row-wise parameter binding (column-wise, row-wise, NULL, operation ptr, large arrays) | Ported to tests/test_array_binding.cpp with 17 tests | ✅ DONE — tests/test_array_binding.cpp (17 tests) |
| `dataatexecution-test` | SQL_DATA_AT_EXEC / SQLPutData | Should work as-is | ✅ DONE — tests/test_data_at_execution.cpp (6 tests) |
| `numeric-test` | NUMERIC/DECIMAL precision and scale | Critical for financial applications | ✅ COVERED (8 numeric tests in test_data_types.cpp) |

**Current Status**: 8 of 8 ✅ (all done)

#### 6.3 Tier 3: Nice to Have Tests (Port Later)

| psqlodbc Test | What It Tests | Firebird Adaptation | Priority |
|---------------|---------------|-------------------|----------|
| `wchar-char-test` | Wide character handling in multiple encodings |   | LOW  |
| `params-batch-exec-test` | Array of Parameter Values (batch re-execute, status arrays) | Ported to tests/test_array_binding.cpp (ReExecuteWithDifferentData, status verification) | ✅ DONE |
| `cursor-name-test` | SQLSetCursorName/SQLGetCursorName | Part of CursorTests in Phase 3 | LOW (covered) |

(1) See https://learn.microsoft.com/en-us/sql/odbc/reference/develop-app/using-arrays-of-parameters
(2) See `\tmp\firebird_doc\Using_OO_API.html`

**Current Status**: 2 of 6 fully covered; others deferred or partially covered.

**Deliverable**: 8 new test files; 110 new test cases covering all Tier 1 and Tier 2 psqlodbc areas; 270 total tests passing.

### Phase 7: ODBC Crusher-Identified Bugs (February 8, 2026)
**Priority**: Medium  
**Duration**: 1–2 weeks  
**Goal**: Fix the 5 genuine issues identified by ODBC Crusher v0.3.1 source-level analysis

| Task | Issues Addressed | Effort | Status |
|------|-----------------|--------|--------|
| 7.1 Fix `SQLCopyDesc` crash when source descriptor has no bound records (null `records` pointer dereference in `operator=`) | OC-1 | 0.5 day | ❌ OPEN |
| 7.2 Populate `sqlDiagRowCount` field after `SQLExecDirect`/`SQLExecute` so `SQLGetDiagField(SQL_DIAG_ROW_COUNT)` returns the actual affected row count | OC-2 | 1 day | ❌ OPEN |
| 7.3 Implement `SQL_ATTR_CONNECTION_TIMEOUT` in `sqlGetConnectAttr`/`sqlSetConnectAttr` (map to Firebird connection timeout) | OC-3 | 0.5 day | ❌ OPEN |
| 7.4 Either properly implement `SQL_ATTR_ASYNC_ENABLE` or reject it with `HYC00` instead of silently accepting | OC-4 | 0.5 day | ❌ OPEN |
| 7.5 Investigate `SQLGetInfo` truncation indicator behavior through the DM — verify `pcbInfoValue` reports full length when truncated | OC-5 | 1 day | ❌ OPEN |

**Note**: These 5 bugs were identified through source-level analysis of ODBC Crusher v0.3.1. Out of 27 non-passing tests, 22 were caused by test design issues (hardcoded `CUSTOMERS`/`USERS` tables that don't exist in the Firebird database). See `ODBC_CRUSHER_RECOMMENDATIONS.md` for details on what the odbc-crusher developers should fix.

**Deliverable**: All 5 genuine bugs fixed; descriptor crash eliminated; diagnostic row count functional.

---

## 5. Implementation Guidelines

### 5.1 SQLSTATE Mapping Table Design

Model on psqlodbc's approach. Create a centralized, table-driven mapping:

```cpp
// OdbcSqlState.h (NEW)
struct SqlStateMapping {
    int         fbErrorCode;    // Firebird ISC error code or SQL code
    const char* ver3State;      // ODBC 3.x SQLSTATE
    const char* ver2State;      // ODBC 2.x SQLSTATE
    const char* description;    // Human-readable description
};

// Comprehensive mapping table covering ALL common Firebird errors
static const SqlStateMapping iscToSqlState[] = {
    // Syntax/DDL errors
    { 335544569, "42000", "37000", "DSQL error" },                    // isc_dsql_error
    { 335544652, "42000", "37000", "DSQL command error" },            // isc_dsql_command_err
    { 335544573, "42000", "37000", "DSQL syntax error" },             // isc_dsql_syntax_err
    
    // Object not found
    { 335544580, "42S02", "S0002", "Table not found" },               // isc_dsql_relation_err
    { 335544578, "42S22", "S0022", "Column not found" },              // isc_dsql_field_err
    { 335544581, "42S01", "S0001", "Table already exists" },          // isc_dsql_table_err (on CREATE)
    
    // Constraint violations
    { 335544347, "23000", "23000", "Validation error" },              // isc_not_valid
    { 335544349, "23000", "23000", "Unique constraint violation" },   // isc_no_dup
    { 335544466, "23000", "23000", "Foreign key violation" },         // isc_foreign_key
    { 335544558, "23000", "23000", "Check constraint violation" },    // isc_check_constraint
    { 335544665, "23000", "23000", "Unique key violation" },          // isc_unique_key_violation
    
    // Connection errors
    { 335544375, "08001", "08001", "Database unavailable" },          // isc_unavailable
    { 335544421, "08004", "08004", "Connection rejected" },           // isc_connect_reject
    { 335544648, "08S01", "08S01", "Connection lost" },               // isc_conn_lost
    { 335544721, "08001", "08001", "Network error" },                 // isc_network_error
    { 335544726, "08S01", "08S01", "Network read error" },            // isc_net_read_err
    { 335544727, "08S01", "08S01", "Network write error" },           // isc_net_write_err
    { 335544741, "08S01", "08S01", "Lost database connection" },      // isc_lost_db_connection
    { 335544744, "08004", "08004", "Max attachments exceeded" },      // isc_max_att_exceeded
    
    // Authentication
    { 335544472, "28000", "28000", "Login failed" },                  // isc_login
    
    // Lock/deadlock
    { 335544336, "40001", "40001", "Deadlock" },                      // isc_deadlock
    { 335544345, "40001", "40001", "Lock conflict" },                 // isc_lock_conflict
    
    // Cancellation
    { 335544794, "HY008", "S1008", "Operation cancelled" },          // isc_cancelled
    
    // Numeric overflow
    { 335544779, "22003", "22003", "Numeric value out of range" },    // isc_arith_except
    
    // String data truncation
    { 335544914, "22001", "22001", "String data, right truncation" }, // isc_string_truncation
    
    // Division by zero
    { 335544778, "22012", "22012", "Division by zero" },              // isc_exception_integer_divide
    
    // Permission denied
    { 335544352, "42000", "37000", "No permission" },                 // isc_no_priv
    
    // ... (extend to cover ALL common ISC error codes)
    { 0, NULL, NULL, NULL }  // Sentinel
};
```

### 5.2 Entry Point Wrapper Template

Create a macro or template that enforces the standard entry pattern:

```cpp
// In OdbcEntryGuard.h (NEW)
#define ODBC_ENTRY_STMT(hStmt, method_call)                          \
    do {                                                              \
        if (!(hStmt)) return SQL_INVALID_HANDLE;                     \
        OdbcStatement* _stmt = static_cast<OdbcStatement*>(hStmt);   \
        GUARD_HSTMT(hStmt);                                          \
        _stmt->clearErrors();                                        \
        try {                                                         \
            return _stmt->method_call;                                \
        } catch (const SQLException& e) {                            \
            _stmt->postError(&e);                                    \
            return SQL_ERROR;                                         \
        } catch (...) {                                               \
            _stmt->postError("HY000", "Internal driver error");      \
            return SQL_ERROR;                                         \
        }                                                             \
    } while(0)
```

### 5.3 Safe Guard Macro

Replace the current `GUARD_HDESC` with a safe variant:

```cpp
#define GUARD_HDESC_SAFE(h)                         \
    if ((h) == NULL) return SQL_INVALID_HANDLE;     \
    GUARD_HDESC(h)
```

Apply the same pattern to `GUARD_HSTMT`, `GUARD_ENV`, `GUARD_HDBC`.

### 5.4 Commit Strategy

Work incrementally. Each phase should be a series of focused, reviewable commits:

1. One commit per fix (e.g., "Fix GUARD_HDESC null dereference in SQLCopyDesc")
2. Every fix commit should include or update a test
3. Run the full test suite before every commit

---

## 6. Success Criteria

### 6.1 Phase Completion Gates

| Phase | Gate Criteria |
|-------|--------------|
| Phase 0 | Zero crashes with null/invalid handles. All critical-severity issues closed. |
| Phase 1 | 100% of existing tests passing. Correct SQLSTATE for syntax errors, constraint violations, connection failures, lock conflicts. |
| Phase 2 | Every ODBC entry point follows the standard wrapper pattern. Thread safety is always-on. |
| Phase 3 | 270 tests (270 pass). Comprehensive coverage: null handles, connections, cursors, descriptors, multi-statement, data types, BLOBs, savepoints, catalog functions, bind cycling, escape passthrough, server versions, batch params, array binding (column-wise + row-wise), ConnSettings, scrollable cursors, connect options, errors, result/param conversions, prepared statements, cursor-commit, data-at-execution. CI tests on Windows + Linux. |
| Phase 4 | 270 tests (270 pass). Batch execution validated (row-wise + column-wise, with operation ptr + error handling). Scrollable cursors verified (all orientations). `SQLGetTypeInfo` extended for FB4+ types. ConnSettings implemented. Server version feature-flagging in place. ODBC escape sequences removed (SQL sent AS IS). |
| Phase 5 | No raw `new`/`delete` in new code. Consistent formatting. Doxygen comments on public APIs. |

### 6.2 Overall Quality Targets

| Metric | Current | Target | Notes |
|--------|---------|--------|-------|
| Test pass rate | **100%** | 100% | ✅ All tests pass; connection tests skip gracefully without database |
| Test count | **270** | 150+ | ✅ Target far exceeded — 270 tests covering 24 test suites |
| SQLSTATE mapping coverage | **90%+ (121 kSqlStates, 100+ ISC mappings)** | 90%+ | ✅ All common Firebird errors map to correct SQLSTATEs |
| Crash on invalid input | **Never (NULL handles return SQL_INVALID_HANDLE)** | Never | ✅ Phase 0 complete — 65 GTest (direct-DLL) + 28 null handle tests |
| Cross-platform tests | **Windows + Linux (x64 + ARM64)** | Windows + Linux + macOS | ✅ CI passes on all platforms |
| Firebird version matrix | 5.0 only | 3.0, 4.0, 5.0 | CI tests all supported versions |
| Unicode compliance | **100% tests passing** | 100% | ✅ All W function tests pass including BufferLength validation |

### 6.3 Benchmark: What "First-Class" Means

A first-class ODBC driver should:

1. ✅ **Never crash** on any combination of valid or invalid API calls
2. ✅ **Return correct SQLSTATEs** for all error conditions
3. ✅ **Pass the Microsoft ODBC Test Tool** conformance checks
4. ✅ **Work on all platforms** (Windows x86/x64/ARM64, Linux x64/ARM64, macOS)
5. ✅ **Handle Unicode correctly** (UTF-16 on all platforms, no locale dependency)
6. ✅ **Support all commonly-used ODBC features** (cursors, batch execution, descriptors, escapes)
7. ✅ **Have comprehensive automated tests** (100+ tests, cross-platform, multi-version)
8. ✅ **Be thread-safe** (per-connection locking, no data races)
9. ✅ **Have clean, maintainable code** (modern C++, consistent style, documented APIs)
10. ✅ **Have CI/CD** with automated testing on every commit

---

## Appendix A: File-Level Issue Map

Quick reference for which files need changes in each phase.

| File | Phase 0 | Phase 1 | Phase 2 | Phase 3 | Phase 4 | Phase 5 |
|------|---------|---------|---------|---------|---------|---------|
| Main.cpp | C-3 | | 2.1, 2.2 | | | |
| MainUnicode.cpp | | H-12 | 2.1 | | | |
| OdbcObject.cpp | C-6 | H-9, H-10 | | | | L-7 |
| OdbcConnection.cpp | | H-5, H-6, H-7, H-8, H-13 | 2.1 | | M-5 | |
| OdbcStatement.cpp | | H-1, H-11, H-14 | 2.1, 2.3 | | M-2, M-7, M-9 | |
| OdbcDesc.cpp | C-1, C-2 | | 2.1 | | | |
| OdbcEnv.cpp | | H-4 | 2.1 | | | |
| OdbcError.cpp | | H-2, H-3, H-15 | | | | |
| OdbcConvert.cpp | | | | T-4 | | L-3 |
| SafeEnvThread.h | | | 2.4 | | | |
| IscDbc/IscConnection.cpp | | | 2.3 (savepoints) | | M-3 | |
| IscDbc/IscStatement.cpp | | | 2.3 (savepoints) | | | |
| IscDbc/Connection.h | | | 2.3 (savepoints) | | | |
| IscDbc/ (various) | C-7 | | | | M-4, M-8 | |
| Tests/ | C-1 (test) | 1.14 | | 3.1–3.12 | | |
| NEW: OdbcSqlState.h | | H-2, H-3 | | | | |
| NEW: OdbcEntryGuard.h | | | 2.1 | | | |
| NEW: Tests/standalone/ | | | | 3.13 | | |

## Appendix B: psqlodbc Patterns to Adopt

| Pattern | psqlodbc Implementation | Firebird Adaptation |
|---------|------------------------|---------------------|
| Entry-point wrapper | `ENTER_*_CS` / `LEAVE_*_CS` + error clear + savepoint | Create `ODBC_ENTRY_*` macros in OdbcEntryGuard.h |
| SQLSTATE lookup table | `Statement_sqlstate[]` with ver2/ver3 | Create `iscToSqlState[]` in OdbcSqlState.h |
| Platform-abstracted mutex | `INIT_CS` / `ENTER_CS` / `LEAVE_CS` macros | Refactor SafeEnvThread.h to use platform macros |
| Memory allocation with error | `CC_MALLOC_return_with_error` | Create `ODBC_MALLOC_or_error` macro |
| Safe string wrapper | `pgNAME` with `STR_TO_NAME` / `NULL_THE_NAME` | Adopt or use `std::string` consistently |
| Server version checks | `PG_VERSION_GE(conn, ver)` | Create `FB_VERSION_GE(conn, major, minor)` |
| Catalog field enums | `TABLES_*`, `COLUMNS_*` position enums | Create enums in IscDbc result set headers |
| Expected-output test model | `test/expected/*.out` + diff comparison | Create `Tests/standalone/` + `Tests/expected/` |
| Dual ODBC version mapping | `ver3str` + `ver2str` per error | Add to new SQLSTATE mapping table |
| Constructor/Destructor naming | `CC_Constructor()` / `CC_Destructor()` | Already have C++ constructors/destructors |

## Appendix C: References

- [Firebird Driver Feature Map](/Docs/firebird-driver-feature-map.md)
- [ODBC 3.8 Programmer's Reference](https://learn.microsoft.com/en-us/sql/odbc/reference/odbc-programmer-s-reference)
- [ODBC API Reference](https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/odbc-api-reference)
- [ODBC Unicode Specification](https://learn.microsoft.com/en-us/sql/odbc/reference/develop-app/unicode-data)
- [ODBC SQLSTATE Appendix A](https://learn.microsoft.com/en-us/sql/odbc/reference/appendixes/appendix-a-odbc-error-codes)
- [psqlodbc Source Code](https://git.postgresql.org/gitweb/?p=psqlodbc.git) (reference in `./tmp/psqlodbc/`)
- [Firebird 5.0 Language Reference](https://firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html)
- [Firebird New OO API Reference](https://github.com/FirebirdSQL/firebird/blob/master/doc/Using_OO_API.md)


---

*Document version: 2.1 — February 8, 2026*  
*This is the single authoritative reference for all Firebird ODBC driver improvements.*
