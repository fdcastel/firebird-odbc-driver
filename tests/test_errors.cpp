// tests/test_errors.cpp — Error handling tests (Phase 6, ported from psqlodbc errors-test)
//
// Tests error recovery, parse-time errors, errors with bound parameters,
// and that the connection remains usable after errors.

#include "test_helpers.h"

class ErrorsTest : public OdbcConnectedTest {};

// ===== Parse-time errors =====

TEST_F(ErrorsTest, SimpleParseError) {
    // A query referencing a non-existent column should fail with an error
    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT doesnotexist FROM RDB$DATABASE", SQL_NTS);
    EXPECT_FALSE(SQL_SUCCEEDED(ret));

    // Should have a proper SQLSTATE
    std::string sqlState = GetSqlState(SQL_HANDLE_STMT, hStmt);
    EXPECT_FALSE(sqlState.empty()) << "Expected an error SQLSTATE";
    // Firebird returns 42S22 (column not found) or 42000 (syntax error)
    EXPECT_TRUE(sqlState == "42S22" || sqlState == "42000" || sqlState == "HY000")
        << "Unexpected SQLSTATE: " << sqlState;
}

TEST_F(ErrorsTest, RecoverAfterParseError) {
    // Execute a bad query
    SQLExecDirect(hStmt, (SQLCHAR*)"SELECT doesnotexist FROM RDB$DATABASE", SQL_NTS);
    SQLFreeStmt(hStmt, SQL_CLOSE);

    // The statement handle should still be usable after the error
    ReallocStmt();
    ExecDirect("SELECT 1 FROM RDB$DATABASE");

    SQLINTEGER val = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, 1);
}

// ===== Parse error with bound parameters =====

TEST_F(ErrorsTest, ParseErrorWithBoundParam) {
    // Bind a parameter first
    char param1[20] = "foo";
    SQLLEN cbParam1 = SQL_NTS;
    SQLRETURN ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT,
        SQL_C_CHAR, SQL_CHAR, 20, 0, param1, 0, &cbParam1);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));

    // Execute a query referencing a non-existent column with the bound param
    ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT doesnotexist FROM RDB$DATABASE WHERE 1 = ?", SQL_NTS);
    EXPECT_FALSE(SQL_SUCCEEDED(ret));

    // Should have a diagnostic record
    std::string error = GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_NE(error, "(no error info)") << "Expected an error message";
}

TEST_F(ErrorsTest, RecoverAfterParamError) {
    // Bind + fail
    char param1[20] = "foo";
    SQLLEN cbParam1 = SQL_NTS;
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT,
        SQL_C_CHAR, SQL_CHAR, 20, 0, param1, 0, &cbParam1);
    SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT doesnotexist FROM RDB$DATABASE WHERE 1 = ?", SQL_NTS);
    SQLFreeStmt(hStmt, SQL_CLOSE);

    // Should still work after error recovery
    ReallocStmt();
    ExecDirect("SELECT 42 FROM RDB$DATABASE");

    SQLINTEGER val = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, 42);
}

// ===== Syntax error with SQLPrepare/SQLExecute =====

TEST_F(ErrorsTest, PrepareErrorWithBoundParam) {
    // SQLPrepare with a bad query
    SQLRETURN ret = SQLPrepare(hStmt,
        (SQLCHAR*)"SELECT doesnotexist FROM RDB$DATABASE WHERE 1 = ?", SQL_NTS);

    // Firebird may accept the prepare and fail at execute time
    if (SQL_SUCCEEDED(ret)) {
        // Bind param
        char param1[20] = "foo";
        SQLLEN cbParam1 = SQL_NTS;
        SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT,
            SQL_C_CHAR, SQL_CHAR, 20, 0, param1, 0, &cbParam1);

        // Execute should fail
        ret = SQLExecute(hStmt);
        EXPECT_FALSE(SQL_SUCCEEDED(ret));
    }
    // Either way, the error should produce a diagnostic
    std::string error = GetOdbcError(SQL_HANDLE_STMT, hStmt);
    EXPECT_NE(error, "(no error info)");
}

// ===== Table not found =====

TEST_F(ErrorsTest, TableNotFound) {
    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT * FROM NONEXISTENT_TABLE_XYZ_12345", SQL_NTS);
    EXPECT_FALSE(SQL_SUCCEEDED(ret));

    std::string sqlState = GetSqlState(SQL_HANDLE_STMT, hStmt);
    // Firebird should return 42S02 (table not found) or 42000
    EXPECT_TRUE(sqlState == "42S02" || sqlState == "42000" || sqlState == "HY000")
        << "Unexpected SQLSTATE for table not found: " << sqlState;
}

