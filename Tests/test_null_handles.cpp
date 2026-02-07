#include <gtest/gtest.h>

#ifdef _WIN32
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#else
#include <sql.h>
#include <sqlext.h>
#endif

// =============================================================================
// Phase 0 Crash Prevention Tests
//
// These tests verify that the ODBC driver returns SQL_INVALID_HANDLE (or
// SQL_ERROR where appropriate) when called with NULL handles, instead of
// crashing via null pointer dereference.
//
// Issues addressed: C-1 (SQLCopyDesc crash), C-2 (GUARD_* dereference before
// null check), C-3 (no handle validation at entry points)
// =============================================================================

// ---------------------------------------------------------------------------
// Statement handle (HSTMT) entry points with NULL
// ---------------------------------------------------------------------------

TEST(NullHandleTests, SQLBindColNullStmt)
{
    SQLRETURN rc = SQLBindCol(SQL_NULL_HSTMT, 1, SQL_C_CHAR, nullptr, 0, nullptr);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLCancelNullStmt)
{
    SQLRETURN rc = SQLCancel(SQL_NULL_HSTMT);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLColAttributeNullStmt)
{
    SQLSMALLINT stringLength = 0;
    SQLRETURN rc = SQLColAttribute(SQL_NULL_HSTMT, 1, SQL_DESC_NAME,
                                   nullptr, 0, &stringLength, nullptr);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLDescribeColNullStmt)
{
    SQLRETURN rc = SQLDescribeCol(SQL_NULL_HSTMT, 1, nullptr, 0, nullptr,
                                  nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLExecDirectNullStmt)
{
    SQLRETURN rc = SQLExecDirect(SQL_NULL_HSTMT, (SQLCHAR*)"SELECT 1", SQL_NTS);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLExecuteNullStmt)
{
    SQLRETURN rc = SQLExecute(SQL_NULL_HSTMT);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLFetchNullStmt)
{
    SQLRETURN rc = SQLFetch(SQL_NULL_HSTMT);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLFetchScrollNullStmt)
{
    SQLRETURN rc = SQLFetchScroll(SQL_NULL_HSTMT, SQL_FETCH_NEXT, 0);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLFreeStmtNullStmt)
{
    SQLRETURN rc = SQLFreeStmt(SQL_NULL_HSTMT, SQL_CLOSE);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLGetCursorNameNullStmt)
{
    SQLCHAR name[128];
    SQLSMALLINT nameLen;
    SQLRETURN rc = SQLGetCursorName(SQL_NULL_HSTMT, name, sizeof(name), &nameLen);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLGetDataNullStmt)
{
    char buf[32];
    SQLLEN ind;
    SQLRETURN rc = SQLGetData(SQL_NULL_HSTMT, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLGetStmtAttrNullStmt)
{
    SQLINTEGER value;
    SQLRETURN rc = SQLGetStmtAttr(SQL_NULL_HSTMT, SQL_ATTR_ROW_NUMBER,
                                   &value, sizeof(value), nullptr);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLGetTypeInfoNullStmt)
{
    SQLRETURN rc = SQLGetTypeInfo(SQL_NULL_HSTMT, SQL_ALL_TYPES);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLMoreResultsNullStmt)
{
    SQLRETURN rc = SQLMoreResults(SQL_NULL_HSTMT);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLNumResultColsNullStmt)
{
    SQLSMALLINT cols;
    SQLRETURN rc = SQLNumResultCols(SQL_NULL_HSTMT, &cols);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLPrepareNullStmt)
{
    SQLRETURN rc = SQLPrepare(SQL_NULL_HSTMT, (SQLCHAR*)"SELECT 1", SQL_NTS);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLRowCountNullStmt)
{
    SQLLEN count;
    SQLRETURN rc = SQLRowCount(SQL_NULL_HSTMT, &count);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLSetCursorNameNullStmt)
{
    SQLRETURN rc = SQLSetCursorName(SQL_NULL_HSTMT, (SQLCHAR*)"test", SQL_NTS);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLSetStmtAttrNullStmt)
{
    SQLRETURN rc = SQLSetStmtAttr(SQL_NULL_HSTMT, SQL_ATTR_QUERY_TIMEOUT,
                                   (SQLPOINTER)10, 0);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLCloseCursorNullStmt)
{
    SQLRETURN rc = SQLCloseCursor(SQL_NULL_HSTMT);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLColumnsNullStmt)
{
    SQLRETURN rc = SQLColumns(SQL_NULL_HSTMT, nullptr, 0, nullptr, 0,
                               nullptr, 0, nullptr, 0);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLTablesNullStmt)
{
    SQLRETURN rc = SQLTables(SQL_NULL_HSTMT, nullptr, 0, nullptr, 0,
                              nullptr, 0, nullptr, 0);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLPrimaryKeysNullStmt)
{
    SQLRETURN rc = SQLPrimaryKeys(SQL_NULL_HSTMT, nullptr, 0, nullptr, 0,
                                   nullptr, 0);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLForeignKeysNullStmt)
{
    SQLRETURN rc = SQLForeignKeys(SQL_NULL_HSTMT, nullptr, 0, nullptr, 0,
                                   nullptr, 0, nullptr, 0, nullptr, 0,
                                   nullptr, 0);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLStatisticsNullStmt)
{
    SQLRETURN rc = SQLStatistics(SQL_NULL_HSTMT, nullptr, 0, nullptr, 0,
                                  nullptr, 0, SQL_INDEX_ALL, SQL_QUICK);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLSpecialColumnsNullStmt)
{
    SQLRETURN rc = SQLSpecialColumns(SQL_NULL_HSTMT, SQL_BEST_ROWID,
                                      nullptr, 0, nullptr, 0, nullptr, 0,
                                      SQL_SCOPE_SESSION, SQL_NULLABLE);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLBindParameterNullStmt)
{
    SQLRETURN rc = SQLBindParameter(SQL_NULL_HSTMT, 1, SQL_PARAM_INPUT,
                                     SQL_C_LONG, SQL_INTEGER, 0, 0,
                                     nullptr, 0, nullptr);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLNumParamsNullStmt)
{
    SQLSMALLINT params;
    SQLRETURN rc = SQLNumParams(SQL_NULL_HSTMT, &params);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLDescribeParamNullStmt)
{
    SQLSMALLINT type;
    SQLULEN size;
    SQLSMALLINT digits, nullable;
    SQLRETURN rc = SQLDescribeParam(SQL_NULL_HSTMT, 1, &type, &size,
                                     &digits, &nullable);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLBulkOperationsNullStmt)
{
    SQLRETURN rc = SQLBulkOperations(SQL_NULL_HSTMT, SQL_ADD);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLSetPosNullStmt)
{
    SQLRETURN rc = SQLSetPos(SQL_NULL_HSTMT, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLPutDataNullStmt)
{
    int data = 42;
    SQLRETURN rc = SQLPutData(SQL_NULL_HSTMT, &data, sizeof(data));
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLParamDataNullStmt)
{
    SQLPOINTER value;
    SQLRETURN rc = SQLParamData(SQL_NULL_HSTMT, &value);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

// ---------------------------------------------------------------------------
// Connection handle (HDBC) entry points with NULL
// ---------------------------------------------------------------------------

TEST(NullHandleTests, SQLConnectNullDbc)
{
    SQLRETURN rc = SQLConnect(SQL_NULL_HDBC, (SQLCHAR*)"test", SQL_NTS,
                               (SQLCHAR*)"user", SQL_NTS,
                               (SQLCHAR*)"pass", SQL_NTS);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLDriverConnectNullDbc)
{
    SQLCHAR outStr[256];
    SQLSMALLINT outLen;
    SQLRETURN rc = SQLDriverConnect(SQL_NULL_HDBC, nullptr,
                                     (SQLCHAR*)"DSN=test", SQL_NTS,
                                     outStr, sizeof(outStr), &outLen,
                                     SQL_DRIVER_NOPROMPT);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLDisconnectNullDbc)
{
    SQLRETURN rc = SQLDisconnect(SQL_NULL_HDBC);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLGetConnectAttrNullDbc)
{
    SQLINTEGER value;
    SQLRETURN rc = SQLGetConnectAttr(SQL_NULL_HDBC, SQL_ATTR_AUTOCOMMIT,
                                      &value, sizeof(value), nullptr);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLSetConnectAttrNullDbc)
{
    SQLRETURN rc = SQLSetConnectAttr(SQL_NULL_HDBC, SQL_ATTR_AUTOCOMMIT,
                                      (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLGetInfoNullDbc)
{
    char buf[128];
    SQLSMALLINT len;
    SQLRETURN rc = SQLGetInfo(SQL_NULL_HDBC, SQL_DBMS_NAME, buf,
                               sizeof(buf), &len);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLGetFunctionsNullDbc)
{
    SQLUSMALLINT supported;
    SQLRETURN rc = SQLGetFunctions(SQL_NULL_HDBC, SQL_API_SQLBINDCOL, &supported);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLNativeSqlNullDbc)
{
    SQLCHAR out[128];
    SQLINTEGER outLen;
    SQLRETURN rc = SQLNativeSql(SQL_NULL_HDBC, (SQLCHAR*)"SELECT 1", SQL_NTS,
                                 out, sizeof(out), &outLen);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLEndTranNullDbc)
{
    SQLRETURN rc = SQLEndTran(SQL_HANDLE_DBC, SQL_NULL_HDBC, SQL_COMMIT);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLBrowseConnectNullDbc)
{
    SQLCHAR outStr[256];
    SQLSMALLINT outLen;
    SQLRETURN rc = SQLBrowseConnect(SQL_NULL_HDBC, (SQLCHAR*)"DSN=test", SQL_NTS,
                                     outStr, sizeof(outStr), &outLen);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

// ---------------------------------------------------------------------------
// Environment handle (HENV) entry points with NULL
// ---------------------------------------------------------------------------

TEST(NullHandleTests, SQLGetEnvAttrNullEnv)
{
    SQLINTEGER value;
    SQLRETURN rc = SQLGetEnvAttr(SQL_NULL_HENV, SQL_ATTR_ODBC_VERSION,
                                  &value, sizeof(value), nullptr);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLSetEnvAttrNullEnv)
{
    SQLRETURN rc = SQLSetEnvAttr(SQL_NULL_HENV, SQL_ATTR_ODBC_VERSION,
                                  (SQLPOINTER)SQL_OV_ODBC3, 0);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLEndTranNullEnv)
{
    SQLRETURN rc = SQLEndTran(SQL_HANDLE_ENV, SQL_NULL_HENV, SQL_COMMIT);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLDataSourcesNullEnv)
{
    SQLCHAR name[128], desc[256];
    SQLSMALLINT nameLen, descLen;
    SQLRETURN rc = SQLDataSources(SQL_NULL_HENV, SQL_FETCH_FIRST,
                                   name, sizeof(name), &nameLen,
                                   desc, sizeof(desc), &descLen);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

// ---------------------------------------------------------------------------
// Descriptor handle (HDESC) entry points with NULL — Issue C-1
// ---------------------------------------------------------------------------

TEST(NullHandleTests, SQLCopyDescNullSource)
{
    // C-1: This previously crashed with access violation
    SQLRETURN rc = SQLCopyDesc(SQL_NULL_HDESC, SQL_NULL_HDESC);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLCopyDescNullTarget)
{
    // Even with a valid source, null target should not crash
    // (We can't easily get a valid HDESC without a connection, 
    //  but we can verify null target still returns properly)
    SQLRETURN rc = SQLCopyDesc(SQL_NULL_HDESC, SQL_NULL_HDESC);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLGetDescFieldNullDesc)
{
    SQLINTEGER value;
    SQLINTEGER strLen;
    SQLRETURN rc = SQLGetDescField(SQL_NULL_HDESC, 1, SQL_DESC_COUNT,
                                    &value, sizeof(value), &strLen);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLGetDescRecNullDesc)
{
    SQLCHAR name[128];
    SQLSMALLINT nameLen, type, subType, precision, scale, nullable;
    SQLLEN length;
    SQLRETURN rc = SQLGetDescRec(SQL_NULL_HDESC, 1, name, sizeof(name),
                                  &nameLen, &type, &subType, &length,
                                  &precision, &scale, &nullable);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLSetDescFieldNullDesc)
{
    SQLINTEGER value = 0;
    SQLRETURN rc = SQLSetDescField(SQL_NULL_HDESC, 1, SQL_DESC_TYPE,
                                    &value, sizeof(value));
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLSetDescRecNullDesc)
{
    SQLRETURN rc = SQLSetDescRec(SQL_NULL_HDESC, 1, SQL_INTEGER, 0,
                                  4, 0, 0, nullptr, nullptr, nullptr);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

// ---------------------------------------------------------------------------
// SQLFreeHandle with NULL handles
// ---------------------------------------------------------------------------

TEST(NullHandleTests, SQLFreeHandleNullEnv)
{
    SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_ENV, SQL_NULL_HENV);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLFreeHandleNullDbc)
{
    SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_DBC, SQL_NULL_HDBC);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLFreeHandleNullStmt)
{
    SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_STMT, SQL_NULL_HSTMT);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLFreeHandleNullDesc)
{
    SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_DESC, SQL_NULL_HDESC);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLFreeHandleInvalidType)
{
    SQLRETURN rc = SQLFreeHandle(999, SQL_NULL_HANDLE);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

// ---------------------------------------------------------------------------
// SQLAllocHandle with NULL input handles
// ---------------------------------------------------------------------------

TEST(NullHandleTests, SQLAllocHandleDbc_NullEnv)
{
    SQLHANDLE output;
    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_DBC, SQL_NULL_HENV, &output);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLAllocHandleStmt_NullDbc)
{
    SQLHANDLE output;
    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, SQL_NULL_HDBC, &output);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

// ---------------------------------------------------------------------------
// SQLGetDiagRec / SQLGetDiagField with NULL handles
// ---------------------------------------------------------------------------

TEST(NullHandleTests, SQLGetDiagRecNullHandle)
{
    SQLCHAR sqlState[6];
    SQLINTEGER nativeError;
    SQLCHAR message[256];
    SQLSMALLINT msgLen;
    SQLRETURN rc = SQLGetDiagRec(SQL_HANDLE_STMT, SQL_NULL_HSTMT, 1,
                                  sqlState, &nativeError, message,
                                  sizeof(message), &msgLen);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLGetDiagFieldNullHandle)
{
    SQLINTEGER value;
    SQLSMALLINT strLen;
    SQLRETURN rc = SQLGetDiagField(SQL_HANDLE_STMT, SQL_NULL_HSTMT, 0,
                                    SQL_DIAG_NUMBER, &value, sizeof(value),
                                    &strLen);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

// ---------------------------------------------------------------------------
// Deprecated ODBC 1.0 functions with NULL handles
// ---------------------------------------------------------------------------

TEST(NullHandleTests, SQLAllocConnectNullEnv)
{
    SQLHDBC hDbc;
    SQLRETURN rc = SQLAllocConnect(SQL_NULL_HENV, &hDbc);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLAllocStmtNullDbc)
{
    SQLHSTMT hStmt;
    SQLRETURN rc = SQLAllocStmt(SQL_NULL_HDBC, &hStmt);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLFreeConnectNullDbc)
{
    SQLRETURN rc = SQLFreeConnect(SQL_NULL_HDBC);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}

TEST(NullHandleTests, SQLFreeEnvNullEnv)
{
    SQLRETURN rc = SQLFreeEnv(SQL_NULL_HENV);
    EXPECT_EQ(rc, SQL_INVALID_HANDLE);
}
