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
    GTEST_SKIP() << "Hangs on Linux: SQLDisconnect deadlocks in mutex after SQLCopyDesc";
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

// ===== SQLCopyDesc crash tests (from ODBC Crusher OC-1) =====

class CopyDescCrashTest : public OdbcConnectedTest {};

TEST_F(CopyDescCrashTest, CopyEmptyARDDoesNotCrash) {
    GTEST_SKIP() << "Requires Phase 7 (OC-1): SQLCopyDesc null records crash fix";
    // Allocate two statements with no bindings (empty ARDs)
    SQLHSTMT stmt1 = AllocExtraStmt();
    SQLHSTMT stmt2 = AllocExtraStmt();

    // Get ARD handles (both have no records — records pointer is NULL)
    SQLHDESC hArd1 = SQL_NULL_HDESC;
    SQLHDESC hArd2 = SQL_NULL_HDESC;
    SQLRETURN ret;

    ret = SQLGetStmtAttr(stmt1, SQL_ATTR_APP_ROW_DESC, &hArd1, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ASSERT_NE(hArd1, (SQLHDESC)SQL_NULL_HDESC);

    ret = SQLGetStmtAttr(stmt2, SQL_ATTR_APP_ROW_DESC, &hArd2, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ASSERT_NE(hArd2, (SQLHDESC)SQL_NULL_HDESC);

    // This previously crashed with access violation (0xC0000005)
    // because operator= tried to dereference sour.records[0] when sour.records was NULL
    ret = SQLCopyDesc(hArd1, hArd2);
    EXPECT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLCopyDesc of empty ARD should succeed, got: "
        << GetOdbcError(SQL_HANDLE_DESC, hArd2);

    // The key test is that we got here without crashing.
    // Note: The DM may report its own descriptor count for implicit descriptors,
    // so we only verify that SQLGetDescField itself succeeds.
    SQLINTEGER count = -1;
    ret = SQLGetDescField(hArd2, 0, SQL_DESC_COUNT, &count, 0, NULL);
    EXPECT_TRUE(SQL_SUCCEEDED(ret));

    SQLFreeHandle(SQL_HANDLE_STMT, stmt1);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt2);
}

TEST_F(CopyDescCrashTest, CopyEmptyToExplicitDescriptor) {
    GTEST_SKIP() << "Requires Phase 7 (OC-1): SQLCopyDesc null records crash fix";
    // Allocate an explicit descriptor
    SQLHDESC hExplicit = SQL_NULL_HDESC;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_DESC, hDbc, &hExplicit);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Get the ARD of a statement with no bindings
    SQLHDESC hArd = SQL_NULL_HDESC;
    ret = SQLGetStmtAttr(hStmt, SQL_ATTR_APP_ROW_DESC, &hArd, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Copy empty ARD to explicit descriptor — must not crash
    ret = SQLCopyDesc(hArd, hExplicit);
    EXPECT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLCopyDesc from empty ARD to explicit desc failed: "
        << GetOdbcError(SQL_HANDLE_DESC, hExplicit);

    SQLFreeHandle(SQL_HANDLE_DESC, hExplicit);
}

TEST_F(CopyDescCrashTest, CopyPopulatedThenEmpty) {
    GTEST_SKIP() << "Requires Phase 7 (OC-1): SQLCopyDesc null records crash fix";
    // First, populate an explicit descriptor by copying a populated ARD
    SQLINTEGER val = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &val, sizeof(val), &ind);

    SQLHDESC hArd = SQL_NULL_HDESC;
    SQLGetStmtAttr(hStmt, SQL_ATTR_APP_ROW_DESC, &hArd, 0, NULL);

    SQLHDESC hExplicit = SQL_NULL_HDESC;
    SQLAllocHandle(SQL_HANDLE_DESC, hDbc, &hExplicit);

    SQLRETURN ret = SQLCopyDesc(hArd, hExplicit);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Verify count = 1
    SQLINTEGER count = 0;
    SQLGetDescField(hExplicit, 0, SQL_DESC_COUNT, &count, 0, NULL);
    EXPECT_EQ(count, 1);

    // Now allocate a second explicit descriptor (which is truly empty)
    SQLHDESC hEmpty = SQL_NULL_HDESC;
    SQLAllocHandle(SQL_HANDLE_DESC, hDbc, &hEmpty);

    // Copy the empty explicit descriptor over the populated one — must not crash
    ret = SQLCopyDesc(hEmpty, hExplicit);
    EXPECT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLCopyDesc of empty explicit desc over populated desc failed: "
        << GetOdbcError(SQL_HANDLE_DESC, hExplicit);

    // For explicit→explicit copy (no DM interception), count should be 0
    count = 0;
    SQLGetDescField(hExplicit, 0, SQL_DESC_COUNT, &count, 0, NULL);
    EXPECT_EQ(count, 0);

    SQLFreeHandle(SQL_HANDLE_DESC, hEmpty);
    SQLFreeHandle(SQL_HANDLE_DESC, hExplicit);
}

