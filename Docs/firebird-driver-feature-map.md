# Firebird Driver Feature Map (3.0, 4.0, 5.0)

This guide is for driver developers (ODBC and other client libraries) who need version-aware SQL, type, and metadata support.

Primary references:
- Firebird 3.0 Language Reference: https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html
- Firebird 4.0 Language Reference: https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html
- Firebird 5.0 Language Reference: https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html

## 1. Baseline for minimum supported version (Firebird 3.0)

If your minimum server is 3.0, these are safe baseline capabilities:

- Core SQL chapters: [SQL Language Structure](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-structure), [Data Types and Subtypes](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-datatypes), [DDL](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-ddl), [DML](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-dml), [PSQL](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-psql).
- Baseline types include [BOOLEAN](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-datatypes-boolean), classic numeric/decimal, DATE/TIME/TIMESTAMP (without timezone type family), character, BLOB, and array types.
- UUID support is function-based (not a dedicated type): [UUID Functions](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-functions-uuid), with binary UUID values represented as `CHAR(16) CHARACTER SET OCTETS` in 3.0.
- Programmatic SQL features include [EXECUTE BLOCK](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-dml-execblock), [FUNCTION](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-ddl-function), [PACKAGE](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-ddl-package), [MERGE](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-dml-merge), [UPDATE OR INSERT](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-dml-update-or-insert), and baseline [window functions](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-windowfuncs).
- Identity autoincrement exists as [Identity Columns (autoincrement)](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-ddl-tbl-identity) (3.0 semantics).
- Transaction control includes [SET TRANSACTION](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-transacs-settransac) and savepoint operations.
- Metadata surfaces for introspection already exist in [System Tables](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-appx04-systables) and [Monitoring Tables](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-appx05-montables).

Driver baseline implications:
- Assume no INT128, DECFLOAT, TIME WITH TIME ZONE, TIMESTAMP WITH TIME ZONE, BINARY/VARBINARY grammar in pure 3.0 mode.
- Keep parser and type mapping conservative, then unlock features when server major version >= 4 or >= 5.

## 2. What changed from 3.0 to 4.0 (driver-impacting)

