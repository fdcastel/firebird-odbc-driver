// test_batch_params.cpp — Tests for batch parameter execution (Task 4.3)
#include "test_helpers.h"

// ============================================================================
// BatchParamTest: Validate SQL_ATTR_PARAMSET_SIZE > 1 batch execution
// ============================================================================
class BatchParamTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (hDbc == SQL_NULL_HDBC) return;

        // Create test table
        ExecIgnoreError("DROP TABLE BATCH_TEST");
        Commit();
        ReallocStmt();
        ExecDirect("CREATE TABLE BATCH_TEST (ID INTEGER NOT NULL PRIMARY KEY, VAL VARCHAR(50))");
        Commit();
        ReallocStmt();
    }

    void TearDown() override {
        if (hDbc != SQL_NULL_HDBC) {
            ExecIgnoreError("DROP TABLE BATCH_TEST");
            SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
        }
        OdbcConnectedTest::TearDown();
    }
};

TEST_F(BatchParamTest, InsertWithRowWiseBinding) {
    // Batch insert 5 rows using row-wise binding (preferred for this driver)
    const int BATCH_SIZE = 5;
    SQLRETURN ret;

    // Row-wise binding structure — must be packed consistently
    struct ParamRow {
        SQLINTEGER id;
        SQLLEN idInd;
        SQLCHAR val[51];
        SQLLEN valInd;
    };

    ParamRow rows[BATCH_SIZE] = {};
    rows[0] = {100, 0, "Alpha",   SQL_NTS};
    rows[1] = {200, 0, "Bravo",   SQL_NTS};
    rows[2] = {300, 0, "Charlie", SQL_NTS};
    rows[3] = {400, 0, "Delta",   SQL_NTS};
    rows[4] = {500, 0, "Echo",    SQL_NTS};

    // Set row-wise binding BEFORE SQLPrepare
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, (SQLPOINTER)(intptr_t)sizeof(ParamRow), 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)BATCH_SIZE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Status array
    SQLUSMALLINT paramStatus[BATCH_SIZE] = {};
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_STATUS_PTR, paramStatus, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Rows processed
    SQLULEN paramsProcessed = 0;
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &paramsProcessed, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Prepare
    ret = SQLPrepare(hStmt, (SQLCHAR*)"INSERT INTO BATCH_TEST (ID, VAL) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Bind parameters pointing to first row
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
                           0, 0, &rows[0].id, sizeof(rows[0].id), &rows[0].idInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
                           50, 0, rows[0].val, 51, &rows[0].valInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Execute batch
    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Verify all rows processed
    EXPECT_EQ(paramsProcessed, (SQLULEN)BATCH_SIZE);

    // Verify all status entries are success
    for (int i = 0; i < BATCH_SIZE; ++i) {
        EXPECT_TRUE(paramStatus[i] == SQL_PARAM_SUCCESS || paramStatus[i] == SQL_PARAM_SUCCESS_WITH_INFO)
            << "Row " << i << " status: " << paramStatus[i];
    }

    Commit();
    ReallocStmt();

    // Verify all rows were inserted
    ExecDirect("SELECT COUNT(*) FROM BATCH_TEST");
    SQLINTEGER count = 0;
    SQLLEN countInd = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &count, sizeof(count), &countInd);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(count, BATCH_SIZE);
}

