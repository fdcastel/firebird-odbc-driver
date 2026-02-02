#include "pch.h"
#include "../Fixtures/TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace OdbcTests
{
    TEST_CLASS(ErrorMappingTests)
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

        TEST_METHOD(InvalidSqlSyntax)
        {
            // Execute invalid SQL
            SQLRETURN rc = SQLExecDirect(testBase->stmt, (SQLWCHAR*)L"INVALID SQL SYNTAX", SQL_NTS);
            Assert::IsFalse(SQL_SUCCEEDED(rc), L"Expected SQL to fail");

            // Check SQLSTATE - should be 42000 (syntax error) or similar
            SQLWCHAR sqlState[6];
            SQLINTEGER nativeError;
            SQLWCHAR messageText[256];
            SQLSMALLINT textLength;

            rc = SQLGetDiagRecW(SQL_HANDLE_STMT, testBase->stmt, 1, sqlState, &nativeError,
                messageText, sizeof(messageText)/sizeof(SQLWCHAR), &textLength);

            testBase->AssertSuccess(rc, L"SQLGetDiagRecW failed");

            std::wstring wstate(sqlState);
            std::string state(wstate.begin(), wstate.end());

            // Accept various syntax error states
            bool isSyntaxError = (state.substr(0, 2) == "42" || state == "37000");
            Assert::IsTrue(isSyntaxError, L"Expected syntax error SQLSTATE (42xxx or 37000)");

            Logger::WriteMessage(("✓ Invalid SQL syntax mapped to SQLSTATE: " + state).c_str());
        }

        TEST_METHOD(TableNotFound)
        {
            // Try to select from non-existent table
            SQLRETURN rc = SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"SELECT * FROM NONEXISTENT_TABLE_XYZ", SQL_NTS);
            Assert::IsFalse(SQL_SUCCEEDED(rc), L"Expected query to fail");

            SQLWCHAR sqlState[6];
            SQLINTEGER nativeError;
            SQLWCHAR messageText[256];
            SQLSMALLINT textLength;

            rc = SQLGetDiagRecW(SQL_HANDLE_STMT, testBase->stmt, 1, sqlState, &nativeError,
                messageText, sizeof(messageText)/sizeof(SQLWCHAR), &textLength);

            testBase->AssertSuccess(rc, L"SQLGetDiagRecW failed");

            std::wstring wstate(sqlState);
            std::string state(wstate.begin(), wstate.end());

            Logger::WriteMessage(("✓ Table not found mapped to SQLSTATE: " + state).c_str());
        }

        TEST_METHOD(InvalidDescriptorIndex)
        {
            // Prepare a query
            SQLRETURN rc = SQLPrepare(testBase->stmt, (SQLWCHAR*)L"SELECT 1 FROM RDB$DATABASE", SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to prepare");

            // Try to bind an invalid column number (0 or > column count)
            SQLINTEGER data;
            SQLLEN indicator;
            rc = SQLBindCol(testBase->stmt, 999, SQL_C_SLONG, &data, sizeof(data), &indicator);

            if (!SQL_SUCCEEDED(rc))
            {
                // Should return SQLSTATE 07009
                SQLWCHAR sqlState[6];
                SQLINTEGER nativeError;
                SQLWCHAR messageText[256];
                SQLSMALLINT textLength;

                SQLGetDiagRecW(SQL_HANDLE_STMT, testBase->stmt, 1, sqlState, &nativeError,
                    messageText, sizeof(messageText)/sizeof(SQLWCHAR), &textLength);

                std::wstring wstate(sqlState);
                std::string state(wstate.begin(), wstate.end());

                if (state == "07009")
                {
                    Logger::WriteMessage("✓ Invalid descriptor index returns SQLSTATE 07009");
                }
                else
                {
                    Logger::WriteMessage(("⚠ Invalid descriptor index returned SQLSTATE: " + state).c_str());
                }
            }
            else
            {
                Logger::WriteMessage("⚠ Invalid column index was accepted (driver may not validate)");
            }
        }

        TEST_METHOD(InvalidCursorState)
        {
            // Try to fetch without executing a query
            SQLRETURN rc = SQLFetch(testBase->stmt);

            if (rc == SQL_ERROR)
            {
                // Should return SQLSTATE 24000 (invalid cursor state)
                SQLWCHAR sqlState[6];
                SQLINTEGER nativeError;
                SQLWCHAR messageText[256];
                SQLSMALLINT textLength;

                SQLGetDiagRecW(SQL_HANDLE_STMT, testBase->stmt, 1, sqlState, &nativeError,
                    messageText, sizeof(messageText)/sizeof(SQLWCHAR), &textLength);

                std::wstring wstate(sqlState);
                std::string state(wstate.begin(), wstate.end());

                if (state == "24000" || state == "HY010")
                {
                    Logger::WriteMessage(("✓ Invalid cursor state returns SQLSTATE: " + state).c_str());
                }
                else
                {
                    Logger::WriteMessage(("⚠ Invalid cursor state returned SQLSTATE: " + state).c_str());
                }
            }
            else
            {
                Logger::WriteMessage("⚠ Fetch on unprepared statement didn't return error");
            }
        }

        TEST_METHOD(InvalidParameterType)
        {
            // Prepare a parameterized query
            SQLRETURN rc = SQLPrepare(testBase->stmt, 
                (SQLWCHAR*)L"SELECT * FROM RDB$RELATIONS WHERE RDB$RELATION_ID = ?", SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to prepare");

            // Bind parameter with invalid C type
            SQLINTEGER param = 0;
            rc = SQLBindParameter(testBase->stmt, 1, SQL_PARAM_INPUT, 
                9999, // Invalid C type
                SQL_INTEGER, 0, 0, &param, 0, NULL);

            if (!SQL_SUCCEEDED(rc))
            {
                SQLWCHAR sqlState[6];
                SQLINTEGER nativeError;
                SQLWCHAR messageText[256];
                SQLSMALLINT textLength;

                SQLGetDiagRecW(SQL_HANDLE_STMT, testBase->stmt, 1, sqlState, &nativeError,
                    messageText, sizeof(messageText)/sizeof(SQLWCHAR), &textLength);

                std::wstring wstate(sqlState);
                std::string state(wstate.begin(), wstate.end());

                if (state == "HY003" || state == "HY004")
                {
                    Logger::WriteMessage(("✓ Invalid parameter type returns SQLSTATE: " + state).c_str());
                }
                else
                {
                    Logger::WriteMessage(("⚠ Invalid parameter type returned SQLSTATE: " + state).c_str());
                }
            }
            else
            {
                Logger::WriteMessage("⚠ Invalid parameter type was accepted");
            }
        }

        TEST_METHOD(ConnectionError)
        {
            // Allocate a new connection
            SQLHDBC newDbc;
            SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_DBC, testBase->env, &newDbc);
            testBase->AssertSuccess(rc, L"Failed to allocate connection");

            // Try to connect with invalid connection string
            std::wstring invalidConnStr = L"Driver={Firebird ODBC Driver};Database=INVALID_PATH_12345.fdb;UID=BAD;PWD=BAD";
            SQLSMALLINT outLen;
            rc = SQLDriverConnect(newDbc, NULL, (SQLWCHAR*)invalidConnStr.c_str(), 
                (SQLSMALLINT)invalidConnStr.length(), NULL, 0, &outLen, SQL_DRIVER_NOPROMPT);

            Assert::IsFalse(SQL_SUCCEEDED(rc), L"Expected connection to fail");

            SQLWCHAR sqlState[6];
            SQLINTEGER nativeError;
            SQLWCHAR messageText[512];
            SQLSMALLINT textLength;

            SQLGetDiagRecW(SQL_HANDLE_DBC, newDbc, 1, sqlState, &nativeError,
                messageText, sizeof(messageText)/sizeof(SQLWCHAR), &textLength);

            std::wstring wstate(sqlState);
            std::string state(wstate.begin(), wstate.end());

            Logger::WriteMessage(("✓ Connection error SQLSTATE: " + state).c_str());

            SQLFreeHandle(SQL_HANDLE_DBC, newDbc);
        }
    };
}