// OC-1 Root Cause 1: SQLSetDescField(SQL_DESC_COUNT) must allocate records
TEST_F(CopyDescCrashTest, SetDescCountAllocatesRecords) {
    GTEST_SKIP() << "Requires Phase 7 (OC-1): SetDescCount record allocation fix";
    // Allocate an explicit descriptor
    SQLHDESC hDesc = SQL_NULL_HDESC;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_DESC, hDbc, &hDesc);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Set SQL_DESC_COUNT to 3 — this should allocate the records array
    ret = SQLSetDescField(hDesc, 0, SQL_DESC_COUNT, (SQLPOINTER)3, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLSetDescField(SQL_DESC_COUNT, 3) failed: "
        << GetOdbcError(SQL_HANDLE_DESC, hDesc);

    // Verify the count is 3
    SQLSMALLINT count = 0;
    ret = SQLGetDescField(hDesc, 0, SQL_DESC_COUNT, &count, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(count, 3);

    // Now set a field on record 2 — this must NOT crash
    // (Previously, records array wasn't allocated, so this would dereference NULL)
    ret = SQLSetDescField(hDesc, 2, SQL_DESC_TYPE, (SQLPOINTER)SQL_C_SLONG, 0);
    EXPECT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLSetDescField on record 2 after setting COUNT failed: "
        << GetOdbcError(SQL_HANDLE_DESC, hDesc);

    SQLFreeHandle(SQL_HANDLE_DESC, hDesc);
}

TEST_F(CopyDescCrashTest, SetDescCountThenCopyDesc) {
    GTEST_SKIP() << "Requires Phase 7 (OC-1): SetDescCount record allocation fix";
    // This is the exact odbc-crusher scenario: set SQL_DESC_COUNT then SQLCopyDesc
    SQLHDESC hSrc = SQL_NULL_HDESC;
    SQLHDESC hDst = SQL_NULL_HDESC;
    SQLAllocHandle(SQL_HANDLE_DESC, hDbc, &hSrc);
    SQLAllocHandle(SQL_HANDLE_DESC, hDbc, &hDst);

    // Set count on source without binding any columns
    SQLRETURN ret = SQLSetDescField(hSrc, 0, SQL_DESC_COUNT, (SQLPOINTER)5, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Copy source to destination — must not crash
    ret = SQLCopyDesc(hSrc, hDst);
    EXPECT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLCopyDesc after SQLSetDescField(COUNT) failed: "
        << GetOdbcError(SQL_HANDLE_DESC, hDst);

    // Verify destination has count = 5
    SQLSMALLINT count = 0;
    ret = SQLGetDescField(hDst, 0, SQL_DESC_COUNT, &count, 0, NULL);
    EXPECT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(count, 5);

    SQLFreeHandle(SQL_HANDLE_DESC, hSrc);
    SQLFreeHandle(SQL_HANDLE_DESC, hDst);
}

TEST_F(CopyDescCrashTest, SetDescCountReduceFreesRecords) {
    GTEST_SKIP() << "Requires Phase 7 (OC-1): SetDescCount record allocation fix";
    // Allocate explicit descriptor and set up 3 records
    SQLHDESC hDesc = SQL_NULL_HDESC;
    SQLAllocHandle(SQL_HANDLE_DESC, hDbc, &hDesc);

    SQLSetDescField(hDesc, 0, SQL_DESC_COUNT, (SQLPOINTER)3, 0);

    // Set type on record 3 to verify it exists
    SQLRETURN ret = SQLSetDescField(hDesc, 3, SQL_DESC_TYPE, (SQLPOINTER)SQL_C_CHAR, 0);
    EXPECT_TRUE(SQL_SUCCEEDED(ret));

    // Reduce count to 1 — records 2 and 3 should be freed
    ret = SQLSetDescField(hDesc, 0, SQL_DESC_COUNT, (SQLPOINTER)1, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLSMALLINT count = 0;
    SQLGetDescField(hDesc, 0, SQL_DESC_COUNT, &count, 0, NULL);
    EXPECT_EQ(count, 1);

    SQLFreeHandle(SQL_HANDLE_DESC, hDesc);
}

TEST_F(CopyDescCrashTest, SetDescCountToZeroUnbindsAll) {
    GTEST_SKIP() << "Requires Phase 7 (OC-1): SetDescCount record allocation fix";
    // Allocate explicit descriptor and set up records
    SQLHDESC hDesc = SQL_NULL_HDESC;
    SQLAllocHandle(SQL_HANDLE_DESC, hDbc, &hDesc);

    SQLSetDescField(hDesc, 0, SQL_DESC_COUNT, (SQLPOINTER)2, 0);

    // Set count to 0 — should unbind all
    SQLRETURN ret = SQLSetDescField(hDesc, 0, SQL_DESC_COUNT, (SQLPOINTER)0, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLSMALLINT count = 99;
    SQLGetDescField(hDesc, 0, SQL_DESC_COUNT, &count, 0, NULL);
    EXPECT_EQ(count, 0);

    SQLFreeHandle(SQL_HANDLE_DESC, hDesc);
}

// ===== IRD before SQLPrepare (#307) =====
TEST_F(DescriptorTest, IrdHeaderFieldsReadableBeforePrepare) {
    SQLHDESC hIrd = SQL_NULL_HDESC;
    ASSERT_TRUE(SQL_SUCCEEDED(SQLGetStmtAttr(hStmt, SQL_ATTR_IMP_ROW_DESC, &hIrd, 0, NULL)));
    ASSERT_NE(hIrd, (SQLHDESC)SQL_NULL_HDESC);

    SQLULEN rowsFetched = 0;
    SQLUSMALLINT rowStatus[4] = {};
    ASSERT_TRUE(SQL_SUCCEEDED(SQLSetStmtAttr(hStmt, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetched, 0)));
    ASSERT_TRUE(SQL_SUCCEEDED(SQLSetStmtAttr(hStmt, SQL_ATTR_ROW_STATUS_PTR, rowStatus, 0)));

    SQLPOINTER ptr = NULL;
    SQLRETURN ret = SQLGetDescField(hIrd, 0, SQL_DESC_ROWS_PROCESSED_PTR, &ptr, 0, NULL);
    if (ret == SQL_ERROR && GetSqlState(SQL_HANDLE_DESC, hIrd) == "HY007") {
        // unixODBC answers HY007 for an unprepared IRD before the driver sees the call
        GTEST_SKIP() << "the driver manager enforces HY007 itself: " << GetOdbcError(SQL_HANDLE_DESC, hIrd);
    }
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_DESC, hIrd);
    EXPECT_EQ(ptr, (SQLPOINTER)&rowsFetched);

    ptr = NULL;
    ret = SQLGetDescField(hIrd, 0, SQL_DESC_ARRAY_STATUS_PTR, &ptr, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_DESC, hIrd);
    EXPECT_EQ(ptr, (SQLPOINTER)rowStatus);

    SQLSMALLINT allocType = 0;
    ret = SQLGetDescField(hIrd, 0, SQL_DESC_ALLOC_TYPE, &allocType, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_DESC, hIrd);
    EXPECT_EQ(allocType, SQL_DESC_ALLOC_AUTO);
}

TEST_F(DescriptorTest, IrdRecordFieldsBeforePrepareAreHY007) {
    SQLHDESC hIrd = SQL_NULL_HDESC;
    ASSERT_TRUE(SQL_SUCCEEDED(SQLGetStmtAttr(hStmt, SQL_ATTR_IMP_ROW_DESC, &hIrd, 0, NULL)));
    ASSERT_NE(hIrd, (SQLHDESC)SQL_NULL_HDESC);

    SQLSMALLINT count = -1;
    SQLRETURN ret = SQLGetDescField(hIrd, 0, SQL_DESC_COUNT, &count, 0, NULL);
    EXPECT_EQ(ret, SQL_ERROR);
    EXPECT_EQ(GetSqlState(SQL_HANDLE_DESC, hIrd), "HY007");

    SQLSMALLINT type = 0;
    ret = SQLGetDescField(hIrd, 1, SQL_DESC_TYPE, &type, 0, NULL);
    EXPECT_EQ(ret, SQL_ERROR);
    EXPECT_EQ(GetSqlState(SQL_HANDLE_DESC, hIrd), "HY007");

    // Both answer once the statement is prepared
    ret = SQLPrepare(hStmt, (SQLCHAR*)"SELECT 1 AS A FROM RDB$DATABASE", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    ret = SQLGetDescField(hIrd, 0, SQL_DESC_COUNT, &count, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_DESC, hIrd);
    EXPECT_EQ(count, 1);
    ret = SQLGetDescField(hIrd, 1, SQL_DESC_TYPE, &type, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_DESC, hIrd);
}

// ===== IRD records after SQLPrepare, before any bind or execute (#316) =====
TEST_F(DescriptorTest, IrdRecordsDescribeColumnsAfterPrepare) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT CAST(1 AS INTEGER) AS INTCOL, CAST('x' AS VARCHAR(5)) AS VARCOL "
                  "FROM RDB$DATABASE",
        SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    SQLHDESC hIrd = SQL_NULL_HDESC;
    ASSERT_TRUE(SQL_SUCCEEDED(SQLGetStmtAttr(hStmt, SQL_ATTR_IMP_ROW_DESC, &hIrd, 0, NULL)));

    // Descriptor reads first, before anything binds or executes the columns
    for (SQLSMALLINT col = 1; col <= 2; col++) {
        SQLSMALLINT descType = 0, conciseType = 0;
        SQLCHAR descName[64] = {};
        SQLINTEGER nameLen = 0;
        ret = SQLGetDescField(hIrd, col, SQL_DESC_TYPE, &descType, 0, NULL);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_DESC, hIrd);
        ret = SQLGetDescField(hIrd, col, SQL_DESC_CONCISE_TYPE, &conciseType, 0, NULL);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_DESC, hIrd);
        ret = SQLGetDescField(hIrd, col, SQL_DESC_NAME, descName, sizeof(descName), &nameLen);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_DESC, hIrd);

        SQLCHAR recName[64] = {};
        SQLSMALLINT recNameLen = 0, recType = 0, recSubType = 0, recPrecision = 0, recScale = 0,
                    recNullable = 0;
        SQLLEN recLength = 0;
        ret = SQLGetDescRec(hIrd, col, recName, sizeof(recName), &recNameLen, &recType, &recSubType,
                            &recLength, &recPrecision, &recScale, &recNullable);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_DESC, hIrd);

        SQLCHAR colName[64] = {};
        SQLSMALLINT colNameLen = 0, dataType = 0, decimalDigits = 0, nullable = 0;
        SQLULEN columnSize = 0;
        ret = SQLDescribeCol(hStmt, col, colName, sizeof(colName), &colNameLen, &dataType,
                             &columnSize, &decimalDigits, &nullable);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

        EXPECT_NE(descType, SQL_C_DEFAULT) << "column " << col << " still holds the record default";
        EXPECT_NE(conciseType, SQL_C_DEFAULT) << "column " << col;
        EXPECT_STREQ((char*)descName, (char*)colName) << "column " << col;
        EXPECT_NE(recType, SQL_C_DEFAULT) << "column " << col;
        EXPECT_STREQ((char*)recName, (char*)colName) << "column " << col;
        if (col == 1) {
            EXPECT_EQ(descType, dataType) << "INTEGER column";
            EXPECT_EQ(recType, dataType) << "INTEGER column";
        }
    }
}