TEST_F(BatchParamTest, RowWiseVerifyValues) {
    // Insert rows with row-wise binding and verify values match
    const int BATCH_SIZE = 3;
    SQLRETURN ret;

    struct ParamRow {
        SQLINTEGER id;
        SQLLEN idInd;
        SQLCHAR val[51];
        SQLLEN valInd;
    };

    ParamRow rows[BATCH_SIZE] = {};
    rows[0] = {10, 0, "First",  SQL_NTS};
    rows[1] = {20, 0, "Second", SQL_NTS};
    rows[2] = {30, 0, "Third",  SQL_NTS};

    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, (SQLPOINTER)(intptr_t)sizeof(ParamRow), 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)BATCH_SIZE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLPrepare(hStmt, (SQLCHAR*)"INSERT INTO BATCH_TEST (ID, VAL) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
                           0, 0, &rows[0].id, sizeof(rows[0].id), &rows[0].idInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
                           50, 0, rows[0].val, 51, &rows[0].valInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    Commit();
    ReallocStmt();

    // Retrieve and verify
    ExecDirect("SELECT ID, VAL FROM BATCH_TEST ORDER BY ID");
    SQLINTEGER id = 0;
    SQLCHAR val[51] = {};
    SQLLEN idInd = 0, valInd = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, sizeof(id), &idInd);
    SQLBindCol(hStmt, 2, SQL_C_CHAR, val, sizeof(val), &valInd);

    const char* expectedVals[] = {"First", "Second", "Third"};
    const int expectedIds[] = {10, 20, 30};

    for (int i = 0; i < BATCH_SIZE; ++i) {
        ret = SQLFetch(hStmt);
        ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Row " << i << ": " << GetOdbcError(SQL_HANDLE_STMT, hStmt);
        EXPECT_EQ(id, expectedIds[i]);
        EXPECT_STREQ((char*)val, expectedVals[i]);
    }

    ret = SQLFetch(hStmt);
    EXPECT_EQ(ret, SQL_NO_DATA);
}

TEST_F(BatchParamTest, ParamsetSizeThree) {
    // Simple 3-row batch with minimal row-wise binding
    const int BATCH_SIZE = 3;
    SQLRETURN ret;

    struct ParamRow {
        SQLINTEGER id;
        SQLLEN idInd;
        SQLCHAR val[51];
        SQLLEN valInd;
    };

    ParamRow rows[BATCH_SIZE] = {};
    rows[0] = {1, 0, "Row1", SQL_NTS};
    rows[1] = {2, 0, "Row2", SQL_NTS};
    rows[2] = {3, 0, "Row3", SQL_NTS};

    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, (SQLPOINTER)(intptr_t)sizeof(ParamRow), 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)BATCH_SIZE, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLULEN paramsProcessed = 0;
    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &paramsProcessed, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLPrepare(hStmt, (SQLCHAR*)"INSERT INTO BATCH_TEST (ID, VAL) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
                           0, 0, &rows[0].id, sizeof(rows[0].id), &rows[0].idInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
                           50, 0, rows[0].val, 51, &rows[0].valInd);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    EXPECT_EQ(paramsProcessed, (SQLULEN)BATCH_SIZE);

    Commit();
    ReallocStmt();

    // Verify count
    ExecDirect("SELECT COUNT(*) FROM BATCH_TEST");
    SQLINTEGER count = 0;
    SQLLEN countInd = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &count, sizeof(count), &countInd);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(count, BATCH_SIZE);
}

TEST_F(BatchParamTest, ParamsetSizeOne) {
    // PARAMSET_SIZE=1 should work the same as normal execution
    SQLRETURN ret = SQLPrepare(hStmt, (SQLCHAR*)"INSERT INTO BATCH_TEST (ID, VAL) VALUES (?, ?)", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    ret = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(intptr_t)1, 0);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    SQLINTEGER id = 42;
    SQLLEN idInd = 0;
    SQLCHAR val[] = "Single";
    SQLLEN valInd = SQL_NTS;

    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id, 0, &idInd);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 50, 0, val, sizeof(val), &valInd);

    ret = SQLExecute(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    Commit();
    ReallocStmt();

    ExecDirect("SELECT VAL FROM BATCH_TEST WHERE ID = 42");
    SQLCHAR result[51] = {};
    SQLLEN resultInd = 0;
    SQLBindCol(hStmt, 1, SQL_C_CHAR, result, sizeof(result), &resultInd);
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)result, "Single");
}
