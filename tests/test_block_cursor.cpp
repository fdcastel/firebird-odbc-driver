// tests/test_block_cursor.cpp — Block cursor (SQL_ATTR_ROW_ARRAY_SIZE > 1) tests
//
// Covers the IRD header fields an application reads back after a rowset
// fetch: SQL_ATTR_ROWS_FETCHED_PTR and SQL_ATTR_ROW_STATUS_PTR. Statement
// attributes persist until the statement is freed or the attribute is set
// again; in particular SQLPrepare / SQLExecDirect must not discard them.

#include "test_helpers.h"

class BlockCursorTest : public OdbcConnectedTest {
protected:
    static constexpr SQLULEN kArraySize = 4;

    // Six rows, no table and no parameters needed.
    static const char* SixRowsSql() {
        return "SELECT 1 FROM rdb$database UNION ALL SELECT 2 FROM rdb$database"
               " UNION ALL SELECT 3 FROM rdb$database UNION ALL SELECT 4 FROM rdb$database"
               " UNION ALL SELECT 5 FROM rdb$database UNION ALL SELECT 6 FROM rdb$database";
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