| Area | New/changed in 4.0 | Driver impact | Reference |
| --- | --- | --- | --- |
| Numeric types | `INT128` and `DECFLOAT` | Add type IDs, precision/scale handling, and literal/parameter support | [INT128](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-datatypes-int128), [DECFLOAT](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-datatypes-decfloat) |
| Time zone types | `TIME WITH TIME ZONE`, `TIMESTAMP WITH TIME ZONE` | Add timezone-aware bind/fetch mappings and conversion rules | [TIME WITH TIME ZONE](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-datatypes-time-tz), [TIMESTAMP WITH TIME ZONE](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-datatypes-timestamp-tz) |
| Context variable behavior | `CURRENT_TIME` and `CURRENT_TIMESTAMP` become timezone-aware in 4.0 | Existing apps expecting old return types can break; use `LOCALTIME`/`LOCALTIMESTAMP` when needed | [CURRENT_TIME](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-contextvars-current-time), [CURRENT_TIMESTAMP](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-contextvars-current-timestamp) |
| Binary string types | `BINARY`, `VARBINARY` | Extend DDL/type parser and metadata mapping | [BINARY](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-datatypes-chartypes-binary), [VARBINARY](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-datatypes-chartypes-varbinary) |
| UUID binary representation naming | UUID functions now report `BINARY(16)` (instead of 3.0 `CHAR(16) CHARACTER SET OCTETS`), function set unchanged | Normalize both representations to one logical UUID-binary mapping in driver metadata | [3.0 UUID Functions](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-functions-uuid), [4.0 UUID Functions](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-functions-uuid), [4.0 BINARY](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-datatypes-chartypes-binary) |
| Type coercion controls | `SET BIND`, `SET DECFLOAT` | Useful compatibility bridge when client type support lags server features | [SET BIND](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-management-setbind), [SET DECFLOAT](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-management-setdecfloat) |
| Analytics SQL | Aggregate `FILTER`, `WINDOW` clause, named windows/frames, added rank functions (`CUME_DIST`, `NTILE`, `PERCENT_RANK`) | Parser and SQL generation need version guards | [FILTER](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-aggfuncs-filter), [WINDOW clause](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-dml-select-window), [Window Frames](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-windowfuncs-frame) |
| DML grammar | `LATERAL` joins and `INSERT ... OVERRIDING` | Update parser/tokenizer and SQL builder for identity workflows | [LATERAL](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-dml-select-joins-lateral), [OVERRIDING](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-dml-insert-overriding) |
| Identity enhancements | Extended identity semantics including `GENERATED ALWAYS` and more alter options | Metadata and generated-key behavior needs version-dependent logic | [Identity Columns (Autoincrement)](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-ddl-tbl-identity) |
| Security model | SQL SECURITY and fine-grained system privileges | Privilege introspection and definer/invoker semantics matter for metadata and DDL execution | [SQL Security](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-security-sql-security), [System Privileges](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-security-sys-privs) |
| Session controls | `SET TIME ZONE`, idle and statement timeout statements, `ALTER SESSION RESET` | Add optional connection/session APIs and SQL passthrough coverage | [SET TIME ZONE](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-management-settimezone), [SET SESSION IDLE TIMEOUT](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-management-setsessionidle), [SET STATEMENT TIMEOUT](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-management-setstatementtimeout), [ALTER SESSION RESET](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-management-session-reset-alter) |
| Transactions | `AUTO COMMIT` option for `SET TRANSACTION` | Transaction option parser and capability flags need update | [AUTO COMMIT](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-transacs-settransac-autocommit) |
| Metadata additions | `RDB$CONFIG`, `RDB$TIME_ZONES`, `RDB$PUBLICATIONS`, `RDB$PUBLICATION_TABLES` | Expand metadata schema maps and avoid hardcoded table lists | [RDB$CONFIG](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref-appx04-config), [RDB$TIME_ZONES](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref-appx04-timezones), [RDB$PUBLICATIONS](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-appx04-publications) |
| Built-in crypto | New cryptographic function set | Driver SQL pass-through tests should include new function namespace | [Cryptographic Functions](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-scalarfuncs-crypto) |

## 3. What changed from 4.0 to 5.0 (driver-impacting)

| Area | New/changed in 5.0 | Driver impact | Reference |
| --- | --- | --- | --- |
| Lock-aware DML | `SKIP LOCKED` for `UPDATE` and `DELETE` | Add grammar support and concurrency test coverage | [UPDATE SKIP LOCKED](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-dml-update-skiplocked), [DELETE SKIP LOCKED](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-dml-delete-skiplocked) |
| Index features | Partial indexes and parallelized index creation | DDL generators and metadata readers must account for new index properties | [Partial Indexes](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-ddl-idx-partial), [Parallelized Index Creation](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-ddl-idx-parallel) |
| Optimizer controls in SQL | `SELECT ... OPTIMIZE FOR` | SQL generators need version check before emitting clause | [OPTIMIZE FOR](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-dml-selec-optimize) |
| DML ordering clauses | `UPDATE OR INSERT` gains `ORDER BY`/`ROWS`; `MERGE` gains `ORDER BY` | New syntax branches in parser and query builders | [UPDATE OR INSERT ORDER BY/ROWS](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-dml-update-or-insert-orderrows), [MERGE ORDER BY](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-dml-merge-order) |
| System packages chapter | New formal chapter for system packages, including `RDB$BLOB_UTIL`, `RDB$PROFILER`, and `RDB$TIME_ZONE_UTIL` | Expose package routine metadata in tooling and SQL completion | [System Packages](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-sys-pckg), [RDB$BLOB_UTIL](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-sys-pckg-blobutil), [RDB$PROFILER](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-sys-pckg-profiler) |
| New scalar functions | `UNICODE_CHAR`, `UNICODE_VAL` | Add function capability flags for SQL generation/validation | [UNICODE_CHAR](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-scalarfuncs-unicode-char), [UNICODE_VAL](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-scalarfuncs-unicode-val) |
| UUID handling | No functional change vs 4.0; still function-based (`CHAR_TO_UUID`, `GEN_UUID`, `UUID_TO_CHAR`) with binary representation as `BINARY(16)` | Keep same UUID code path as 4.0; main compatibility split remains 3.0 vs 4+ type naming | [5.0 UUID Functions](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-functions-uuid) |
| PSQL cursor declaration | Explicit forward-only and scrollable cursor declaration types | PSQL parser support update if your driver parses/rewrites PSQL text | [Forward-Only and Scrollable Cursors](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-psql-declare-cursor-types) |
| Comparison conversion rules | New section for implicit conversion during comparisons | Potential behavior differences in predicate evaluation; add regression tests | [Implicit Conversion During Comparison Operations](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-datatypes-convert-implicit-compare) |
| Metadata additions | `RDB$KEYWORDS`, `MON$COMPILED_STATEMENTS`, plugin tables appendix | Update metadata introspection maps and monitoring adapters | [RDB$KEYWORDS](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref-appx04-keywords), [MON$COMPILED_STATEMENTS](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref-appx05-moncompst), [Plugin tables](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-appx07-plgtables) |

