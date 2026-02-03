# ODBC Spec Compliance Fix Plan

Date: February 1, 2026  
Updated: February 2, 2026 (Unicode Support Issue #244)

This plan documents the mismatches found and concrete fixes with exact locations.

## 1) SQL_ATTR_ODBC_VERSION is not honored
- Finding: SQLSetEnvAttr stores the app ODBC version in `useAppOdbcVersion`, but SQLGetEnvAttr always returns SQL_OV_ODBC3.
- Location: [OdbcEnv.cpp](OdbcEnv.cpp#L150-L182)
- Fix:
  - In `OdbcEnv::sqlGetEnvAttr`, return `useAppOdbcVersion` for `SQL_ATTR_ODBC_VERSION`.
  - Keep default at SQL_OV_ODBC3 when not set, but do not hardcode on read.
  - Ensure `SQLSetEnvAttr` validates only supported values (SQL_OV_ODBC2, SQL_OV_ODBC3, SQL_OV_ODBC3_80 if supported) and returns HY092 for unsupported.

## 2) SQLSetConnectAttr silently accepts unsupported attributes
- Finding: `OdbcConnection::sqlSetConnectAttr` lacks a default error path and returns success for unknown attributes.
- Location: [OdbcConnection.cpp](OdbcConnection.cpp#L386-L520)
- Fix:
  - Add a `default:` in the switch that returns SQL_ERROR with SQLSTATE HY092 for invalid/unknown attributes.
  - For recognized-but-unsupported attributes, return SQL_ERROR with SQLSTATE HYC00.
  - Keep existing special cases (e.g., SQL_ATTR_ANSI_APP) but align error codes per spec.

## 3) SQLGetConnectAttr ignores caller StringLengthPtr
- Finding: `OdbcConnection::sqlGetConnectAttr` overwrites `lengthPtr` with a local pointer, so callers never receive length output.
- Location: [OdbcConnection.cpp](OdbcConnection.cpp#L2134-L2162)
- Fix:
  - Remove the local `len` and pass through the caller’s `lengthPtr`.
  - Only write to `lengthPtr` when it is non-null.

## 4) SQLGetInfo mishandles non-string InfoTypes
- Finding: For numeric InfoTypes, code dereferences `InfoValuePtr` without NULL checks and uses `BufferLength` to decide 16-bit vs 32-bit storage even though `BufferLength` is ignored for non-strings.
- Location: [OdbcConnection.cpp](OdbcConnection.cpp#L1486-L1538)
- Fix:
  - If `ptr` is NULL, do not write; still set `actualLength` to the required size.
  - Always return the correct size for the InfoType (SQLUSMALLINT vs SQLUINTEGER) independent of `maxLength`.
  - Ensure all non-string InfoTypes are written as the correct width.

## 5) SQL_SCHEMA_USAGE uses catalog check for index definition
- Finding: `SQL_SCHEMA_USAGE` uses `supportsCatalogsInIndexDefinitions()` instead of schema check for index definitions.
- Location: [OdbcConnection.cpp](OdbcConnection.cpp#L1236-L1262)
- Fix:
  - Replace with `supportsSchemasInIndexDefinitions()` (or equivalent metadata call).
  - If the metadata API lacks this method, derive from schema support and document behavior.

## 6) SQLGetDiagRec returns SQL_NO_DATA_FOUND (ODBC 2.x)
- Finding: SQLGetDiagRec returns SQL_NO_DATA_FOUND instead of SQL_NO_DATA for no diagnostic records.
- Location: [OdbcObject.cpp](OdbcObject.cpp#L290-L312)
- Fix:
  - Return SQL_NO_DATA (ODBC 3.x) when no records exist.
  - Keep SQLSTATE "00000" and empty message buffer behavior intact.

## 7) SQLGetDiagField reads StringLengthPtr without NULL checks
- Finding: SQL_DIAG_NUMBER path uses `stringLength` without checking for NULL.
- Location: [OdbcObject.cpp](OdbcObject.cpp#L314-L341)
- Fix:
  - Guard `stringLength` before dereference.
  - Populate `ptr` if non-null, and ignore `stringLength` if NULL.

## 8) SQLSetStmtAttr cursor-state validations missing
- Finding: Attributes like SQL_ATTR_CONCURRENCY, SQL_ATTR_CURSOR_TYPE, SQL_ATTR_USE_BOOKMARKS should error when cursor is open (24000) or statement prepared (HY011), but current implementation allows changes.
- Location: [OdbcStatement.cpp](OdbcStatement.cpp#L3260-L3415)
- Fix:
  - Track open cursor state and prepared state (existing flags can be reused).
  - On set of those attributes, return SQL_ERROR with SQLSTATE 24000 if cursor open, HY011 if prepared.
  - Keep Option Value Changed (01S02) behavior where required by the spec.

## 9) Unicode W APIs do not validate even BufferLength
- Finding: W entry points do not enforce that `BufferLength` is even for Unicode buffers; spec requires HY090 when odd.
- Locations:
  - [MainUnicode.cpp](MainUnicode.cpp#L503-L555) for SQLGetInfoW
  - [MainUnicode.cpp](MainUnicode.cpp#L930-L940) for SQLGetConnectAttrW
  - [MainUnicode.cpp](MainUnicode.cpp#L1078-L1086) for SQLGetStmtAttrW
  - [MainUnicode.cpp](MainUnicode.cpp#L1051-L1065) for SQLGetDiagRecW
- Fix:
  - Add a shared helper to validate even BufferLength for W APIs when a character buffer is expected.
  - Return SQL_ERROR with SQLSTATE HY090 when odd.
  - Keep zero or negative buffer-length rules aligned with each API.

## 10) SQLGetInfo string handling should tolerate NULL InfoValuePtr
- Finding: When `InfoValuePtr` is NULL for string InfoTypes, current code still attempts to write through setString in some paths.
- Location: [OdbcConnection.cpp](OdbcConnection.cpp#L1486-L1538)
- Fix:
  - If `ptr` is NULL, skip writing and only return the required length in `actualLength`.
  - Ensure SQL_SUCCESS is returned (or SQL_SUCCESS_WITH_INFO only when truncation would occur).

## Implementation Order (Recommended)
1. Fix SQLGetConnectAttr length pointer handling.
2. Fix SQLGetInfo numeric storage and NULL handling.
3. Add SQLSetConnectAttr default error handling.
4. Update SQL_ATTR_ODBC_VERSION reporting.
5. Correct SQL_SCHEMA_USAGE index-definition check.
6. Fix SQLGetDiagRec and SQLGetDiagField behaviors.
7. Enforce cursor-state rules in SQLSetStmtAttr.
8. Add Unicode BufferLength validation in W APIs.

## Validation Checklist
- Run existing Test suite and confirm no regressions.
- Add targeted tests for:
  - SQLGetEnvAttr returns the set ODBC version.
  - SQLSetConnectAttr with an invalid attribute returns HY092.
  - SQLGetConnectAttr returns correct StringLengthPtr.
  - SQLGetInfo with NULL InfoValuePtr returns required length without crash.
  - SQLGetDiagRec returns SQL_NO_DATA when no records exist.
  - SQLGetDiagField SQL_DIAG_NUMBER with NULL StringLengthPtr does not crash.
  - SQLSetStmtAttr rejects changes with open cursor or prepared statement.
  - SQLGetInfoW/SQLGetConnectAttrW/SQLGetStmtAttrW reject odd BufferLength with HY090.
  - **[NEW] All Unicode tests in Issue244Tests.cpp pass**
  - **[NEW] Unicode functions work on Linux with unixODBC and iODBC**
  - **[NEW] Data roundtrip correctly with Unicode characters outside current locale**
