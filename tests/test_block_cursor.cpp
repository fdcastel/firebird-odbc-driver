// tests/test_block_cursor.cpp — Block cursor (SQL_ATTR_ROW_ARRAY_SIZE > 1) tests
//
// Covers the IRD header fields an application reads back after a rowset
// fetch: SQL_ATTR_ROWS_FETCHED_PTR and SQL_ATTR_ROW_STATUS_PTR. Statement
// attributes persist until the statement is freed or the attribute is set
// again; in particular SQLPrepare / SQLExecDirect must not discard them.

#include "test_helpers.h"
#include <cstring>
#include <functional>

class BlockCursorTest : public OdbcConnectedTest {
protected:
    static constexpr SQLULEN kArraySize = 4;

    // Six rows, no table and no parameters needed.
    static const char* SixRowsSql() {
        return "SELECT 1 FROM rdb$database UNION ALL SELECT 2 FROM rdb$database"
               " UNION ALL SELECT 3 FROM rdb$database UNION ALL SELECT 4 FROM rdb$database"
               " UNION ALL SELECT 5 FROM rdb$database UNION ALL SELECT 6 FROM rdb$database";
    }

    // Six rows with a second, character column.
    static const char* SixRowsWithNamesSql() {
        return "SELECT 1, CAST('r1' AS VARCHAR(4)) FROM rdb$database"
               " UNION ALL SELECT 2, CAST('r2' AS VARCHAR(4)) FROM rdb$database"
               " UNION ALL SELECT 3, CAST('r3' AS VARCHAR(4)) FROM rdb$database"
               " UNION ALL SELECT 4, CAST('r4' AS VARCHAR(4)) FROM rdb$database"
               " UNION ALL SELECT 5, CAST('r5' AS VARCHAR(4)) FROM rdb$database"
               " UNION ALL SELECT 6, CAST('r6' AS VARCHAR(4)) FROM rdb$database";
    }

    SQLULEN rowsFetched_ = 777;
    SQLUSMALLINT rowStatus_[kArraySize] = {9, 9, 9, 9};
    SQLINTEGER values_[kArraySize] = {-1, -1, -1, -1};
    SQLLEN indicators_[kArraySize] = {0, 0, 0, 0};

    void SetRowsetAttrs() {
        ASSERT_TRUE(SQL_SUCCEEDED(SQLSetStmtAttr(hStmt, SQL_ATTR_ROW_ARRAY_SIZE,
                                                 (SQLPOINTER)kArraySize, 0)));
        ASSERT_TRUE(SQL_SUCCEEDED(SQLSetStmtAttr(hStmt, SQL_ATTR_ROWS_FETCHED_PTR,
                                                 &rowsFetched_, 0)));
        ASSERT_TRUE(SQL_SUCCEEDED(SQLSetStmtAttr(hStmt, SQL_ATTR_ROW_STATUS_PTR,
                                                 rowStatus_, 0)));
    }

    void BindColumn() {
        ASSERT_TRUE(SQL_SUCCEEDED(SQLBindCol(hStmt, 1, SQL_C_SLONG, values_, 0, indicators_)));
    }

    // Poison every output so an unwritten value is visible.
    void ResetOutputs() {
        rowsFetched_ = 777;
        for (SQLULEN i = 0; i < kArraySize; i++) {
            rowStatus_[i] = 9;
            values_[i] = -1;
        }
    }

    // One SQLFetchScroll, expected to return `expectRows` rows starting at
    // `firstValue`, with the counter and the status array written.
    void FetchScrollExpect(SQLSMALLINT orientation, SQLLEN offset,
                           SQLULEN expectRows, SQLINTEGER firstValue) {
        ResetOutputs();
        SQLRETURN ret = SQLFetchScroll(hStmt, orientation, offset);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
        EXPECT_EQ(rowsFetched_, expectRows);
        for (SQLULEN i = 0; i < kArraySize; i++) {
            if (i < expectRows) {
                EXPECT_EQ(rowStatus_[i], SQL_ROW_SUCCESS) << "row status " << i;
                EXPECT_EQ(values_[i], firstValue + (SQLINTEGER)i) << "value " << i;
            } else {
                EXPECT_EQ(rowStatus_[i], SQL_ROW_NOROW) << "row status " << i;
            }
        }
    }