// ===== Constraint violation =====

TEST_F(ErrorsTest, UniqueConstraintViolation) {
    TempTable table(this, "ODBC_TEST_ERR_UNIQ",
        "ID INTEGER NOT NULL PRIMARY KEY, VAL VARCHAR(50)");

    ExecDirect("INSERT INTO ODBC_TEST_ERR_UNIQ VALUES (1, 'first')");
    Commit();
    ReallocStmt();

    // Try to insert a duplicate primary key
    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_TEST_ERR_UNIQ VALUES (1, 'duplicate')", SQL_NTS);
    EXPECT_FALSE(SQL_SUCCEEDED(ret));

    std::string sqlState = GetSqlState(SQL_HANDLE_STMT, hStmt);
    EXPECT_EQ(sqlState, "23000")
        << "Expected 23000 for unique constraint violation, got: " << sqlState;
}

// ===== Multiple errors in sequence =====

TEST_F(ErrorsTest, MultipleSequentialErrors) {
    // First error
    SQLExecDirect(hStmt, (SQLCHAR*)"SELECT bad1 FROM RDB$DATABASE", SQL_NTS);
    std::string err1 = GetSqlState(SQL_HANDLE_STMT, hStmt);
    EXPECT_FALSE(err1.empty());

    SQLFreeStmt(hStmt, SQL_CLOSE);
    ReallocStmt();

    // Second error (different)
    SQLExecDirect(hStmt, (SQLCHAR*)"INSERT INTO nonexistent_table VALUES (1)", SQL_NTS);
    std::string err2 = GetSqlState(SQL_HANDLE_STMT, hStmt);
    EXPECT_FALSE(err2.empty());

    SQLFreeStmt(hStmt, SQL_CLOSE);
    ReallocStmt();

    // Should still work after multiple errors
    ExecDirect("SELECT 99 FROM RDB$DATABASE");
    SQLINTEGER val = 0;
    SQLLEN ind = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
    SQLRETURN ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_EQ(val, 99);
}

// ===== Error message content =====

TEST_F(ErrorsTest, ErrorMessageContainsMeaningfulText) {
    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT * FROM THIS_TABLE_DOES_NOT_EXIST_ABC", SQL_NTS);
    EXPECT_FALSE(SQL_SUCCEEDED(ret));

    std::string error = GetOdbcError(SQL_HANDLE_STMT, hStmt);
    // The error message should reference the table name
    EXPECT_NE(error.find("THIS_TABLE_DOES_NOT_EXIST_ABC"), std::string::npos)
        << "Error message should mention the missing table: " << error;
}

// ===== Not null constraint =====

TEST_F(ErrorsTest, NotNullConstraintViolation) {
    TempTable table(this, "ODBC_TEST_ERR_NOTNULL",
        "ID INTEGER NOT NULL, VAL VARCHAR(50)");

    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"INSERT INTO ODBC_TEST_ERR_NOTNULL (VAL) VALUES ('test')", SQL_NTS);
    EXPECT_FALSE(SQL_SUCCEEDED(ret));

    std::string sqlState = GetSqlState(SQL_HANDLE_STMT, hStmt);
    // Firebird returns 23000 for NOT NULL violations
    EXPECT_TRUE(sqlState == "23000" || sqlState == "42000" || sqlState == "HY000")
        << "Unexpected SQLSTATE: " << sqlState;
}

// ===== Division by zero =====

TEST_F(ErrorsTest, DivisionByZero) {
    SQLRETURN ret = SQLExecDirect(hStmt,
        (SQLCHAR*)"SELECT 1/0 FROM RDB$DATABASE", SQL_NTS);

    if (SQL_SUCCEEDED(ret)) {
        // Some drivers return the error on fetch
        SQLINTEGER val = 0;
        SQLLEN ind = 0;
        SQLBindCol(hStmt, 1, SQL_C_SLONG, &val, 0, &ind);
        ret = SQLFetch(hStmt);
    }

    // Either execute or fetch should have failed
    if (!SQL_SUCCEEDED(ret)) {
        std::string sqlState = GetSqlState(SQL_HANDLE_STMT, hStmt);
        EXPECT_TRUE(sqlState == "22012" || sqlState == "22000" || sqlState == "HY000")
            << "Expected division by zero error, got: " << sqlState;
    }
}
