#include "pch.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace OdbcTests
{
    TEST_CLASS(EnvAttrsTests)
    {
    public:
        
        TEST_METHOD(ValidateOdbcVersionSetGet)
        {
            SQLHENV testEnv = SQL_NULL_HENV;
            SQLRETURN rc;

            // Allocate environment handle
            rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &testEnv);
            Assert::AreEqual((int)SQL_SUCCESS, (int)rc, L"Failed to allocate environment");

            // Set ODBC version to 3
            rc = SQLSetEnvAttr(testEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
            Assert::AreEqual((int)SQL_SUCCESS, (int)rc, L"Failed to set ODBC version");

            // Get ODBC version
            SQLINTEGER version;
            rc = SQLGetEnvAttr(testEnv, SQL_ATTR_ODBC_VERSION, &version, 0, NULL);
            Assert::AreEqual((int)SQL_SUCCESS, (int)rc, L"Failed to get ODBC version");
            Assert::AreEqual((int)SQL_OV_ODBC3, (int)version, L"ODBC version mismatch");

            Logger::WriteMessage("✓ ODBC version set and verified as 3.x");
            
            SQLFreeHandle(SQL_HANDLE_ENV, testEnv);
        }

        TEST_METHOD(ValidateOutputNts)
        {
            SQLHENV testEnv = SQL_NULL_HENV;
            SQLRETURN rc;

            // Allocate environment handle
            rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &testEnv);
            Assert::AreEqual((int)SQL_SUCCESS, (int)rc);

            // Set ODBC version first
            rc = SQLSetEnvAttr(testEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
            Assert::AreEqual((int)SQL_SUCCESS, (int)rc);

            // Get SQL_ATTR_OUTPUT_NTS (should default to SQL_TRUE)
            SQLINTEGER outputNts;
            rc = SQLGetEnvAttr(testEnv, SQL_ATTR_OUTPUT_NTS, &outputNts, 0, NULL);
            Assert::AreEqual((int)SQL_SUCCESS, (int)rc, L"Failed to get SQL_ATTR_OUTPUT_NTS");
            Assert::AreEqual((int)SQL_TRUE, (int)outputNts, L"SQL_ATTR_OUTPUT_NTS should default to SQL_TRUE");

            Logger::WriteMessage("✓ SQL_ATTR_OUTPUT_NTS defaults to SQL_TRUE");
            
            SQLFreeHandle(SQL_HANDLE_ENV, testEnv);
        }

        TEST_METHOD(InvalidAttributeReturnsError)
        {
            SQLHENV testEnv = SQL_NULL_HENV;
            SQLRETURN rc;

            // Allocate environment handle
            rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &testEnv);
            Assert::AreEqual((int)SQL_SUCCESS, (int)rc);

            rc = SQLSetEnvAttr(testEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
            Assert::AreEqual((int)SQL_SUCCESS, (int)rc);

            // Try to get an invalid attribute (999999)
            SQLINTEGER value;
            rc = SQLGetEnvAttr(testEnv, 999999, &value, 0, NULL);
            Assert::AreNotEqual((int)SQL_SUCCESS, (int)rc, L"Invalid attribute should fail");

            Logger::WriteMessage("✓ Invalid attribute returns error as expected");
            
            SQLFreeHandle(SQL_HANDLE_ENV, testEnv);
        }
    };
}