## 4. Quick compatibility map

`Yes` means documented as available in that version.

| Capability | 3.0 | 4.0 | 5.0 | Reference |
| --- | --- | --- | --- | --- |
| `BOOLEAN` type | Yes | Yes | Yes | [3.0 BOOLEAN](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-datatypes-boolean) |
| Stored SQL functions (`CREATE FUNCTION`) | Yes | Yes | Yes | [3.0 FUNCTION](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-ddl-function) |
| Packages (`PACKAGE`/`PACKAGE BODY`) | Yes | Yes | Yes | [3.0 PACKAGE](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-ddl-package) |
| `EXECUTE BLOCK` | Yes | Yes | Yes | [3.0 EXECUTE BLOCK](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-dml-execblock) |
| `MERGE` | Yes | Yes | Yes | [3.0 MERGE](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-dml-merge) |
| `UPDATE OR INSERT` | Yes | Yes | Yes | [3.0 UPDATE OR INSERT](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-dml-update-or-insert) |
| Identity columns (base support) | Yes | Yes | Yes | [3.0 Identity Columns](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-ddl-tbl-identity) |
| Identity `GENERATED ALWAYS` and expanded identity options | No | Yes | Yes | [4.0 Identity Columns](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-ddl-tbl-identity) |
| `INT128` | No | Yes | Yes | [4.0 INT128](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-datatypes-int128) |
| `DECFLOAT` | No | Yes | Yes | [4.0 DECFLOAT](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-datatypes-decfloat) |
| `TIME WITH TIME ZONE` / `TIMESTAMP WITH TIME ZONE` | No | Yes | Yes | [4.0 TIME WITH TIME ZONE](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-datatypes-time-tz) |
| `CURRENT_TIME`/`CURRENT_TIMESTAMP` timezone-aware return type | No | Yes | Yes | [4.0 CURRENT_TIME](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-contextvars-current-time) |
| UUID functions (`CHAR_TO_UUID`, `GEN_UUID`, `UUID_TO_CHAR`) | Yes | Yes | Yes | [3.0 UUID Functions](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-functions-uuid) |
| UUID binary result type shown as `CHAR(16) CHARACTER SET OCTETS` | Yes | No | No | [3.0 GEN_UUID](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-scalarfuncs-gen-uuid) |
| UUID binary result type shown as `BINARY(16)` | No | Yes | Yes | [4.0 GEN_UUID](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-scalarfuncs-gen-uuid), [5.0 GEN_UUID](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-scalarfuncs-gen-uuid) |
| UUID string result type `CHAR(36)` (`UUID_TO_CHAR`) | Yes | Yes | Yes | [3.0 UUID_TO_CHAR](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-scalarfuncs-uuid-to-char) |
| `BINARY` / `VARBINARY` | No | Yes | Yes | [4.0 BINARY](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-datatypes-chartypes-binary) |
| Aggregate `FILTER` clause | No | Yes | Yes | [4.0 FILTER](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-aggfuncs-filter) |
| `SELECT ... WINDOW` clause | No | Yes | Yes | [4.0 WINDOW Clause](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-dml-select-window) |
| Advanced window features (frames, named windows, `CUME_DIST`/`NTILE`/`PERCENT_RANK`) | No | Yes | Yes | [4.0 Window Frames](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-windowfuncs-frame) |
| `LATERAL` derived table joins | No | Yes | Yes | [4.0 LATERAL](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-dml-select-joins-lateral) |
| `INSERT ... OVERRIDING` | No | Yes | Yes | [4.0 OVERRIDING](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-dml-insert-overriding) |
| SQL SECURITY | No | Yes | Yes | [4.0 SQL Security](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-security-sql-security) |
| Fine-grained system privileges | No | Yes | Yes | [4.0 System Privileges](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-security-sys-privs) |
| `SET BIND` | No | Yes | Yes | [4.0 SET BIND](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-management-setbind) |
| `SET SESSION IDLE TIMEOUT` / `SET STATEMENT TIMEOUT` | No | Yes | Yes | [4.0 Session Timeouts](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-management-setsessionidle) |
| `SET TIME ZONE` | No | Yes | Yes | [4.0 SET TIME ZONE](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-management-settimezone) |
| `ALTER SESSION RESET` | No | Yes | Yes | [4.0 ALTER SESSION RESET](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-management-session-reset-alter) |
| `SET TRANSACTION ... AUTO COMMIT` option | No | Yes | Yes | [4.0 AUTO COMMIT](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-transacs-settransac-autocommit) |
| `RDB$CONFIG` / `RDB$TIME_ZONES` metadata tables | No | Yes | Yes | [4.0 RDB$CONFIG](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref-appx04-config) |
| `RDB$PUBLICATIONS` / `RDB$PUBLICATION_TABLES` | No | Yes | Yes | [4.0 RDB$PUBLICATIONS](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-appx04-publications) |
| Cryptographic built-ins (`ENCRYPT`, etc.) | No | Yes | Yes | [4.0 Cryptographic Functions](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-scalarfuncs-crypto) |
| `UPDATE`/`DELETE` `SKIP LOCKED` | No | No | Yes | [5.0 UPDATE SKIP LOCKED](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-dml-update-skiplocked) |
| Partial indexes | No | No | Yes | [5.0 Partial Indexes](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-ddl-idx-partial) |
| Parallelized index creation | No | No | Yes | [5.0 Parallelized Index Creation](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-ddl-idx-parallel) |
| `SELECT ... OPTIMIZE FOR` | No | No | Yes | [5.0 OPTIMIZE FOR](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-dml-selec-optimize) |
| `UPDATE OR INSERT` with `ORDER BY`/`ROWS` | No | No | Yes | [5.0 UPDATE OR INSERT ORDER BY/ROWS](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-dml-update-or-insert-orderrows) |
| `MERGE` with `ORDER BY` | No | No | Yes | [5.0 MERGE ORDER BY](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-dml-merge-order) |
| System packages chapter (`RDB$BLOB_UTIL`, `RDB$PROFILER`, etc.) | No | No | Yes | [5.0 System Packages](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-sys-pckg) |
| `UNICODE_CHAR` / `UNICODE_VAL` | No | No | Yes | [5.0 UNICODE_CHAR](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-scalarfuncs-unicode-char) |
| PSQL explicit forward-only/scrollable cursor declaration types | No | No | Yes | [5.0 Cursor Types](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-psql-declare-cursor-types) |
| New documented comparison conversion rules | No | No | Yes | [5.0 Implicit Conversion During Comparison Operations](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-datatypes-convert-implicit-compare) |
| `RDB$KEYWORDS` | No | No | Yes | [5.0 RDB$KEYWORDS](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref-appx04-keywords) |
| `MON$COMPILED_STATEMENTS` | No | No | Yes | [5.0 MON$COMPILED_STATEMENTS](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref-appx05-moncompst) |
| Plugin tables appendix | No | No | Yes | [5.0 Plugin tables](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-appx07-plgtables) |

