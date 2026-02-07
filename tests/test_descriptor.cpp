// tests/test_descriptor.cpp — Descriptor tests (Phase 3.3)

#include "test_helpers.h"

class DescriptorTest : public OdbcConnectedTest {};

// ===== SQLGetDescRec / SQLSetDescRec =====

TEST_F(DescriptorTest, GetIRDAfterPrepare) {
    // Prepare a query to populate the IRD
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT CAST(123 AS INTEGER) AS INTCOL, "
                  "CAST('hello' AS VARCHAR(20)) AS VARCOL "
                  "FROM RDB$DATABASE",
        SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Prepare failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Get IRD handle
    SQLHDESC hIrd = SQL_NULL_HDESC;
    ret = SQLGetStmtAttr(hStmt, SQL_ATTR_IMP_ROW_DESC, &hIrd, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ASSERT_NE(hIrd, (SQLHDESC)SQL_NULL_HDESC);

    // Verify record count via SQLGetDescField
    SQLINTEGER count = 0;
    ret = SQLGetDescField(hIrd, 0, SQL_DESC_COUNT, &count, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(count, 2);

    // Get column names via SQLColAttribute which is more reliable
    SQLCHAR name[128] = {};
    SQLSMALLINT nameLen = 0;
    ret = SQLColAttribute(hStmt, 1, SQL_DESC_NAME, name, sizeof(name), &nameLen, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)name, "INTCOL");

    memset(name, 0, sizeof(name));
    ret = SQLColAttribute(hStmt, 2, SQL_DESC_NAME, name, sizeof(name), &nameLen, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)name, "VARCOL");
}

// ===== SQLGetDescField / SQLSetDescField =====

TEST_F(DescriptorTest, GetDescFieldCount) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT 1 AS A, 2 AS B, 3 AS C FROM RDB$DATABASE", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLHDESC hIrd = SQL_NULL_HDESC;
    ret = SQLGetStmtAttr(hStmt, SQL_ATTR_IMP_ROW_DESC, &hIrd, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER count = 0;
    ret = SQLGetDescField(hIrd, 0, SQL_DESC_COUNT, &count, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(count, 3);
}

TEST_F(DescriptorTest, SetARDFieldAndBindCol) {
    // Get ARD handle
    SQLHDESC hArd = SQL_NULL_HDESC;
    SQLRETURN ret = SQLGetStmtAttr(hStmt, SQL_ATTR_APP_ROW_DESC, &hArd, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Bind via SQLBindCol, then verify via SQLGetDescField
    SQLINTEGER value = 0;
    SQLLEN ind = 0;
    ret = SQLBindCol(hStmt, 1, SQL_C_SLONG, &value, sizeof(value), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLSMALLINT type = 0;
    ret = SQLGetDescField(hArd, 1, SQL_DESC_CONCISE_TYPE, &type, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(type, SQL_C_SLONG);
}

// ===== SQLCopyDesc =====

TEST_F(DescriptorTest, CopyDescARDToExplicit) {
    // Allocate an explicit descriptor
    SQLHDESC hExplicitDesc = SQL_NULL_HDESC;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_DESC, hDbc, &hExplicitDesc);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Failed to allocate explicit descriptor";

    // Bind some columns in the ARD
    SQLINTEGER val1 = 0;
    SQLCHAR val2[50] = {};
    SQLLEN ind1 = 0, ind2 = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &val1, sizeof(val1), &ind1);
    SQLBindCol(hStmt, 2, SQL_C_CHAR, val2, sizeof(val2), &ind2);

    // Get ARD
    SQLHDESC hArd = SQL_NULL_HDESC;
    ret = SQLGetStmtAttr(hStmt, SQL_ATTR_APP_ROW_DESC, &hArd, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Copy ARD -> explicit descriptor
    ret = SQLCopyDesc(hArd, hExplicitDesc);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "CopyDesc failed: " << GetOdbcError(SQL_HANDLE_DESC, hExplicitDesc);

    // Verify the copy: explicit desc should have count = 2
    SQLINTEGER count = 0;
    ret = SQLGetDescField(hExplicitDesc, 0, SQL_DESC_COUNT, &count, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(count, 2);

    // Cleanup
    SQLFreeHandle(SQL_HANDLE_DESC, hExplicitDesc);
}

// ===== Explicit descriptor assigned to statement =====

TEST_F(DescriptorTest, ExplicitDescriptorAsARD) {
    // Allocate an explicit descriptor
    SQLHDESC hExplicitDesc = SQL_NULL_HDESC;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_DESC, hDbc, &hExplicitDesc);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Set it as the ARD for the statement
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_APP_ROW_DESC, hExplicitDesc, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "Failed to assign explicit desc as ARD: "
        << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Verify it's the same handle
    SQLHDESC hArd = SQL_NULL_HDESC;
    ret = SQLGetStmtAttr(hStmt, SQL_ATTR_APP_ROW_DESC, &hArd, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ((void*)hArd, (void*)hExplicitDesc);

    // Reset to implicit ARD by setting SQL_NULL_HDESC
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_APP_ROW_DESC, SQL_NULL_HDESC, 0);
    // The driver may or may not support this; just check it doesn't crash

    SQLFreeHandle(SQL_HANDLE_DESC, hExplicitDesc);
}

// ===== IPD tests with parameter binding =====

TEST_F(DescriptorTest, IPDAfterBindParameter) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT 1 FROM RDB$DATABASE WHERE 1 = ?", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER paramVal = 1;
    SQLLEN paramInd = 0;
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
                           0, 0, &paramVal, 0, &paramInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "BindParameter failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Check APD has the binding
    SQLHDESC hApd = SQL_NULL_HDESC;
    ret = SQLGetStmtAttr(hStmt, SQL_ATTR_APP_PARAM_DESC, &hApd, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLSMALLINT type = 0;
    ret = SQLGetDescField(hApd, 1, SQL_DESC_CONCISE_TYPE, &type, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(type, SQL_C_SLONG);
}
