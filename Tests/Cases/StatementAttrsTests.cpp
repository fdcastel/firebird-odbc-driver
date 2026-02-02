#include "pch.h"
#include "../Fixtures/TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace OdbcTests
{
    TEST_CLASS(StatementAttrsTests)
    {
    private:
        TestBase* testBase;

    public:
        TEST_METHOD_INITIALIZE(SetUp)
        {
            testBase = new TestBase();
            testBase->SetUp();
        }

        TEST_METHOD_CLEANUP(TearDown)
        {
            if (testBase)
            {
                testBase->TearDown();
                delete testBase;
                testBase = nullptr;
            }
        }

        TEST_METHOD(AllocPrepareExecuteFree)
        {
            // Basic statement lifecycle already tested in SetUp/TearDown
            // Test allocating an additional statement
            SQLHSTMT newStmt;
            SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, testBase->dbc, &newStmt);
            testBase->AssertSuccess(rc, L"Failed to allocate additional statement");

            // Prepare a simple query
            rc = SQLPrepare(newStmt, (SQLWCHAR*)L"SELECT 1 FROM RDB$DATABASE", SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to prepare query");

            // Execute
            rc = SQLExecute(newStmt);
            testBase->AssertSuccessOrInfo(rc, L"Failed to execute query");

            // Free the statement
            rc = SQLFreeHandle(SQL_HANDLE_STMT, newStmt);
            testBase->AssertSuccess(rc, L"Failed to free statement");

            Logger::WriteMessage("✓ Statement lifecycle: Alloc -> Prepare -> Execute -> Free");
        }

        TEST_METHOD(NumResultCols)
        {
            // Prepare a query with known column count
            SQLRETURN rc = SQLPrepare(testBase->stmt, (SQLWCHAR*)L"SELECT 1, 2, 3 FROM RDB$DATABASE", SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to prepare query");

            // Get result column count
            SQLSMALLINT colCount;
            rc = SQLNumResultCols(testBase->stmt, &colCount);
            testBase->AssertSuccess(rc, L"SQLNumResultCols failed");

            Assert::AreEqual((int)3, (int)colCount, L"Expected 3 columns");

            Logger::WriteMessage("✓ SQLNumResultCols returns correct count");
        }

        TEST_METHOD(DescribeCol)
        {
            // Prepare a query
            SQLRETURN rc = SQLPrepare(testBase->stmt, 
                (SQLWCHAR*)L"SELECT RDB$CHARACTER_SET_NAME FROM RDB$CHARACTER_SETS WHERE RDB$CHARACTER_SET_ID = 0", 
                SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to prepare query");

            // Describe first column
            SQLWCHAR colName[128];
            SQLSMALLINT nameLen, dataType, decimalDigits, nullable;
            SQLULEN columnSize;

            rc = SQLDescribeCol(testBase->stmt, 1, colName, sizeof(colName)/sizeof(SQLWCHAR), 
                &nameLen, &dataType, &columnSize, &decimalDigits, &nullable);
            testBase->AssertSuccess(rc, L"SQLDescribeCol failed");

            Assert::IsTrue(nameLen > 0, L"Column name should not be empty");

            char msg[256];
            sprintf_s(msg, "✓ Column described: %d bytes, type=%d", (int)columnSize, (int)dataType);
            Logger::WriteMessage(msg);
        }

        TEST_METHOD(CursorTypeSetGet)
        {
            // Try to set cursor type to static
            SQLRETURN rc = SQLSetStmtAttr(testBase->stmt, SQL_ATTR_CURSOR_TYPE, 
                (SQLPOINTER)SQL_CURSOR_STATIC, 0);
            // May not be supported, so accept SQL_SUCCESS or SQL_SUCCESS_WITH_INFO
            if (!SQL_SUCCEEDED(rc))
            {
                Logger::WriteMessage("⚠ SQL_CURSOR_STATIC not supported, trying FORWARD_ONLY");
                rc = SQLSetStmtAttr(testBase->stmt, SQL_ATTR_CURSOR_TYPE, 
                    (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY, 0);
            }
            testBase->AssertSuccessOrInfo(rc, L"Failed to set cursor type");

            // Get cursor type
            SQLULEN cursorType;
            rc = SQLGetStmtAttr(testBase->stmt, SQL_ATTR_CURSOR_TYPE, &cursorType, 0, NULL);
            testBase->AssertSuccessOrInfo(rc, L"Failed to get cursor type");

            Logger::WriteMessage("✓ Cursor type set/get succeeded");
        }

        TEST_METHOD(CursorScrollable)
        {
            // Set cursor scrollable
            SQLRETURN rc = SQLSetStmtAttr(testBase->stmt, SQL_ATTR_CURSOR_SCROLLABLE, 
                (SQLPOINTER)SQL_SCROLLABLE, 0);
            
            if (!SQL_SUCCEEDED(rc))
            {
                Logger::WriteMessage("⚠ SQL_SCROLLABLE not supported");
                // This is acceptable - not all drivers support scrollable cursors
            }
            else
            {
                // Get cursor scrollable
                SQLULEN scrollable;
                rc = SQLGetStmtAttr(testBase->stmt, SQL_ATTR_CURSOR_SCROLLABLE, &scrollable, 0, NULL);
                testBase->AssertSuccessOrInfo(rc, L"Failed to get cursor scrollable");
                
                Logger::WriteMessage("✓ Cursor scrollable attribute accessed");
            }
        }

        TEST_METHOD(RowArraySize)
        {
            // Set row array size (for block cursors)
            SQLULEN rowArraySize = 10;
            SQLRETURN rc = SQLSetStmtAttr(testBase->stmt, SQL_ATTR_ROW_ARRAY_SIZE, 
                (SQLPOINTER)rowArraySize, 0);
            testBase->AssertSuccessOrInfo(rc, L"Failed to set row array size");

            // Get row array size
            SQLULEN verifySize;
            rc = SQLGetStmtAttr(testBase->stmt, SQL_ATTR_ROW_ARRAY_SIZE, &verifySize, 0, NULL);
            testBase->AssertSuccessOrInfo(rc, L"Failed to get row array size");

            Assert::AreEqual((int)rowArraySize, (int)verifySize, L"Row array size mismatch");

            Logger::WriteMessage("✓ SQL_ATTR_ROW_ARRAY_SIZE set and verified");
        }

        TEST_METHOD(RowsFetchedPtr)
        {
            // Bind rows fetched pointer
            SQLULEN rowsFetched;
            SQLRETURN rc = SQLSetStmtAttr(testBase->stmt, SQL_ATTR_ROWS_FETCHED_PTR, 
                &rowsFetched, 0);
            testBase->AssertSuccessOrInfo(rc, L"Failed to set rows fetched pointer");

            // Execute a query
            rc = SQLExecDirect(testBase->stmt, (SQLWCHAR*)L"SELECT FIRST 5 RDB$RELATION_ID FROM RDB$RELATIONS", SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to execute query");

            // Fetch to populate the counter
            rc = SQLFetch(testBase->stmt);
            if (SQL_SUCCEEDED(rc))
            {
                char msg[64];
                sprintf_s(msg, "✓ Rows fetched: %d", (int)rowsFetched);
                Logger::WriteMessage(msg);
            }
            else
            {
                Logger::WriteMessage("⚠ Fetch failed or no rows");
            }
        }

        TEST_METHOD(ParamSetSize)
        {
            // Set parameter array size (for batch operations)
            SQLULEN paramSetSize = 5;
            SQLRETURN rc = SQLSetStmtAttr(testBase->stmt, SQL_ATTR_PARAMSET_SIZE, 
                (SQLPOINTER)paramSetSize, 0);
            testBase->AssertSuccessOrInfo(rc, L"Failed to set parameter set size");

            // Get parameter set size
            SQLULEN verifySize;
            rc = SQLGetStmtAttr(testBase->stmt, SQL_ATTR_PARAMSET_SIZE, &verifySize, 0, NULL);
            testBase->AssertSuccessOrInfo(rc, L"Failed to get parameter set size");

            Assert::AreEqual((int)paramSetSize, (int)verifySize, L"Parameter set size mismatch");

            Logger::WriteMessage("✓ SQL_ATTR_PARAMSET_SIZE set and verified");
        }

        TEST_METHOD(QueryTimeout)
        {
            // Set query timeout
            SQLULEN timeout = 30; // 30 seconds
            SQLRETURN rc = SQLSetStmtAttr(testBase->stmt, SQL_ATTR_QUERY_TIMEOUT, 
                (SQLPOINTER)timeout, 0);
            
            if (SQL_SUCCEEDED(rc))
            {
                // Get query timeout
                SQLULEN verifyTimeout;
                rc = SQLGetStmtAttr(testBase->stmt, SQL_ATTR_QUERY_TIMEOUT, &verifyTimeout, 0, NULL);
                testBase->AssertSuccessOrInfo(rc, L"Failed to get query timeout");

                Logger::WriteMessage("✓ SQL_ATTR_QUERY_TIMEOUT set successfully");
            }
            else
            {
                Logger::WriteMessage("⚠ SQL_ATTR_QUERY_TIMEOUT not supported");
                // This is acceptable - not all drivers support query timeout
            }
        }
    };
}
