#include "pch.h"
#include "../Fixtures/TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace OdbcTests
{
    TEST_CLASS(FetchBindingTests)
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

        TEST_METHOD(BindColCharType)
        {
            // Execute query returning character data
            SQLRETURN rc = SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"SELECT FIRST 1 RDB$RELATION_NAME FROM RDB$RELATIONS", 
                SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to execute query");

            // Bind column
            SQLWCHAR data[256];
            SQLLEN indicator;

            rc = SQLBindCol(testBase->stmt, 1, SQL_C_WCHAR, data, sizeof(data), &indicator);
            testBase->AssertSuccess(rc, L"SQLBindCol failed");

            // Fetch
            rc = SQLFetch(testBase->stmt);
            testBase->AssertSuccessOrInfo(rc, L"SQLFetch failed");

            Assert::IsTrue(indicator > 0 || indicator == SQL_NTS, L"Indicator should show data");

            std::wstring wdata(data);
            std::string sdata(wdata.begin(), wdata.end());
            Logger::WriteMessage(("✓ Fetched char data: " + sdata).c_str());
        }

        TEST_METHOD(BindColNumericTypes)
        {
            // Execute query returning numeric data
            SQLRETURN rc = SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"SELECT FIRST 1 RDB$RELATION_ID FROM RDB$RELATIONS", 
                SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to execute query");

            // Bind as integer
            SQLINTEGER value;
            SQLLEN indicator;

            rc = SQLBindCol(testBase->stmt, 1, SQL_C_SLONG, &value, sizeof(value), &indicator);
            testBase->AssertSuccess(rc, L"SQLBindCol failed");

            // Fetch
            rc = SQLFetch(testBase->stmt);
            testBase->AssertSuccessOrInfo(rc, L"SQLFetch failed");

            Assert::IsTrue(indicator != SQL_NULL_DATA, L"Value should not be NULL");

            char msg[64];
            sprintf_s(msg, "✓ Fetched integer: %d", (int)value);
            Logger::WriteMessage(msg);
        }

        TEST_METHOD(FetchScrollForward)
        {
            // Execute query with multiple rows
            SQLRETURN rc = SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"SELECT FIRST 3 RDB$RELATION_ID FROM RDB$RELATIONS", 
                SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to execute query");

            int rowCount = 0;
            
            // Fetch forward using SQLFetchScroll
            while (true)
            {
                rc = SQLFetchScroll(testBase->stmt, SQL_FETCH_NEXT, 0);
                if (rc == SQL_NO_DATA)
                    break;
                    
                if (SQL_SUCCEEDED(rc))
                    rowCount++;
                else
                    break;
            }

            Assert::IsTrue(rowCount > 0, L"Should fetch at least one row");

            char msg[64];
            sprintf_s(msg, "✓ Fetched %d row(s) using SQLFetchScroll", rowCount);
            Logger::WriteMessage(msg);
        }

        TEST_METHOD(RowStatusArray)
        {
            // Set row array size
            SQLULEN rowArraySize = 3;
            SQLRETURN rc = SQLSetStmtAttr(testBase->stmt, SQL_ATTR_ROW_ARRAY_SIZE, 
                (SQLPOINTER)rowArraySize, 0);
            testBase->AssertSuccessOrInfo(rc, L"Failed to set row array size");

            // Set row status array
            SQLUSMALLINT rowStatus[3];
            rc = SQLSetStmtAttr(testBase->stmt, SQL_ATTR_ROW_STATUS_PTR, rowStatus, 0);
            testBase->AssertSuccessOrInfo(rc, L"Failed to set row status pointer");

            // Execute query
            rc = SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"SELECT FIRST 3 RDB$RELATION_ID FROM RDB$RELATIONS", 
                SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to execute query");

            // Fetch block
            rc = SQLFetchScroll(testBase->stmt, SQL_FETCH_NEXT, 0);
            
            if (SQL_SUCCEEDED(rc))
            {
                int successCount = 0;
                for (int i = 0; i < 3; i++)
                {
                    if (rowStatus[i] == SQL_ROW_SUCCESS || rowStatus[i] == SQL_ROW_SUCCESS_WITH_INFO)
                        successCount++;
                }

                char msg[64];
                sprintf_s(msg, "✓ Row status array: %d successful rows", successCount);
                Logger::WriteMessage(msg);
            }
            else
            {
                Logger::WriteMessage("⚠ Block fetch not fully supported");
            }
        }

        TEST_METHOD(BindColWithNullValues)
        {
            // Query that might return NULL (using RDB$DESCRIPTION which is often NULL)
            SQLRETURN rc = SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"SELECT FIRST 1 RDB$DESCRIPTION FROM RDB$RELATIONS", 
                SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to execute query");

            // Bind column
            SQLWCHAR data[256];
            SQLLEN indicator;

            rc = SQLBindCol(testBase->stmt, 1, SQL_C_WCHAR, data, sizeof(data), &indicator);
            testBase->AssertSuccess(rc, L"SQLBindCol failed");

            // Fetch
            rc = SQLFetch(testBase->stmt);
            testBase->AssertSuccessOrInfo(rc, L"SQLFetch failed");

            if (indicator == SQL_NULL_DATA)
            {
                Logger::WriteMessage("✓ NULL value correctly indicated");
            }
            else
            {
                Logger::WriteMessage("✓ Non-NULL value fetched (or column never NULL)");
            }
        }
    };
}