    // Opens a catalog result set through `open` on a fresh statement and
    // counts its rows with the default rowset of one, then opens it again
    // on hStmt with the rowset attributes set beforehand and checks that
    // every fetch reports a consistent rowset and that the rowsets add up.
    void ExpectCatalogRowsets(const std::function<SQLRETURN(SQLHSTMT)>& open) {
        SQLHSTMT ref = AllocExtraStmt();
        SQLRETURN ret = open(ref);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, ref);
        SQLULEN total = 0;
        while (SQL_SUCCEEDED(SQLFetch(ref))) total++;
        SQLFreeHandle(SQL_HANDLE_STMT, ref);
        ASSERT_GT(total, kArraySize) << "need more rows than one rowset";

        SetRowsetAttrs();
        ret = open(hStmt);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

        SQLULEN sum = 0;
        bool sawPartial = false;
        for (;;) {
            ResetOutputs();
            ret = SQLFetch(hStmt);
            if (ret == SQL_NO_DATA) break;
            ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
            ASSERT_FALSE(sawPartial) << "a partial rowset must be the last one";
            ASSERT_GE(rowsFetched_, 1u);
            ASSERT_LE(rowsFetched_, kArraySize);
            for (SQLULEN i = 0; i < kArraySize; i++)
                EXPECT_EQ(rowStatus_[i], i < rowsFetched_ ? SQL_ROW_SUCCESS : SQL_ROW_NOROW)
                    << "row status " << i << " after " << sum << " rows";
            sawPartial = rowsFetched_ < kArraySize;
            sum += rowsFetched_;
        }
        EXPECT_EQ(sum, total);
        EXPECT_EQ(rowsFetched_, 0u) << "SQL_NO_DATA must report zero rows fetched";
    }

    // Expects the six rows to arrive as a full rowset of 4 followed by a
    // partial rowset of 2, with the counter and status array written both times.
    void FetchAndVerifySixRows() {
        ResetOutputs();
        SQLRETURN ret = SQLFetch(hStmt);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
        EXPECT_EQ(rowsFetched_, 4u) << "first rowset: SQL_ATTR_ROWS_FETCHED_PTR not written";
        for (SQLULEN i = 0; i < kArraySize; i++) {
            EXPECT_EQ(rowStatus_[i], SQL_ROW_SUCCESS) << "row status " << i;
            EXPECT_EQ(values_[i], (SQLINTEGER)(i + 1)) << "value " << i;
        }

        ResetOutputs();
        ret = SQLFetch(hStmt);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
        EXPECT_EQ(rowsFetched_, 2u) << "last rowset: SQL_ATTR_ROWS_FETCHED_PTR not written";
        EXPECT_EQ(rowStatus_[0], SQL_ROW_SUCCESS);
        EXPECT_EQ(rowStatus_[1], SQL_ROW_SUCCESS);
        EXPECT_EQ(rowStatus_[2], SQL_ROW_NOROW);
        EXPECT_EQ(rowStatus_[3], SQL_ROW_NOROW);
        EXPECT_EQ(values_[0], 5);
        EXPECT_EQ(values_[1], 6);

        ret = SQLFetch(hStmt);
        EXPECT_EQ(ret, SQL_NO_DATA);
    }
};

