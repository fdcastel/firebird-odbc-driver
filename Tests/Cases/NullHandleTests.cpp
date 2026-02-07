#include "pch.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace OdbcTests
{
    // =========================================================================
    // Phase 0 Crash Prevention Tests (MSTest)
    //
    // These tests verify that the ODBC driver returns SQL_INVALID_HANDLE
    // when called with NULL handles, instead of crashing via null pointer
    // dereference.
    //
    // Issues addressed: C-1, C-2, C-3
    // =========================================================================

    TEST_CLASS(NullHandleTests)
    {
    public:

        // -- Statement handle tests --

        TEST_METHOD(SQLExecDirectNullStmt)
        {
            SQLRETURN rc = SQLExecDirect(SQL_NULL_HSTMT, nullptr, SQL_NTS);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLExecDirect should return SQL_INVALID_HANDLE for null stmt");
        }

        TEST_METHOD(SQLExecuteNullStmt)
        {
            SQLRETURN rc = SQLExecute(SQL_NULL_HSTMT);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLExecute should return SQL_INVALID_HANDLE for null stmt");
        }

        TEST_METHOD(SQLFetchNullStmt)
        {
            SQLRETURN rc = SQLFetch(SQL_NULL_HSTMT);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLFetch should return SQL_INVALID_HANDLE for null stmt");
        }

        TEST_METHOD(SQLBindColNullStmt)
        {
            SQLRETURN rc = SQLBindCol(SQL_NULL_HSTMT, 1, SQL_C_CHAR, nullptr, 0, nullptr);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLBindCol should return SQL_INVALID_HANDLE for null stmt");
        }

        TEST_METHOD(SQLCancelNullStmt)
        {
            SQLRETURN rc = SQLCancel(SQL_NULL_HSTMT);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLCancel should return SQL_INVALID_HANDLE for null stmt");
        }

        TEST_METHOD(SQLCloseCursorNullStmt)
        {
            SQLRETURN rc = SQLCloseCursor(SQL_NULL_HSTMT);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLCloseCursor should return SQL_INVALID_HANDLE for null stmt");
        }

        TEST_METHOD(SQLPrepareNullStmt)
        {
            SQLRETURN rc = SQLPrepare(SQL_NULL_HSTMT, nullptr, SQL_NTS);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLPrepare should return SQL_INVALID_HANDLE for null stmt");
        }

        TEST_METHOD(SQLGetStmtAttrNullStmt)
        {
            SQLINTEGER value;
            SQLRETURN rc = SQLGetStmtAttr(SQL_NULL_HSTMT, SQL_ATTR_ROW_NUMBER, &value, sizeof(value), nullptr);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLGetStmtAttr should return SQL_INVALID_HANDLE for null stmt");
        }

        TEST_METHOD(SQLSetStmtAttrNullStmt)
        {
            SQLRETURN rc = SQLSetStmtAttr(SQL_NULL_HSTMT, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)10, 0);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLSetStmtAttr should return SQL_INVALID_HANDLE for null stmt");
        }

        // -- Connection handle tests --

        TEST_METHOD(SQLConnectNullDbc)
        {
            SQLRETURN rc = SQLConnect(SQL_NULL_HDBC, nullptr, SQL_NTS,
                nullptr, SQL_NTS, nullptr, SQL_NTS);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLConnect should return SQL_INVALID_HANDLE for null dbc");
        }

        TEST_METHOD(SQLDisconnectNullDbc)
        {
            SQLRETURN rc = SQLDisconnect(SQL_NULL_HDBC);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLDisconnect should return SQL_INVALID_HANDLE for null dbc");
        }

        TEST_METHOD(SQLGetConnectAttrNullDbc)
        {
            SQLINTEGER value;
            SQLRETURN rc = SQLGetConnectAttr(SQL_NULL_HDBC, SQL_ATTR_AUTOCOMMIT, &value, sizeof(value), nullptr);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLGetConnectAttr should return SQL_INVALID_HANDLE for null dbc");
        }

        TEST_METHOD(SQLSetConnectAttrNullDbc)
        {
            SQLRETURN rc = SQLSetConnectAttr(SQL_NULL_HDBC, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLSetConnectAttr should return SQL_INVALID_HANDLE for null dbc");
        }

        TEST_METHOD(SQLGetInfoNullDbc)
        {
            char buf[128];
            SQLSMALLINT len;
            SQLRETURN rc = SQLGetInfo(SQL_NULL_HDBC, SQL_DBMS_NAME, buf, sizeof(buf), &len);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLGetInfo should return SQL_INVALID_HANDLE for null dbc");
        }

        TEST_METHOD(SQLEndTranNullDbc)
        {
            SQLRETURN rc = SQLEndTran(SQL_HANDLE_DBC, SQL_NULL_HDBC, SQL_COMMIT);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLEndTran should return SQL_INVALID_HANDLE for null dbc");
        }

        // -- Environment handle tests --

        TEST_METHOD(SQLGetEnvAttrNullEnv)
        {
            SQLINTEGER value;
            SQLRETURN rc = SQLGetEnvAttr(SQL_NULL_HENV, SQL_ATTR_ODBC_VERSION, &value, sizeof(value), nullptr);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLGetEnvAttr should return SQL_INVALID_HANDLE for null env");
        }

        TEST_METHOD(SQLSetEnvAttrNullEnv)
        {
            SQLRETURN rc = SQLSetEnvAttr(SQL_NULL_HENV, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLSetEnvAttr should return SQL_INVALID_HANDLE for null env");
        }

        // -- Descriptor handle tests (Issue C-1) --

        TEST_METHOD(SQLCopyDescBothNull)
        {
            // C-1: This previously crashed with access violation
            SQLRETURN rc = SQLCopyDesc(SQL_NULL_HDESC, SQL_NULL_HDESC);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLCopyDesc should return SQL_INVALID_HANDLE for null descs");
        }

        TEST_METHOD(SQLGetDescFieldNullDesc)
        {
            SQLINTEGER value;
            SQLINTEGER strLen;
            SQLRETURN rc = SQLGetDescField(SQL_NULL_HDESC, 1, SQL_DESC_COUNT, &value, sizeof(value), &strLen);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLGetDescField should return SQL_INVALID_HANDLE for null desc");
        }

        TEST_METHOD(SQLSetDescFieldNullDesc)
        {
            SQLINTEGER value = 0;
            SQLRETURN rc = SQLSetDescField(SQL_NULL_HDESC, 1, SQL_DESC_TYPE, &value, sizeof(value));
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLSetDescField should return SQL_INVALID_HANDLE for null desc");
        }

        // -- SQLFreeHandle with NULL --

        TEST_METHOD(SQLFreeHandleNullEnv)
        {
            SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_ENV, SQL_NULL_HENV);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLFreeHandle should return SQL_INVALID_HANDLE for null env");
        }

        TEST_METHOD(SQLFreeHandleNullDbc)
        {
            SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_DBC, SQL_NULL_HDBC);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLFreeHandle should return SQL_INVALID_HANDLE for null dbc");
        }

        TEST_METHOD(SQLFreeHandleNullStmt)
        {
            SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_STMT, SQL_NULL_HSTMT);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLFreeHandle should return SQL_INVALID_HANDLE for null stmt");
        }

        TEST_METHOD(SQLFreeHandleNullDesc)
        {
            SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_DESC, SQL_NULL_HDESC);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLFreeHandle should return SQL_INVALID_HANDLE for null desc");
        }

        // -- SQLAllocHandle with NULL parent --

        TEST_METHOD(SQLAllocHandleDbc_NullEnv)
        {
            SQLHANDLE output;
            SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_DBC, SQL_NULL_HENV, &output);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLAllocHandle DBC should return SQL_INVALID_HANDLE for null env");
        }

        TEST_METHOD(SQLAllocHandleStmt_NullDbc)
        {
            SQLHANDLE output;
            SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, SQL_NULL_HDBC, &output);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLAllocHandle STMT should return SQL_INVALID_HANDLE for null dbc");
        }

        // -- Deprecated functions with NULL --

        TEST_METHOD(SQLFreeConnectNullDbc)
        {
            SQLRETURN rc = SQLFreeConnect(SQL_NULL_HDBC);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLFreeConnect should return SQL_INVALID_HANDLE for null dbc");
        }

        TEST_METHOD(SQLFreeEnvNullEnv)
        {
            SQLRETURN rc = SQLFreeEnv(SQL_NULL_HENV);
            Assert::AreEqual((int)SQL_INVALID_HANDLE, (int)rc, L"SQLFreeEnv should return SQL_INVALID_HANDLE for null env");
        }
    };
}
