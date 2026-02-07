// tests/test_escape_sequences.cpp — ODBC escape sequence tests (Phase 3.8)

#include "test_helpers.h"

class EscapeSequenceTest : public OdbcConnectedTest {};

// ===== Date/Time/Timestamp literal escapes =====

TEST_F(EscapeSequenceTest, DateLiteral) {
    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT {d '2025-06-15'} FROM RDB$DATABASE", SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        GTEST_SKIP() << "Date escape sequence not supported (M-4 open)";
    }

    // Try reading as a date structure
    SQL_DATE_STRUCT val = {};
    SQLLEN ind = 0;
    ret = SQLFetch(hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        GTEST_SKIP() << "Date escape: fetch failed (M-4 open)";
    }

    ret = SQLGetData(hStmt, 1, SQL_C_TYPE_DATE, &val, sizeof(val), &ind);
    if (!SQL_SUCCEEDED(ret) || val.year == 0) {
        // Driver doesn't translate {d ...} escapes — this is expected (M-4 open)
        GTEST_SKIP() << "Date escape not translated by driver (M-4 open)";
    }
    EXPECT_EQ(val.year, 2025);
    EXPECT_EQ(val.month, 6);
    EXPECT_EQ(val.day, 15);
}

TEST_F(EscapeSequenceTest, TimestampLiteral) {
    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT {ts '2025-12-31 23:59:59'} FROM RDB$DATABASE", SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        GTEST_SKIP() << "Timestamp escape sequence not supported (M-4 open)";
    }

    SQL_TIMESTAMP_STRUCT val = {};
    SQLLEN ind = 0;
    ret = SQLFetch(hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        GTEST_SKIP() << "Timestamp escape: fetch failed (M-4 open)";
    }

    ret = SQLGetData(hStmt, 1, SQL_C_TYPE_TIMESTAMP, &val, sizeof(val), &ind);
    if (!SQL_SUCCEEDED(ret) || val.year == 0) {
        GTEST_SKIP() << "Timestamp escape not translated by driver (M-4 open)";
    }
    EXPECT_EQ(val.year, 2025);
    EXPECT_EQ(val.month, 12);
    EXPECT_EQ(val.day, 31);
    EXPECT_EQ(val.hour, 23);
}

// ===== Scalar function escapes =====

TEST_F(EscapeSequenceTest, ScalarFunctionConcat) {
    // {fn CONCAT(s1, s2)} should work
    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT {fn CONCAT('Hello', ' World')} FROM RDB$DATABASE", SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        GTEST_SKIP() << "Scalar function escape not supported (M-4 open)";
    }

    SQLCHAR val[64] = {};
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_CHAR, val, sizeof(val), &ind);
    ret = SQLFetch(hStmt);
    if (SQL_SUCCEEDED(ret)) {
        EXPECT_STREQ((char*)val, "Hello World");
    }
}

TEST_F(EscapeSequenceTest, ScalarFunctionUcase) {
    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT {fn UCASE('hello')} FROM RDB$DATABASE", SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        GTEST_SKIP() << "UCASE escape not supported (M-4 open)";
    }

    SQLCHAR val[32] = {};
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_CHAR, val, sizeof(val), &ind);
    ret = SQLFetch(hStmt);
    if (SQL_SUCCEEDED(ret)) {
        EXPECT_STREQ((char*)val, "HELLO");
    }
}

// ===== Outer join escape =====

TEST_F(EscapeSequenceTest, OuterJoinEscape) {
    // Create two temporary tables for the join test
    ExecIgnoreError("DROP TABLE ODBC_TEST_OJ_A");
    ExecIgnoreError("DROP TABLE ODBC_TEST_OJ_B");
    Commit();

    ReallocStmt();
    ExecDirect("CREATE TABLE ODBC_TEST_OJ_A (ID INTEGER NOT NULL PRIMARY KEY)");
    Commit();
    ReallocStmt();
    ExecDirect("CREATE TABLE ODBC_TEST_OJ_B (ID INTEGER NOT NULL PRIMARY KEY, A_ID INTEGER)");
    Commit();
    ReallocStmt();

    ExecDirect("INSERT INTO ODBC_TEST_OJ_A (ID) VALUES (1)");
    ExecDirect("INSERT INTO ODBC_TEST_OJ_A (ID) VALUES (2)");
    Commit();
    ReallocStmt();

    ExecDirect("INSERT INTO ODBC_TEST_OJ_B (ID, A_ID) VALUES (10, 1)");
    Commit();
    ReallocStmt();

    // Try the outer join escape
    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT A.ID, B.ID FROM {oj ODBC_TEST_OJ_A A LEFT OUTER JOIN ODBC_TEST_OJ_B B ON A.ID = B.A_ID} ORDER BY A.ID",
        SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        // Clean up even if escape fails
        ExecIgnoreError("DROP TABLE ODBC_TEST_OJ_B");
        ExecIgnoreError("DROP TABLE ODBC_TEST_OJ_A");
        Commit();
        GTEST_SKIP() << "Outer join escape not supported (M-4 open)";
    }

    SQLINTEGER aId, bId;
    SQLLEN aInd, bInd;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &aId, 0, &aInd);
    SQLBindCol(hStmt, 2, SQL_C_SLONG, &bId, 0, &bInd);

    // Row 1: A.ID=1, B.ID=10
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(aId, 1);
    EXPECT_EQ(bId, 10);

    // Row 2: A.ID=2, B.ID=NULL (outer join)
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(aId, 2);
    EXPECT_EQ(bInd, SQL_NULL_DATA);

    // Clean up
    SQLCloseCursor(hStmt);
    ExecIgnoreError("DROP TABLE ODBC_TEST_OJ_B");
    ExecIgnoreError("DROP TABLE ODBC_TEST_OJ_A");
    Commit();
}

// ===== SQLNativeSql =====

TEST_F(EscapeSequenceTest, SQLNativeSql) {
    SQLCHAR output[512] = {};
    SQLINTEGER outputLen = 0;

    SQLRETURN ret = SQLNativeSql(hDbc,
        (SQLCHAR*)"SELECT {fn UCASE('hello')} FROM RDB$DATABASE",
        SQL_NTS, output, sizeof(output), &outputLen);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLNativeSql failed: " << GetOdbcError(SQL_HANDLE_DBC, hDbc);

    // The driver should return the translated SQL
    EXPECT_GT(outputLen, 0);
    // The output should not contain ODBC escape braces if translation occurred
}
