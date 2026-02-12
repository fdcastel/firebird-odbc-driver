# Suggestions for fb-cpp Library Authors

**Date**: February 11, 2026  
**Context**: Reviewed fb-cpp v0.0.2 while evaluating adoption for the Firebird ODBC Driver project  
**Repository**: https://github.com/asfernandes/fb-cpp

---

## Executive Summary

`fb-cpp` is an excellent modern C++ wrapper around the Firebird OO API. The design is clean, the API is intuitive, and the use of RAII, `std::optional`, and strong typing makes it a pleasure to work with. 

The following suggestions are offered to help mature the library for production use in high-performance, cross-platform scenarios like ODBC drivers.

---

## 🌟 What We Love

1. **Modern C++20 Design** — Fluent builders (`AttachmentOptions`, `TransactionOptions`, `StatementOptions`), `std::optional` for nullable values, `std::chrono` for date/time types, `std::span` for buffers.

2. **RAII Throughout** — `Attachment`, `Transaction`, `Statement`, `Blob`, `EventListener` all have proper destructors with exception swallowing. No resource leaks possible.

3. **Type-Safe Parameter Binding** — `setInt32()`, `setString()`, `setDate()`, etc. with `std::optional` for NULL handling is far superior to raw SQLDA manipulation.

4. **Boost.Multiprecision Integration** — Optional support for `INT128` and `DECFLOAT` types via `BoostInt128`, `BoostDecFloat16`, `BoostDecFloat34`.

5. **Boost.DLL Integration** — Runtime loading of `fbclient.dll`/`libfbclient.so` without hardcoded paths.

6. **vcpkg Integration** — First-class `vcpkg.json` manifest and custom registry for Firebird makes dependency management trivial.

7. **Comprehensive Documentation** — Doxygen comments on every public method, clear examples in README.

---

## 📋 Suggestions for Improvement

### 1. Missing `IResultSet` Wrapper / Multi-Row Fetch

**Issue**: The `Statement` class has `execute()` and `fetchNext()`, but there's no separate `ResultSet` abstraction. This makes it awkward to:
- Differentiate between a statement that returns results vs. one that doesn't
- Pass a result set to another function without passing the statement
- Support multiple simultaneous result sets from the same statement (e.g., stored procedures returning multiple cursors)

**Suggestion**: Consider adding a `ResultSet` class that wraps `IResultSet` and is returned from `Statement::execute()` when the statement produces a result set. This matches the JDBC pattern (`Statement.executeQuery()` returns `ResultSet`).

**Alternative**: If keeping the single-class design, document that `Statement::execute()` returns `true` if there are results to fetch, and `fetchNext()` should only be called after `execute()` returns `true`.

---

### 2. Batch Execution (`IBatch`) Not Exposed

**Issue**: The Firebird 4.0+ `IBatch` interface enables high-performance bulk inserts with a single server roundtrip. This is critical for ETL workloads and ODBC array parameter binding. The current API requires calling `execute()` N times for N rows.

**Suggestion**: Add a `Batch` class or extend `Statement` with:
```cpp
void Statement::addBatch();  // Queue current parameter values
BatchResult Statement::executeBatch();  // Execute all queued rows in one roundtrip
```

The `BatchResult` could expose per-row status (`IBatchCompletionState::getState()`).

**Priority**: High — this is the single biggest performance differentiator between naive row-by-row and professional-grade bulk operations.

---

### 3. No Scrollable Cursor Support

**Issue**: `Statement` only exposes `fetchNext()`, `fetchPrior()`, `fetchFirst()`, `fetchLast()`, `fetchAbsolute()`, `fetchRelative()`. However, these require opening the result set with `IStatement::openCursor()` specifying `CURSOR_TYPE_SCROLLABLE`. The current implementation uses `CURSOR_TYPE_SCROLLABLE` (line 167 of Statement.cpp), but this is always enabled even for forward-only queries.

**Suggestion**: Add a `CursorType` enum to `StatementOptions`:
```cpp
enum class CursorType { FORWARD_ONLY, SCROLLABLE };
```
- Default to `FORWARD_ONLY` for performance (no server-side cursor buffering)
- When `SCROLLABLE`, call `openCursor()` with `CURSOR_TYPE_SCROLLABLE`

**Note**: We observed that `execute()` always passes `IStatement::CURSOR_TYPE_SCROLLABLE`. For forward-only streaming of large result sets, this forces the server to buffer all rows. Consider defaulting to `0` (forward-only) and only using `CURSOR_TYPE_SCROLLABLE` when scrollable methods are needed.

---

### 5. Missing `Descriptor` Metadata Access