// Attributes set before SQLPrepare must survive it.
TEST_F(BlockCursorTest, RowsFetchedPtrSetBeforePrepare) {
    SetRowsetAttrs();

    SQLRETURN ret = SQLPrepare(hStmt, (SQLCHAR*)SixRowsSql(), SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // The attribute must still be readable after the prepare.
    SQLPOINTER ptr = nullptr;
    ret = SQLGetStmtAttr(hStmt, SQL_ATTR_ROW_STATUS_PTR, &ptr, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(ptr, (SQLPOINTER)rowStatus_) << "SQLPrepare discarded SQL_ATTR_ROW_STATUS_PTR";

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    BindColumn();
    FetchAndVerifySixRows();
}

// Attributes set before SQLExecDirect must survive it.
TEST_F(BlockCursorTest, RowsFetchedPtrSetBeforeExecDirect) {
    SetRowsetAttrs();
    ExecDirect(SixRowsSql());
    BindColumn();
    FetchAndVerifySixRows();
}

// Control: attributes set between SQLPrepare and SQLExecute already worked.
TEST_F(BlockCursorTest, RowsFetchedPtrSetAfterPrepare) {
    SQLRETURN ret = SQLPrepare(hStmt, (SQLCHAR*)SixRowsSql(), SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    SetRowsetAttrs();
    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    BindColumn();
    FetchAndVerifySixRows();
}

// A statement handle reused for a second statement keeps the attributes
// (the pattern of an application that configures a handle once).
TEST_F(BlockCursorTest, RowsFetchedPtrSurvivesHandleReuse) {
    SetRowsetAttrs();
    ExecDirect(SixRowsSql());
    BindColumn();
    FetchAndVerifySixRows();

    ASSERT_TRUE(SQL_SUCCEEDED(SQLFreeStmt(hStmt, SQL_CLOSE)));

    ExecDirect(SixRowsSql());
    FetchAndVerifySixRows();
}

// --- Other fetch entry points, layouts and cursor kinds --------------------

// SQLFetchScroll(SQL_FETCH_NEXT) on a forward-only cursor takes the same path
// as SQLFetch and must report the rowset the same way, including a zero
// counter on SQL_NO_DATA.
TEST_F(BlockCursorTest, FetchScrollNextRowsets) {
    SetRowsetAttrs();
    ExecDirect(SixRowsSql());
    BindColumn();

    FetchScrollExpect(SQL_FETCH_NEXT, 0, 4, 1);
    FetchScrollExpect(SQL_FETCH_NEXT, 0, 2, 5);

    ResetOutputs();
    SQLRETURN ret = SQLFetchScroll(hStmt, SQL_FETCH_NEXT, 0);
    EXPECT_EQ(ret, SQL_NO_DATA);
    EXPECT_EQ(rowsFetched_, 0u) << "SQL_NO_DATA must report zero rows fetched";
}

// SQLExtendedFetch receives the counter and the status array as arguments.
TEST_F(BlockCursorTest, ExtendedFetchRowsets) {
    ASSERT_TRUE(SQL_SUCCEEDED(SQLSetStmtAttr(hStmt, SQL_ATTR_ROW_ARRAY_SIZE,
                                             (SQLPOINTER)kArraySize, 0)));
    ExecDirect(SixRowsSql());
    BindColumn();

    ResetOutputs();
    SQLRETURN ret = SQLExtendedFetch(hStmt, SQL_FETCH_NEXT, 0, &rowsFetched_, rowStatus_);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(rowsFetched_, 4u);
    for (SQLULEN i = 0; i < kArraySize; i++) {
        EXPECT_EQ(rowStatus_[i], SQL_ROW_SUCCESS) << "row status " << i;
        EXPECT_EQ(values_[i], (SQLINTEGER)(i + 1)) << "value " << i;
    }

    ResetOutputs();
    ret = SQLExtendedFetch(hStmt, SQL_FETCH_NEXT, 0, &rowsFetched_, rowStatus_);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(rowsFetched_, 2u);
    EXPECT_EQ(rowStatus_[0], SQL_ROW_SUCCESS);
    EXPECT_EQ(rowStatus_[1], SQL_ROW_SUCCESS);
    EXPECT_EQ(rowStatus_[2], SQL_ROW_NOROW);
    EXPECT_EQ(rowStatus_[3], SQL_ROW_NOROW);
    EXPECT_EQ(values_[0], 5);
    EXPECT_EQ(values_[1], 6);

    ResetOutputs();
    ret = SQLExtendedFetch(hStmt, SQL_FETCH_NEXT, 0, &rowsFetched_, rowStatus_);
    EXPECT_EQ(ret, SQL_NO_DATA);
    EXPECT_EQ(rowsFetched_, 0u) << "SQL_NO_DATA must report zero rows fetched";
}

// A rowset larger than the whole result set: one partial fetch, then
// SQL_NO_DATA.
TEST_F(BlockCursorTest, ArraySizeLargerThanResultSet) {
    constexpr SQLULEN kBig = 10;
    SQLULEN fetched = 777;
    SQLUSMALLINT status[kBig];
    SQLINTEGER values[kBig];
    SQLLEN indicators[kBig];
    for (SQLULEN i = 0; i < kBig; i++) {
        status[i] = 9;
        values[i] = -1;
    }

    ASSERT_TRUE(SQL_SUCCEEDED(SQLSetStmtAttr(hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)kBig, 0)));
    ASSERT_TRUE(SQL_SUCCEEDED(SQLSetStmtAttr(hStmt, SQL_ATTR_ROWS_FETCHED_PTR, &fetched, 0)));
    ASSERT_TRUE(SQL_SUCCEEDED(SQLSetStmtAttr(hStmt, SQL_ATTR_ROW_STATUS_PTR, status, 0)));
    ExecDirect(SixRowsSql());
    ASSERT_TRUE(SQL_SUCCEEDED(SQLBindCol(hStmt, 1, SQL_C_SLONG, values, 0, indicators)));

    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(fetched, 6u);
    for (SQLULEN i = 0; i < kBig; i++) {
        if (i < 6) {
            EXPECT_EQ(status[i], SQL_ROW_SUCCESS) << "row status " << i;
            EXPECT_EQ(values[i], (SQLINTEGER)(i + 1)) << "value " << i;
        } else {
            EXPECT_EQ(status[i], SQL_ROW_NOROW) << "row status " << i;
            EXPECT_EQ(values[i], -1) << "value " << i << " must be untouched";
        }
    }

    fetched = 777;
    ret = SQLFetch(hStmt);
    EXPECT_EQ(ret, SQL_NO_DATA);
    EXPECT_EQ(fetched, 0u) << "SQL_NO_DATA must report zero rows fetched";
}

// Row-wise binding: SQL_ATTR_ROW_BIND_TYPE is the size of one row structure,
// and every bound address is advanced by it for each row of the rowset.
TEST_F(BlockCursorTest, RowWiseBinding) {
    struct Row {
        SQLINTEGER id;
        SQLLEN idInd;
        SQLCHAR name[8];
        SQLLEN nameInd;
    };
    Row rows[kArraySize];
    memset(rows, 0, sizeof(rows));

    ASSERT_TRUE(SQL_SUCCEEDED(SQLSetStmtAttr(hStmt, SQL_ATTR_ROW_BIND_TYPE,
                                             (SQLPOINTER)sizeof(Row), 0)));
    SetRowsetAttrs();
    ExecDirect(SixRowsWithNamesSql());
    ASSERT_TRUE(SQL_SUCCEEDED(SQLBindCol(hStmt, 1, SQL_C_SLONG, &rows[0].id, 0, &rows[0].idInd)));
    ASSERT_TRUE(SQL_SUCCEEDED(SQLBindCol(hStmt, 2, SQL_C_CHAR, rows[0].name,
                                         sizeof(rows[0].name), &rows[0].nameInd)));

    ResetOutputs();
    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(rowsFetched_, 4u);
    for (SQLULEN i = 0; i < kArraySize; i++) {
        char expected[8];
        snprintf(expected, sizeof(expected), "r%d", (int)(i + 1));
        EXPECT_EQ(rowStatus_[i], SQL_ROW_SUCCESS) << "row status " << i;
        EXPECT_EQ(rows[i].id, (SQLINTEGER)(i + 1)) << "id " << i;
        EXPECT_EQ(rows[i].idInd, (SQLLEN)sizeof(SQLINTEGER)) << "id indicator " << i;
        EXPECT_STREQ((const char*)rows[i].name, expected) << "name " << i;
        EXPECT_EQ(rows[i].nameInd, 2) << "name indicator " << i;
    }

    memset(rows, 0, sizeof(rows));
    ResetOutputs();
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(rowsFetched_, 2u);
    EXPECT_EQ(rows[0].id, 5);
    EXPECT_STREQ((const char*)rows[0].name, "r5");
    EXPECT_EQ(rows[1].id, 6);
    EXPECT_STREQ((const char*)rows[1].name, "r6");
    EXPECT_EQ(rows[2].id, 0) << "row 2 must be untouched";
    EXPECT_EQ(rowStatus_[2], SQL_ROW_NOROW);
}

// Column-wise binding with SQL_ATTR_ROW_BIND_OFFSET_PTR: the offset is added
// to every bound data and indicator address when the rowset is fetched, and
// the rows still step through the arrays element by element.
//
// Skipped: the driver routes any bind-offset pointer through its row-wise
// fetch path, which advances the addresses by SQL_ATTR_ROW_BIND_TYPE per
// row. That is 0 for column-wise binding, so every row of the rowset is
// written to the same (shifted) first element while the counter and the
// status array still report a full rowset.
TEST_F(BlockCursorTest, ColumnWiseBindingWithOffset) {
    GTEST_SKIP() << "Driver defect: column-wise binding with SQL_ATTR_ROW_BIND_OFFSET_PTR "
                    "writes every row of a rowset to the same address (row stride 0)";

    constexpr SQLULEN kSlots = 32;
    SQLINTEGER values[kSlots];
    SQLLEN indicators[kSlots];
    for (SQLULEN i = 0; i < kSlots; i++) {
        values[i] = -1;
        indicators[i] = -99;
    }
    // A multiple of both element sizes, so the shifted rows start on an
    // element boundary of each array.
    SQLLEN offset = 8 * sizeof(SQLLEN);
    const SQLULEN valueBase = offset / sizeof(SQLINTEGER);
    const SQLULEN indBase = offset / sizeof(SQLLEN);

    SetRowsetAttrs();
    ASSERT_TRUE(SQL_SUCCEEDED(SQLSetStmtAttr(hStmt, SQL_ATTR_ROW_BIND_OFFSET_PTR, &offset, 0)));
    ExecDirect(SixRowsSql());
    ASSERT_TRUE(SQL_SUCCEEDED(SQLBindCol(hStmt, 1, SQL_C_SLONG, values, 0, indicators)));

    ResetOutputs();
    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(rowsFetched_, 4u);
    for (SQLULEN i = 0; i < kArraySize; i++) {
        EXPECT_EQ(values[valueBase + i], (SQLINTEGER)(i + 1)) << "shifted value " << i;
        EXPECT_EQ(indicators[indBase + i], (SQLLEN)sizeof(SQLINTEGER)) << "shifted indicator " << i;
        EXPECT_EQ(values[i], -1) << "unshifted slot " << i << " must be untouched";
    }

    // Dropping the offset to zero before the next fetch moves the rows back
    // to the start of the arrays.
    offset = 0;
    ResetOutputs();
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(rowsFetched_, 2u);
    EXPECT_EQ(values[0], 5);
    EXPECT_EQ(values[1], 6);
    EXPECT_EQ(indicators[0], (SQLLEN)sizeof(SQLINTEGER));
    EXPECT_EQ(indicators[1], (SQLLEN)sizeof(SQLINTEGER));
    EXPECT_EQ(values[2], -1) << "slot 2 must be untouched by a two-row rowset";
}

// A static scrollable cursor fetches rowsets in every orientation and
// reports each one through the same counter and status array.
TEST_F(BlockCursorTest, StaticCursorRowsets) {
    ASSERT_TRUE(SQL_SUCCEEDED(SQLSetStmtAttr(hStmt, SQL_ATTR_CURSOR_TYPE,
                                             (SQLPOINTER)SQL_CURSOR_STATIC, 0)));
    ASSERT_TRUE(SQL_SUCCEEDED(SQLSetStmtAttr(hStmt, SQL_ATTR_CURSOR_SCROLLABLE,
                                             (SQLPOINTER)SQL_SCROLLABLE, 0)));
    SetRowsetAttrs();
    ExecDirect(SixRowsSql());
    BindColumn();

    FetchScrollExpect(SQL_FETCH_NEXT, 0, 4, 1);
    FetchScrollExpect(SQL_FETCH_NEXT, 0, 2, 5);

    ResetOutputs();
    SQLRETURN ret = SQLFetchScroll(hStmt, SQL_FETCH_NEXT, 0);
    EXPECT_EQ(ret, SQL_NO_DATA);
    EXPECT_EQ(rowsFetched_, 0u) << "SQL_NO_DATA must report zero rows fetched";

    FetchScrollExpect(SQL_FETCH_FIRST, 0, 4, 1);
    FetchScrollExpect(SQL_FETCH_ABSOLUTE, 5, 2, 5);
    FetchScrollExpect(SQL_FETCH_ABSOLUTE, 3, 4, 3);
    FetchScrollExpect(SQL_FETCH_LAST, 0, 4, 3);
    FetchScrollExpect(SQL_FETCH_FIRST, 0, 4, 1);
    FetchScrollExpect(SQL_FETCH_RELATIVE, 2, 4, 3);
}

// Catalog result sets from SQLTables take the static-cursor fetch path.
TEST_F(BlockCursorTest, CatalogRowsetsViaSqlTables) {
    ExpectCatalogRowsets([](SQLHSTMT s) {
        return SQLTables(s, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
    });
}

// SQLGetTypeInfo's result set takes the ordinary forward fetch path.
TEST_F(BlockCursorTest, CatalogRowsetsViaSqlGetTypeInfo) {
    ExpectCatalogRowsets([](SQLHSTMT s) {
        return SQLGetTypeInfo(s, SQL_ALL_TYPES);
    });
}
