#include "pch.h"
#include "TestBase.h"
#include <cstdlib>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace OdbcTests
{
    TestBase::TestBase()
    {
        env = SQL_NULL_HENV;
        dbc = SQL_NULL_HDBC;
        stmt = SQL_NULL_HSTMT;
    }

    TestBase::~TestBase()
    {
    }

    std::string TestBase::GetConnectionString()
    {
        // Read from environment variable
        char* connStr = nullptr;
        size_t len = 0;
        _dupenv_s(&connStr, &len, "FIREBIRD_ODBC_CONNECTION");
        
        if (connStr == nullptr || len == 0)
        {
            if (connStr) free(connStr);
            Assert::Fail(L"Environment variable FIREBIRD_ODBC_CONNECTION is not set.\n"
                        L"Please set it to a valid connection string, e.g.:\n"
                        L"Driver={Firebird ODBC Driver};Database=/path/to/test.fdb;UID=SYSDBA;PWD=masterkey");
        }
        
        std::string result(connStr);
        free(connStr);
        return result;
    }

    void TestBase::SetUp()
    {
        SQLRETURN rc;

        // Get connection string
        connectionString = GetConnectionString();

        // Allocate environment handle
        rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
        AssertSuccess(rc, L"Failed to allocate environment handle");

        // Set ODBC version
        rc = SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
        AssertSuccess(rc, L"Failed to set ODBC version");

        // Allocate connection handle
        rc = SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
        AssertSuccess(rc, L"Failed to allocate connection handle");

        // Connect to database
        SQLSMALLINT outLen;
        // Convert connection string to wide char
        std::wstring wideConnStr(connectionString.begin(), connectionString.end());
        rc = SQLDriverConnect(dbc, NULL, 
            (SQLWCHAR*)wideConnStr.c_str(), (SQLSMALLINT)wideConnStr.length(),
            NULL, 0, &outLen, SQL_DRIVER_NOPROMPT);
        
        if (!SQL_SUCCEEDED(rc))
        {
            std::string diag = GetDiagnostics(dbc, SQL_HANDLE_DBC);
            std::wstring msg = L"Failed to connect to database.\n"
                              L"Connection string: ";
            msg += std::wstring(connectionString.begin(), connectionString.end());
            msg += L"\nDiagnostics: ";
            msg += std::wstring(diag.begin(), diag.end());
            Assert::Fail(msg.c_str());
        }

        // Allocate statement handle
        rc = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
        AssertSuccess(rc, L"Failed to allocate statement handle");
    }

    void TestBase::TearDown()
    {
        // Free statement handle
        if (stmt != SQL_NULL_HSTMT)
        {
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            stmt = SQL_NULL_HSTMT;
        }

        // Disconnect
        if (dbc != SQL_NULL_HDBC)
        {
            SQLDisconnect(dbc);
            SQLFreeHandle(SQL_HANDLE_DBC, dbc);
            dbc = SQL_NULL_HDBC;
        }

        // Free environment handle
        if (env != SQL_NULL_HENV)
        {
            SQLFreeHandle(SQL_HANDLE_ENV, env);
            env = SQL_NULL_HENV;
        }
    }

    void TestBase::AssertSuccess(SQLRETURN rc, const wchar_t* message)
    {
        if (rc != SQL_SUCCESS)
        {
            Assert::Fail(message);
        }
    }

    void TestBase::AssertSuccessOrInfo(SQLRETURN rc, const wchar_t* message)
    {
        if (!SQL_SUCCEEDED(rc))
        {
            Assert::Fail(message);
        }
    }

    void TestBase::AssertSqlState(SQLHANDLE handle, SQLSMALLINT handleType, const char* expectedState)
    {
        SQLWCHAR sqlState[6];
        SQLINTEGER nativeError;
        SQLWCHAR messageText[256];
        SQLSMALLINT textLength;

        SQLRETURN rc = SQLGetDiagRec(handleType, handle, 1, sqlState, &nativeError,
            messageText, sizeof(messageText)/sizeof(SQLWCHAR), &textLength);

        if (SQL_SUCCEEDED(rc))
        {
            // Convert SQLWCHAR to char for comparison
            char actualState[6];
            for (int i = 0; i < 5; i++)
                actualState[i] = (char)sqlState[i];
            actualState[5] = 0;
            
            std::string actual(actualState);
            std::string expected(expectedState);
            
            if (actual != expected)
            {
                std::wstring msg = L"Expected SQLSTATE: ";
                msg += std::wstring(expected.begin(), expected.end());
                msg += L", Actual: ";
                msg += std::wstring(actual.begin(), actual.end());
                msg += L"\nMessage: ";
                msg += messageText;
                Assert::Fail(msg.c_str());
            }
        }
        else
        {
            Assert::Fail(L"Failed to get diagnostic record");
        }
    }

    void TestBase::AssertSqlStateStartsWith(SQLHANDLE handle, SQLSMALLINT handleType, const char* prefix)
    {
        SQLWCHAR sqlState[6];
        SQLINTEGER nativeError;
        SQLWCHAR messageText[256];
        SQLSMALLINT textLength;

        SQLRETURN rc = SQLGetDiagRec(handleType, handle, 1, sqlState, &nativeError,
            messageText, sizeof(messageText)/sizeof(SQLWCHAR), &textLength);

        if (SQL_SUCCEEDED(rc))
        {
            // Convert SQLWCHAR to char for comparison
            char actualState[6];
            for (int i = 0; i < 5; i++)
                actualState[i] = (char)sqlState[i];
            actualState[5] = 0;
            
            std::string actual(actualState);
            std::string expectedPrefix(prefix);
            
            if (actual.substr(0, expectedPrefix.length()) != expectedPrefix)
            {
                std::wstring msg = L"Expected SQLSTATE to start with: ";
                msg += std::wstring(expectedPrefix.begin(), expectedPrefix.end());
                msg += L", Actual: ";
                msg += std::wstring(actual.begin(), actual.end());
                Assert::Fail(msg.c_str());
            }
        }
        else
        {
            Assert::Fail(L"Failed to get diagnostic record");
        }
    }

    std::string TestBase::GetDiagnostics(SQLHANDLE handle, SQLSMALLINT handleType)
    {
        SQLWCHAR sqlState[6];
        SQLINTEGER nativeError;
        SQLWCHAR messageText[1024];
        SQLSMALLINT textLength;
        std::string result;

        for (SQLSMALLINT i = 1; ; i++)
        {
            SQLRETURN rc = SQLGetDiagRec(handleType, handle, i, sqlState, &nativeError,
                messageText, sizeof(messageText)/sizeof(SQLWCHAR), &textLength);

            if (rc == SQL_NO_DATA)
                break;

            if (SQL_SUCCEEDED(rc))
            {
                if (i > 1)
                    result += "\n";
                
                // Convert SQLWCHAR to char
                char state[6];
                for (int j = 0; j < 5; j++)
                    state[j] = (char)sqlState[j];
                state[5] = 0;
                
                result += "[";
                result += state;
                result += "] ";
                
                // Convert message to char (simple conversion)
                for (SQLSMALLINT j = 0; j < textLength && j < 1023; j++)
                    result += (char)messageText[j];
            }
            else
            {
                break;
            }
        }

        return result;
    }
}
