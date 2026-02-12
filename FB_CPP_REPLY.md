> Hi Adriano!
> I’m working on a [full revamp of Firebird ODBC Driver](https://github.com/rebirdSQL/firebird-odbc-driver/pull/275) and this library caught my eye. ❤️
> **May I ask you what's its current status?** Are you planning to keep olving it, or was it mainly a prototype?

It's to keep evolving, it just need good use to streess it.

> I’d **really** like to use it in the ODBC driver; it could help us remove a lot** of legacy code.

Good!

> With a little help from Claude 😄 we jotted down a few topics we’d need to ver to move forward -- would you mind taking a look?
> _P.S. Of course, I'd be happy to contribute with PRs, as well._
> ## 📋 Suggestions for Improvement
> ### 1. Missing `IResultSet` Wrapper / Multi-Row Fetch
> **Issue**: The `Statement` class has `execute()` and `fetchNext()`, but ere's no separate `ResultSet` abstraction. This makes it awkward to:

This project had private non-published versions before and I always used a parate ResultSet class, but here decided to not. It's not possible to have ltiple active result sets opened from a single statement in Firebird.

> * Differentiate between a statement that returns results vs. one that esn't
> * Pass a result set to another function without passing the statement
>   *>
> * Support multiple simultaneous result sets from the same statement (e.g., ored procedures returning multiple cursors)

Does not exist in Firebird.

> **Suggestion**: Consider adding a `ResultSet` class that wraps ResultSet` and is returned from `Statement::execute()` when the statement oduces a result set. This matches the JDBC pattern (`Statement.executeQuery()returns `ResultSet`).
> **Alternative**: If keeping the single-class design, document that tatement::execute()` returns `true` if there are results to fetch, and etchNext()` should only be called after `execute()` returns `true`.

Don't it already has that?

```
		///
		/// @brief Executes a prepared statement using the supplied ansaction.
		/// @param transaction Transaction that will own the execution context.
		/// @return `true` when execution yields a record.
		///
		bool execute(Transaction& transaction);
```

> ### 2. Batch Execution (`IBatch`) Not Exposed
> **Issue**: The Firebird 4.0+ `IBatch` interface enables high-performance lk inserts with a single server roundtrip. This is critical for ETL rkloads and ODBC array parameter binding. The current API requires calling xecute()` N times for N rows.
> **Suggestion**: Add a `Batch` class or extend `Statement` with:
> void Statement::addBatch();  // Queue current parameter values
> BatchResult Statement::executeBatch();  // Execute all queued rows in one undtrip
> The `BatchResult` could expose per-row status IBatchCompletionState::getState()`).
> **Priority**: High — this is the single biggest performance differentiator tween naive row-by-row and professional-grade bulk operations.

Batch should be added.

> ### 3. No Scrollable Cursor Support
> **Issue**: `Statement` only exposes `fetchNext()`, `fetchPrior()`, etchFirst()`, `fetchLast()`, `fetchAbsolute()`, `fetchRelative()`. However, ese require opening the result set with `IStatement::openCursor()` ecifying `CURSOR_TYPE_SCROLLABLE`. The current implementation uses URSOR_TYPE_SCROLLABLE` (line 167 of Statement.cpp), but this is always abled even for forward-only queries.
> **Suggestion**: Add a `CursorType` enum to `StatementOptions`:
> enum class CursorType { FORWARD_ONLY, SCROLLABLE };
> 
> * Default to `FORWARD_ONLY` for performance (no server-side cursor ffering)
> * When `SCROLLABLE`, call `openCursor()` with `CURSOR_TYPE_SCROLLABLE`
> 
> **Note**: We observed that `execute()` always passes Statement::CURSOR_TYPE_SCROLLABLE`. For forward-only streaming of large sult sets, this forces the server to buffer all rows. Consider defaulting to ` (forward-only) and only using `CURSOR_TYPE_SCROLLABLE` when scrollable thods are needed.

Ok.

> ### 5. Missing `Descriptor` Metadata Access
> **Issue**: The `Descriptor` struct in `Descriptor.h` contains rich tadata (type, scale, length, nullability, field/relation names). However:
> 
> * `Statement::getInputDescriptors()` and `getOutputDescriptors()` return onst std::vector<Descriptor>&`
> * The `Descriptor` struct is only defined for read access — applications n't modify it for deferred binding
> 
> **Suggestion**: For ODBC drivers and similar adapters, we need:
> 
> 1. Access to `IMessageMetadata::getField()`, `getRelation()`, `getAlias()` c. — currently these are processed internally but not fully exposed

Could be added.

> 3. Ability to override input metadata (e.g., bind a string value as RCHAR when the server expects INTEGER) — this requires `IMetadataBuilder`>
>    Consider exposing:
> 
> 
> * `Statement::getInputMetadata()` → `FbRef<IMessageMetadata>` (already ists!)
> * `Statement::getOutputMetadata()` → `FbRef<IMessageMetadata>` (already ists!)
> * A helper to build custom `IMessageMetadata` for input override scenarios

Should be discussed.

> ### 6. `Statement` Copy/Move Semantics
> **Issue**: `Statement` is move-only (copy deleted, move-only). After a ve, `isValid()` returns `false`. This is correct, but:
> 
> * `Statement::operator=(Statement&&)` is deleted — why? This prevents td::swap` and vector/map usage
> * The moved-from statement's `statementHandle` is moved but `attachment` ference becomes dangling
> 
> **Suggestion**: Either:
> 
> 1. Make `Statement` fully movable (implement `operator=(Statement&&)`)
> 2. Or document explicitly that `Statement` should be heap-allocated and naged via `std::unique_ptr`

It should be movable.

> ### 7. Thread Safety Documentation
> **Issue**: The library doesn't document thread safety guarantees. estions:

Should be added.

> * Is it safe to use one `Client` from multiple threads?

Yes, unless one thread is not deleting it.

> * Is it safe to use one `Attachment` from multiple threads with different ransaction`s?

Should be.

> * Is `StatusWrapper` thread-safe (it contains a `dirty` flag)?

No.

> **Suggestion**: Add a "Thread Safety" section to the documentation. Based  our reading of the code:
> 
> * `Client` appears thread-safe (only reads from `master`)

> * `Attachment` is NOT thread-safe (shared `Transaction` state)

What you mean?

> * `Statement` is NOT thread-safe (mutable `inMessage`/`outMessage`)

It's not.

> ### 8. Error Message Localization
> **Issue**: `DatabaseException::what()` returns the Firebird error message, ich is localized based on the server's `LC_MESSAGES` setting. For ODBC ivers, we need access to:
> 
> * The raw ISC error vector (`intptr_t*`) for SQLSTATE mapping

Should be copied from original status and exposed.

> * The SQLCODE (via `IUtil::formatStatus()` or equivalent)
> 
> **Suggestion**: Expose the raw error data in `DatabaseException`:
> const std::vector<intptr_t>& getErrorVector() const;
> int getSqlCode() const;  // Computed via IStatus::getErrors()
> ### 9. Missing `IUtil` Wrapper
> **Issue**: Firebird's `IUtil` provides valuable utilities:
> 
> * `formatStatus()` — format error messages
> * `decodeDate()`/`encodeDate()` — manual date conversion
> * `getClientVersion()` — client library version
> * `getInt128()`/`getDecFloat16()`/`getDecFloat34()` — type converters
> 
> Some of these are used internally (`NumericConverter`, alendarConverter`) but not exposed.
> **Suggestion**: Consider exposing `Client::getUtil()` → `fb::IUtil*` or apping the most useful methods.

It's already exposed, isn't it?