TEST_F(DescriptorTest, CopyDescFromIrdAfterPrepare) {
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT CAST(1 AS INTEGER) AS INTCOL FROM RDB$DATABASE", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);
    SQLHDESC hIrd = SQL_NULL_HDESC;
    ASSERT_TRUE(SQL_SUCCEEDED(SQLGetStmtAttr(hStmt, SQL_ATTR_IMP_ROW_DESC, &hIrd, 0, NULL)));

    SQLCHAR colName[64] = {};
    SQLSMALLINT colNameLen = 0, dataType = 0, decimalDigits = 0, nullable = 0;
    SQLULEN columnSize = 0;
    ret = SQLDescribeCol(hStmt, 1, colName, sizeof(colName), &colNameLen, &dataType,
                         &columnSize, &decimalDigits, &nullable);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Copy the IRD before anything binds or executes the column
    SQLHDESC hCopy = SQL_NULL_HDESC;
    ASSERT_TRUE(SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_DESC, hDbc, &hCopy)));
    ret = SQLCopyDesc(hIrd, hCopy);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_DESC, hCopy);

    SQLSMALLINT descType = 0;
    ret = SQLGetDescField(hCopy, 1, SQL_DESC_TYPE, &descType, 0, NULL);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_DESC, hCopy);
    EXPECT_EQ(descType, dataType);
    SQLFreeHandle(SQL_HANDLE_DESC, hCopy);
}