## 5. UUID handling for drivers

Firebird does not define a standalone SQL type named `UUID`. UUID support is provided through functions and binary string types.

### 5.1 UUID support by version

| Topic | 3.0 | 4.0 | 5.0 | Reference |
| --- | --- | --- | --- | --- |
| UUID function family (`CHAR_TO_UUID`, `GEN_UUID`, `UUID_TO_CHAR`) | Yes | Yes | Yes | [3.0 UUID Functions](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-functions-uuid), [4.0 UUID Functions](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-functions-uuid), [5.0 UUID Functions](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-functions-uuid) |
| Binary UUID result type for `GEN_UUID`/`CHAR_TO_UUID` | `CHAR(16) CHARACTER SET OCTETS` | `BINARY(16)` | `BINARY(16)` | [3.0 GEN_UUID](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-scalarfuncs-gen-uuid), [4.0 GEN_UUID](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-scalarfuncs-gen-uuid), [5.0 GEN_UUID](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-scalarfuncs-gen-uuid) |
| Text UUID result type for `UUID_TO_CHAR` | `CHAR(36)` | `CHAR(36)` | `CHAR(36)` | [3.0 UUID_TO_CHAR](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-scalarfuncs-uuid-to-char), [5.0 UUID_TO_CHAR](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-scalarfuncs-uuid-to-char) |
| Binary type naming in SQL/metadata | `CHAR/VARCHAR ... CHARACTER SET OCTETS` | `BINARY/VARBINARY` aliases are available | `BINARY/VARBINARY` aliases are available | [3.0 Special Character Sets](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref30/firebird-30-language-reference.html#fblangref30-datatypes-chartypes-special), [4.0 BINARY](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref40/firebird-40-language-reference.html#fblangref40-datatypes-chartypes-binary), [5.0 BINARY](https://www.firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-datatypes-chartypes-binary) |

### 5.2 Driver implementation guidance

- Treat UUID as a logical type over a physical 16-byte binary field.
- In 3.0 metadata, normalize `CHAR(16) CHARACTER SET OCTETS` to the same internal UUID-binary mapping you use for `BINARY(16)` on 4.0+.
- Do not apply character-set transcoding to UUID binary values (`OCTETS`/`BINARY` are byte containers).
- For text APIs, map UUID strings to `CHAR(36)` and convert using server functions:
  - Input: `CHAR_TO_UUID(?)`
  - Output: `UUID_TO_CHAR(column)`
- For ODBC-style typing, a pragmatic mapping is:
  - Binary UUID columns: `SQL_BINARY` length `16`
  - Text UUID projections: `SQL_CHAR` (or wide-char equivalent) length `36`

### 5.3 SQL patterns (portable from 3.0+)

```sql
-- 3.0-style declaration
id CHAR(16) CHARACTER SET OCTETS DEFAULT GEN_UUID() NOT NULL

-- 4.0/5.0 equivalent declaration
id BINARY(16) DEFAULT GEN_UUID() NOT NULL

-- Insert text UUID through conversion
INSERT INTO t(id, ...) VALUES (CHAR_TO_UUID(?), ...);

-- Fetch as canonical text UUID
SELECT UUID_TO_CHAR(id) AS id_text, ... FROM t;
```

`CHAR_TO_UUID` expects the standard 36-character UUID form (`8-4-4-4-12` with hyphens), so drivers should validate input length/format before binding when possible.

## 6. Recommended driver capability flags

At connect time, derive a capability profile from server major version, then gate SQL generation and type mapping with flags similar to:

- `has_int128`, `has_decfloat`, `has_time_zone_types` (4+)
- `has_set_bind`, `has_statement_timeout`, `has_sql_security` (4+)
- `has_binary_alias_types` (`BINARY`/`VARBINARY`) (4+)
- `has_skip_locked_update_delete`, `has_partial_indexes`, `has_system_packages`, `has_optimize_for` (5+)

This keeps one codebase compatible with 3.0 minimum while safely enabling newer syntax and types on 4.0/5.0.