**Issue**: The `Descriptor` struct in `Descriptor.h` contains rich metadata (type, scale, length, nullability, field/relation names). However:
- `Statement::getInputDescriptors()` and `getOutputDescriptors()` return `const std::vector<Descriptor>&`
- The `Descriptor` struct is only defined for read access — applications can't modify it for deferred binding

**Suggestion**: For ODBC drivers and similar adapters, we need:
1. Access to `IMessageMetadata::getField()`, `getRelation()`, `getAlias()` etc. — currently these are processed internally but not fully exposed
2. Ability to override input metadata (e.g., bind a string value as VARCHAR when the server expects INTEGER) — this requires `IMetadataBuilder`

Consider exposing:
- `Statement::getInputMetadata()` → `FbRef<IMessageMetadata>` (already exists!)
- `Statement::getOutputMetadata()` → `FbRef<IMessageMetadata>` (already exists!)
- A helper to build custom `IMessageMetadata` for input override scenarios

---

---

### 6. `Statement` Copy/Move Semantics

**Issue**: `Statement` is move-only (copy deleted, move-only). After a move, `isValid()` returns `false`. This is correct, but:
- `Statement::operator=(Statement&&)` is deleted — why? This prevents `std::swap` and vector/map usage
- The moved-from statement's `statementHandle` is moved but `attachment` reference becomes dangling

**Suggestion**: Either:
1. Make `Statement` fully movable (implement `operator=(Statement&&)`)
2. Or document explicitly that `Statement` should be heap-allocated and managed via `std::unique_ptr`

---

### 7. Thread Safety Documentation

**Issue**: The library doesn't document thread safety guarantees. Questions:
- Is it safe to use one `Client` from multiple threads?
- Is it safe to use one `Attachment` from multiple threads with different `Transaction`s?
- Is `StatusWrapper` thread-safe (it contains a `dirty` flag)?

**Suggestion**: Add a "Thread Safety" section to the documentation. Based on our reading of the code:
- `Client` appears thread-safe (only reads from `master`)
- `Attachment` is NOT thread-safe (shared `Transaction` state)
- `Statement` is NOT thread-safe (mutable `inMessage`/`outMessage`)

---

### 8. Error Message Localization

**Issue**: `DatabaseException::what()` returns the Firebird error message, which is localized based on the server's `LC_MESSAGES` setting. For ODBC drivers, we need access to:
- The raw ISC error vector (`intptr_t*`) for SQLSTATE mapping
- The SQLCODE (via `IUtil::formatStatus()` or equivalent)

**Suggestion**: Expose the raw error data in `DatabaseException`:
```cpp
const std::vector<intptr_t>& getErrorVector() const;
int getSqlCode() const;  // Computed via IStatus::getErrors()
```

---

### 9. Missing `IUtil` Wrapper

**Issue**: Firebird's `IUtil` provides valuable utilities:
- `formatStatus()` — format error messages
- `decodeDate()`/`encodeDate()` — manual date conversion
- `getClientVersion()` — client library version
- `getInt128()`/`getDecFloat16()`/`getDecFloat34()` — type converters

Some of these are used internally (`NumericConverter`, `CalendarConverter`) but not exposed.

**Suggestion**: Consider exposing `Client::getUtil()` → `fb::IUtil*` or wrapping the most useful methods.

---

### 10. Example: ODBC-Style Usage Pattern

For users building ODBC drivers or similar adapters, an example showing:
1. Deferred parameter binding (set type at bind time, value at execute time)
2. Chunked BLOB read/write (`SQLGetData` / `SQLPutData` style)
3. Result set metadata introspection
4. Error vector extraction for SQLSTATE mapping

...would be extremely valuable.

---

## 🎯 Summary: Priority Ranking

| Priority | Item | Impact |
|----------|------|--------|
| **High** | #2 — IBatch support | Critical for bulk performance |
| **High** | #3 — Scrollable cursor control | Performance + correctness |
| **Medium** | #1 — ResultSet abstraction | API clarity |
| **Medium** | #7 — Thread safety docs | Developer experience |
| **Medium** | #8 — Error vector exposure | ODBC/JDBC adapter support |
| **Low** | #5 — Descriptor access | Already partially exposed |
| **Low** | #6 — Move assignment | Container compatibility |
| **Low** | #9 — IUtil wrapper | Convenience |

---

## 🙏 Thank You!

This library is a significant step forward for the Firebird ecosystem. The modern C++ design makes it a pleasure to use, and the vcpkg integration solves the long-standing "how do I get Firebird headers?" problem. We look forward to adopting fb-cpp as the core database layer in the Firebird ODBC Driver.

**— Firebird ODBC Driver Team, February 2026**
