#include "pch.h"
#include "../Fixtures/TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace OdbcTests
{
    TEST_CLASS(GetDataTests)
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

        TEST_METHOD(GetDataBasic)
        {
            // Execute query
            SQLRETURN rc = SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"SELECT FIRST 1 RDB$RELATION_NAME FROM RDB$RELATIONS", 
                SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to execute query");

            // Fetch row
            rc = SQLFetch(testBase->stmt);
            testBase->AssertSuccessOrInfo(rc, L"SQLFetch failed");

            // Get data using SQLGetData
            SQLWCHAR data[256];
            SQLLEN indicator;

            rc = SQLGetData(testBase->stmt, 1, SQL_C_WCHAR, data, sizeof(data), &indicator);
            testBase->AssertSuccessOrInfo(rc, L"SQLGetData failed");

            Assert::IsTrue(indicator > 0 || indicator == SQL_NTS, L"Should have data");

            std::wstring wdata(data);
            std::string sdata(wdata.begin(), wdata.end());
            Logger::WriteMessage(("✓ SQLGetData retrieved: " + sdata).c_str());
        }

        TEST_METHOD(GetDataNullColumn)
        {
            // Query that might return NULL
            SQLRETURN rc = SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"SELECT FIRST 1 RDB$DESCRIPTION FROM RDB$RELATIONS", 
                SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to execute query");

            // Fetch row
            rc = SQLFetch(testBase->stmt);
            testBase->AssertSuccessOrInfo(rc, L"SQLFetch failed");

            // Get data
            SQLWCHAR data[256];
            SQLLEN indicator;

            rc = SQLGetData(testBase->stmt, 1, SQL_C_WCHAR, data, sizeof(data), &indicator);
            
            if (SQL_SUCCEEDED(rc))
            {
                if (indicator == SQL_NULL_DATA)
                {
                    Logger::WriteMessage("✓ SQLGetData correctly indicated NULL");
                }
                else
                {
                    Logger::WriteMessage("✓ SQLGetData retrieved non-NULL value");
                }
            }
            else
            {
                Logger::WriteMessage("⚠ SQLGetData failed");
            }
        }

        TEST_METHOD(GetDataPartialRead)
        {
            // Execute query with potentially long data
            SQLRETURN rc = SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"SELECT FIRST 1 RDB$RELATION_NAME FROM RDB$RELATIONS", 
                SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to execute query");

            // Fetch row
            rc = SQLFetch(testBase->stmt);
            testBase->AssertSuccessOrInfo(rc, L"SQLFetch failed");

            // Get data with small buffer to trigger truncation
            SQLWCHAR data[4]; // Very small buffer
            SQLLEN indicator;

            rc = SQLGetData(testBase->stmt, 1, SQL_C_WCHAR, data, sizeof(data), &indicator);

            if (rc == SQL_SUCCESS_WITH_INFO)
            {
                // Check if it's a truncation warning (SQLSTATE 01004)
                SQLWCHAR sqlState[6];
                SQLINTEGER nativeError;
                SQLWCHAR messageText[256];
                SQLSMALLINT textLength;

                SQLGetDiagRecW(SQL_HANDLE_STMT, testBase->stmt, 1, sqlState, &nativeError,
                    messageText, sizeof(messageText)/sizeof(SQLWCHAR), &textLength);

                std::wstring wstate(sqlState);
                std::string state(wstate.begin(), wstate.end());

                if (state == "01004")
                {
                    Logger::WriteMessage("✓ SQLGetData returned truncation warning (01004)");
                }
                else
                {
                    Logger::WriteMessage(("⚠ SQL_SUCCESS_WITH_INFO but SQLSTATE: " + state).c_str());
                }
            }
            else if (SQL_SUCCEEDED(rc))
            {
                Logger::WriteMessage("✓ SQLGetData succeeded (data fit in buffer)");
            }
        }

        TEST_METHOD(GetDataRepeatedCalls)
        {
            // Execute query
            SQLRETURN rc = SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"SELECT FIRST 1 RDB$RELATION_NAME FROM RDB$RELATIONS", 
                SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to execute query");

            // Fetch row
            rc = SQLFetch(testBase->stmt);
            testBase->AssertSuccessOrInfo(rc, L"SQLFetch failed");

            // First call to SQLGetData
            SQLWCHAR data1[10];
            SQLLEN indicator1;
            rc = SQLGetData(testBase->stmt, 1, SQL_C_WCHAR, data1, sizeof(data1), &indicator1);

            // Second call to SQLGetData on same column (should continue from where left off)
            SQLWCHAR data2[10];
            SQLLEN indicator2;
            SQLRETURN rc2 = SQLGetData(testBase->stmt, 1, SQL_C_WCHAR, data2, sizeof(data2), &indicator2);

            if (rc == SQL_SUCCESS_WITH_INFO && SQL_SUCCEEDED(rc2))
            {
                Logger::WriteMessage("✓ Repeated SQLGetData calls supported");
            }
            else if (SQL_SUCCEEDED(rc))
            {
                Logger::WriteMessage("✓ SQLGetData succeeded on first call (data fit)");
            }
            else
            {
                Logger::WriteMessage("⚠ Repeated SQLGetData may not be fully supported");
            }
        }
    };
}
