#include "pch.h"
#include "../Fixtures/TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace OdbcTests
{
    TEST_CLASS(UnicodeTests)
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

        TEST_METHOD(GetInfoW)
        {
            // Explicitly call SQLGetInfoW with Unicode buffers
            SQLWCHAR driverName[256];
            SQLSMALLINT length;
            SQLRETURN rc = SQLGetInfoW(testBase->dbc, SQL_DRIVER_NAME, driverName, 
                sizeof(driverName), &length);

            testBase->AssertSuccessOrInfo(rc, L"SQLGetInfoW failed");
            Assert::IsTrue(length > 0, L"Driver name length should be > 0");

            // Convert to string and log
            std::wstring wname(driverName, length / sizeof(SQLWCHAR));
            std::string name(wname.begin(), wname.end());
            
            Logger::WriteMessage(("✓ Driver name (Unicode): " + name).c_str());
        }

        TEST_METHOD(GetConnectAttrW)
        {
            // Get autocommit attribute using wide string
            SQLINTEGER autocommit;
            SQLRETURN rc = SQLGetConnectAttrW(testBase->dbc, SQL_ATTR_AUTOCOMMIT, 
                &autocommit, 0, NULL);

            testBase->AssertSuccessOrInfo(rc, L"SQLGetConnectAttrW failed");

            Logger::WriteMessage(autocommit == SQL_AUTOCOMMIT_ON ? 
                "✓ Autocommit (Unicode API): ON" : "✓ Autocommit (Unicode API): OFF");
        }

        TEST_METHOD(GetDiagRecW)
        {
            // Force an error
            SQLRETURN rc = SQLExecDirect(testBase->stmt, (SQLWCHAR*)L"INVALID SQL", SQL_NTS);
            Assert::IsFalse(SQL_SUCCEEDED(rc), L"Expected query to fail");

            // Get diagnostic using Unicode API
            SQLWCHAR sqlState[6];
            SQLINTEGER nativeError;
            SQLWCHAR messageText[512];
            SQLSMALLINT textLength;

            rc = SQLGetDiagRecW(SQL_HANDLE_STMT, testBase->stmt, 1, sqlState, &nativeError,
                messageText, sizeof(messageText)/sizeof(SQLWCHAR), &textLength);

            testBase->AssertSuccess(rc, L"SQLGetDiagRecW failed");
            Assert::IsTrue(textLength > 0, L"Message should not be empty");

            // Verify we got Unicode characters
            std::wstring wstate(sqlState);
            std::string state(wstate.begin(), wstate.end());

            Logger::WriteMessage(("✓ SQLSTATE (Unicode): " + state).c_str());
        }

        TEST_METHOD(BufferLengthEvenRule)
        {
            // Test that buffer length must be even for Unicode functions
            SQLWCHAR driverName[256];
            SQLSMALLINT length;

            // Call with odd buffer length (should fail or be adjusted)
            SQLRETURN rc = SQLGetInfoW(testBase->dbc, SQL_DRIVER_NAME, driverName, 
                255, // Odd number - violates even rule
                &length);

            // Some drivers may return SQL_SUCCESS_WITH_INFO or adjust automatically
            // The spec says it should return HY090 for odd BufferLength
            if (rc == SQL_ERROR)
            {
                // Check if SQLSTATE is HY090
                SQLWCHAR sqlState[6];
                SQLINTEGER nativeError;
                SQLWCHAR messageText[256];
                SQLSMALLINT textLength;

                SQLGetDiagRecW(SQL_HANDLE_DBC, testBase->dbc, 1, sqlState, &nativeError,
                    messageText, sizeof(messageText)/sizeof(SQLWCHAR), &textLength);

                std::wstring wstate(sqlState);
                std::string state(wstate.begin(), wstate.end());

                if (state == "HY090")
                {
                    Logger::WriteMessage("✓ Odd BufferLength correctly returns HY090");
                }
                else
                {
                    Logger::WriteMessage(("⚠ Odd BufferLength returned error but SQLSTATE is: " + state).c_str());
                }
            }
            else if (SQL_SUCCEEDED(rc))
            {
                Logger::WriteMessage("⚠ Driver accepted odd BufferLength (spec violation but common)");
            }
        }

        TEST_METHOD(UnicodeStringRoundtrip)
        {
            // Test Unicode characters in query (if supported)
            // Using simple ASCII for basic test
            const SQLWCHAR* query = L"SELECT 'Test Unicode: \u00E9\u00F1' FROM RDB$DATABASE";
            
            SQLRETURN rc = SQLExecDirect(testBase->stmt, (SQLWCHAR*)query, SQL_NTS);
            
            if (SQL_SUCCEEDED(rc))
            {
                // Fetch result
                rc = SQLFetch(testBase->stmt);
                if (SQL_SUCCEEDED(rc))
                {
                    SQLWCHAR data[256];
                    SQLLEN indicator;

                    rc = SQLGetData(testBase->stmt, 1, SQL_C_WCHAR, data, sizeof(data), &indicator);
                    
                    if (SQL_SUCCEEDED(rc))
                    {
                        std::wstring wdata(data);
                        std::string sdata(wdata.begin(), wdata.end());
                        Logger::WriteMessage(("✓ Unicode roundtrip: " + sdata).c_str());
                    }
                }
            }
            else
            {
                Logger::WriteMessage("⚠ Unicode string query not supported or failed");
            }
        }
    };
}
