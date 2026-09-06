// tests/test_stmt_attrs.cpp — Statement attribute round trips
//
// Every statement attribute the driver accepts in SQLSetStmtAttr must come
// back unchanged from SQLGetStmtAttr. The pointer attributes live in the
// descriptor headers and are only dereferenced by a later fetch or execute,
// so any address will do here; nothing is fetched or executed.

#include "test_helpers.h"

class StmtAttrTest : public OdbcConnectedTest {
protected:
    // Sets `attr` to a pointer, reads it back, then sets it to NULL and
    // reads that back as well.
    void ExpectPointerRoundTrip(SQLINTEGER attr, const char* name) {
        SQLULEN sink[4] = {};
        SQLPOINTER in = sink;

        SQLRETURN ret = SQLSetStmtAttr(hStmt, attr, in, 0);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << name << ": set failed: "
                                        << GetOdbcError(SQL_HANDLE_STMT, hStmt);

        SQLPOINTER out = nullptr;
        ret = SQLGetStmtAttr(hStmt, attr, &out, 0, nullptr);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << name << ": get failed: "
                                        << GetOdbcError(SQL_HANDLE_STMT, hStmt);
        EXPECT_EQ(out, in) << name;

        ret = SQLSetStmtAttr(hStmt, attr, nullptr, 0);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << name << ": set NULL failed: "
                                        << GetOdbcError(SQL_HANDLE_STMT, hStmt);

        out = in;
        ret = SQLGetStmtAttr(hStmt, attr, &out, 0, nullptr);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << name << ": get after NULL failed: "
                                        << GetOdbcError(SQL_HANDLE_STMT, hStmt);
        EXPECT_EQ(out, nullptr) << name;
    }
};

TEST_F(StmtAttrTest, RowsFetchedPtr) {
    ExpectPointerRoundTrip(SQL_ATTR_ROWS_FETCHED_PTR, "SQL_ATTR_ROWS_FETCHED_PTR");
}

TEST_F(StmtAttrTest, RowBindOffsetPtr) {
    ExpectPointerRoundTrip(SQL_ATTR_ROW_BIND_OFFSET_PTR, "SQL_ATTR_ROW_BIND_OFFSET_PTR");
}

TEST_F(StmtAttrTest, RowOperationPtr) {
    ExpectPointerRoundTrip(SQL_ATTR_ROW_OPERATION_PTR, "SQL_ATTR_ROW_OPERATION_PTR");
}

TEST_F(StmtAttrTest, ParamsProcessedPtr) {
    ExpectPointerRoundTrip(SQL_ATTR_PARAMS_PROCESSED_PTR, "SQL_ATTR_PARAMS_PROCESSED_PTR");
}

TEST_F(StmtAttrTest, ParamBindOffsetPtr) {
    ExpectPointerRoundTrip(SQL_ATTR_PARAM_BIND_OFFSET_PTR, "SQL_ATTR_PARAM_BIND_OFFSET_PTR");
}

TEST_F(StmtAttrTest, ParamOperationPtr) {
    ExpectPointerRoundTrip(SQL_ATTR_PARAM_OPERATION_PTR, "SQL_ATTR_PARAM_OPERATION_PTR");
}

TEST_F(StmtAttrTest, ParamStatusPtr) {
    ExpectPointerRoundTrip(SQL_ATTR_PARAM_STATUS_PTR, "SQL_ATTR_PARAM_STATUS_PTR");
}

// Control: the one pointer attribute that could already be read back.
TEST_F(StmtAttrTest, RowStatusPtr) {
    ExpectPointerRoundTrip(SQL_ATTR_ROW_STATUS_PTR, "SQL_ATTR_ROW_STATUS_PTR");
}

TEST_F(StmtAttrTest, CursorScrollable) {
    SQLULEN value = 999;
    SQLRETURN ret = SQLGetStmtAttr(hStmt, SQL_ATTR_CURSOR_SCROLLABLE, &value, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(value, (SQLULEN)SQL_NONSCROLLABLE) << "default";

    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER)SQL_SCROLLABLE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    value = 999;
    ret = SQLGetStmtAttr(hStmt, SQL_ATTR_CURSOR_SCROLLABLE, &value, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(value, (SQLULEN)SQL_SCROLLABLE);

    // Asking for a scrollable cursor moves the cursor type off forward-only.
    value = 999;
    ret = SQLGetStmtAttr(hStmt, SQL_ATTR_CURSOR_TYPE, &value, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_NE(value, (SQLULEN)SQL_CURSOR_FORWARD_ONLY);

    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER)SQL_NONSCROLLABLE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    value = 999;
    ret = SQLGetStmtAttr(hStmt, SQL_ATTR_CURSOR_SCROLLABLE, &value, 0, nullptr);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(value, (SQLULEN)SQL_NONSCROLLABLE);
}
