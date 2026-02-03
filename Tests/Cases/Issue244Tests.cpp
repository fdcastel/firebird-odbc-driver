// Tests for Unicode support issue #244
// https://github.com/FirebirdSQL/firebird-odbc-driver/issues/244
//
// These tests document the current broken behavior and should FAIL until the issue is fixed.
// Once fixed, these tests should PASS.

#include "pch.h"
#include "../Fixtures/TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace OdbcTests
{
    TEST_CLASS(Issue244Tests)
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

        // Test 1: Verify SQLWCHAR is always 16-bit regardless of wchar_t size
        TEST_METHOD(Test_SQLWCHAR_Size)
        {
            // SQLWCHAR must always be 16-bit (2 bytes) per ODBC spec
            // This should pass on all platforms
            Assert::AreEqual(2, (int)sizeof(SQLWCHAR), 
                L"SQLWCHAR must be 16-bit (2 bytes) on all platforms per ODBC spec");

            // wchar_t varies by platform: 2 bytes on Windows, 4 bytes on most Unix/Linux
#ifdef _WIN32
            Assert::AreEqual(2, (int)sizeof(wchar_t), L"wchar_t is 2 bytes on Windows");
#else
            // On Linux, wchar_t is typically 4 bytes (UTF-32)
            Logger::WriteMessage("Note: On Linux/Unix, wchar_t is typically 4 bytes (UTF-32)");
            Logger::WriteMessage(("sizeof(wchar_t) = " + std::to_string(sizeof(wchar_t))).c_str());
#endif
        }

        // Test 2: SQLDescribeColW should return SQL_WCHAR type for CHAR columns
        TEST_METHOD(Test_DescribeCol_Returns_WCHAR_Type)
        {
            // Create a test table with VARCHAR column
            SQLRETURN rc = SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"CREATE TABLE test_unicode_describe (name VARCHAR(50))", SQL_NTS);
            testBase->AssertSuccess(rc, L"Failed to create test table");

            // Prepare a SELECT statement
            rc = SQLPrepare(testBase->stmt, 
                (SQLWCHAR*)L"SELECT name FROM test_unicode_describe", SQL_NTS);
            testBase->AssertSuccess(rc, L"Failed to prepare SELECT");

            // Describe the column using Unicode API
            SQLWCHAR colName[256];
            SQLSMALLINT nameLen;
            SQLSMALLINT dataType;
            SQLULEN columnSize;
            SQLSMALLINT decimalDigits;
            SQLSMALLINT nullable;

            rc = SQLDescribeColW(testBase->stmt, 1, colName, 
                sizeof(colName) / sizeof(SQLWCHAR), &nameLen, 
                &dataType, &columnSize, &decimalDigits, &nullable);
            
            testBase->AssertSuccess(rc, L"SQLDescribeColW failed");

            // Per ODBC spec, when using Unicode API (xxxW functions), 
            // character columns should be reported as SQL_WCHAR/SQL_WVARCHAR
            // CURRENTLY FAILS: Driver returns SQL_CHAR/SQL_VARCHAR instead
            Assert::AreEqual((int)SQL_WVARCHAR, (int)dataType, 
                L"Unicode API should return SQL_WVARCHAR for VARCHAR columns");

            // Cleanup
            SQLFreeStmt(testBase->stmt, SQL_CLOSE);
            SQLExecDirect(testBase->stmt, (SQLWCHAR*)L"DROP TABLE test_unicode_describe", SQL_NTS);
        }

        // Test 3: SQLColAttributeW should return correct string lengths
        TEST_METHOD(Test_ColAttribute_String_Length)
        {
            SQLRETURN rc = SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"SELECT 'TestValue' AS test_col FROM RDB$DATABASE", SQL_NTS);
            testBase->AssertSuccess(rc, L"Failed to execute SELECT");

            SQLWCHAR colName[256];
            SQLSMALLINT strLen;

            rc = SQLColAttributeW(testBase->stmt, 1, SQL_DESC_NAME, 
                colName, sizeof(colName), &strLen, NULL);
            testBase->AssertSuccess(rc, L"SQLColAttributeW failed");

            // strLen should be in bytes for xxxW functions
            // CURRENTLY FAILS on Linux: returns character count instead of byte count
            Assert::IsTrue(strLen > 0, L"String length should be > 0");
            Assert::IsTrue(strLen % 2 == 0, L"String length should be even (bytes)");

            std::wstring name(colName);
            Logger::WriteMessage(("Column name: " + std::string(name.begin(), name.end())).c_str());
            Logger::WriteMessage(("String length returned: " + std::to_string(strLen)).c_str());
        }

        // Test 4: Test UTF-16 data roundtrip with non-ASCII characters
        TEST_METHOD(Test_UTF16_Data_Roundtrip)
        {
            // Drop table if it exists from previous run (ignore errors)
            SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"DROP TABLE test_unicode_data", SQL_NTS);
            
            // Create table with Unicode data
            SQLRETURN rc = SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"CREATE TABLE test_unicode_data (text VARCHAR(100) CHARACTER SET UTF8)", 
                SQL_NTS);
            testBase->AssertSuccess(rc, L"Failed to create table");

            // Insert Unicode data: Russian, Japanese, emoji
            const SQLWCHAR* testData = L"Привет 日本 🔥";
            
            rc = SQLPrepare(testBase->stmt, 
                (SQLWCHAR*)L"INSERT INTO test_unicode_data VALUES (?)", SQL_NTS);
            testBase->AssertSuccess(rc, L"Failed to prepare INSERT");

            SQLLEN indicator = SQL_NTS;
            rc = SQLBindParameter(testBase->stmt, 1, SQL_PARAM_INPUT, 
                SQL_C_WCHAR, SQL_WVARCHAR, 100, 0, (SQLPOINTER)testData, 0, &indicator);
            testBase->AssertSuccess(rc, L"Failed to bind parameter");

            rc = SQLExecute(testBase->stmt);
            testBase->AssertSuccess(rc, L"Failed to execute INSERT");

            SQLFreeStmt(testBase->stmt, SQL_CLOSE);
            SQLFreeStmt(testBase->stmt, SQL_RESET_PARAMS);

            // Retrieve the data
            rc = SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"SELECT text FROM test_unicode_data", SQL_NTS);
            testBase->AssertSuccess(rc, L"Failed to execute SELECT");

            rc = SQLFetch(testBase->stmt);
            testBase->AssertSuccess(rc, L"Failed to fetch");

            SQLWCHAR retrieved[256];
            SQLLEN retrievedLen;

            rc = SQLGetData(testBase->stmt, 1, SQL_C_WCHAR, 
                retrieved, sizeof(retrieved), &retrievedLen);
            testBase->AssertSuccess(rc, L"Failed to get data");

            // CURRENTLY FAILS on Linux: Data corruption due to wchar_t/SQLWCHAR mismatch
            std::wstring original(testData);
            std::wstring result(retrieved);
            
            Assert::AreEqual(original, result, 
                L"Unicode data should roundtrip correctly");

            // Cleanup
            SQLFreeStmt(testBase->stmt, SQL_CLOSE);
            SQLExecDirect(testBase->stmt, (SQLWCHAR*)L"DROP TABLE test_unicode_data", SQL_NTS);
        }

        // Test 5: Test BufferLength must be even for xxxW functions
        TEST_METHOD(Test_BufferLength_Even_Validation)
        {
            SQLWCHAR driverName[256];
            SQLSMALLINT length;

            // Call with odd buffer length - should return HY090 error
            SQLRETURN rc = SQLGetInfoW(testBase->dbc, SQL_DRIVER_NAME, 
                driverName, 255, &length); // 255 is odd

            // CURRENTLY FAILS: Driver doesn't validate even BufferLength
            if (rc == SQL_ERROR)
            {
                SQLWCHAR sqlState[6];
                SQLINTEGER nativeError;
                SQLWCHAR messageText[256];
                SQLSMALLINT textLength;

                SQLGetDiagRecW(SQL_HANDLE_DBC, testBase->dbc, 1, 
                    sqlState, &nativeError, messageText, 
                    sizeof(messageText) / sizeof(SQLWCHAR), &textLength);

                std::wstring state(sqlState);
                std::string stateStr(state.begin(), state.end());

                Assert::AreEqual(std::string("HY090"), stateStr, 
                    L"Odd BufferLength should return SQLSTATE HY090");
            }
            else
            {
                Assert::Fail(L"Driver should return SQL_ERROR for odd BufferLength");
            }
        }

        // Test 6: Test connection with Unicode DSN name
        TEST_METHOD(Test_ConnectW_With_Unicode_DSN)
        {
            // Test that SQLConnectW properly handles UTF-16 encoding
            // CURRENTLY FAILS on Linux: Uses mbstowcs which is locale-specific
            
            SQLHANDLE testEnv, testDbc;
            
            SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &testEnv);
            Assert::AreEqual((int)SQL_SUCCESS, (int)rc);

            rc = SQLSetEnvAttr(testEnv, SQL_ATTR_ODBC_VERSION, 
                (SQLPOINTER)SQL_OV_ODBC3, 0);
            Assert::AreEqual((int)SQL_SUCCESS, (int)rc);

            rc = SQLAllocHandle(SQL_HANDLE_DBC, testEnv, &testDbc);
            Assert::AreEqual((int)SQL_SUCCESS, (int)rc);

            // Try to connect with Unicode DSN
            // Even if connection fails, we're testing the conversion doesn't crash
            const SQLWCHAR* dsn = L"TestDSN_Тест";
            const SQLWCHAR* user = L"SYSDBA";
            const SQLWCHAR* pwd = L"masterkey";

            rc = SQLConnectW(testDbc, (SQLWCHAR*)dsn, SQL_NTS, 
                (SQLWCHAR*)user, SQL_NTS, (SQLWCHAR*)pwd, SQL_NTS);

            // We expect this to fail (DSN doesn't exist), but it shouldn't crash
            Logger::WriteMessage(SQL_SUCCEEDED(rc) ? 
                "Connection succeeded (unexpected)" : 
                "Connection failed as expected (DSN doesn't exist)");

            SQLFreeHandle(SQL_HANDLE_DBC, testDbc);
            SQLFreeHandle(SQL_HANDLE_ENV, testEnv);
        }

        // Test 7: Test that mbstowcs/wcstombs are NOT used for UTF-16 conversion
        TEST_METHOD(Test_No_Locale_Dependency)
        {
            // The driver should NOT use mbstowcs/wcstombs which are locale-dependent
            // It should use proper UTF-16 <-> UTF-8 conversion
            
            // Drop table if it exists from previous run (ignore errors)
            SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"DROP TABLE test_locale", SQL_NTS);
            
            // Insert data with characters outside current locale
            SQLRETURN rc = SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"CREATE TABLE test_locale (text VARCHAR(100) CHARACTER SET UTF8)", 
                SQL_NTS);
            testBase->AssertSuccess(rc, L"Failed to create table");

            // Japanese characters that might not be in current locale
            const SQLWCHAR* japaneseText = L"日本語テスト";
            
            rc = SQLPrepare(testBase->stmt, 
                (SQLWCHAR*)L"INSERT INTO test_locale VALUES (?)", SQL_NTS);
            testBase->AssertSuccess(rc, L"Failed to prepare INSERT");

            SQLLEN indicator = SQL_NTS;
            rc = SQLBindParameter(testBase->stmt, 1, SQL_PARAM_INPUT, 
                SQL_C_WCHAR, SQL_WVARCHAR, 100, 0, 
                (SQLPOINTER)japaneseText, 0, &indicator);
            testBase->AssertSuccess(rc, L"Failed to bind parameter");

            rc = SQLExecute(testBase->stmt);
            // CURRENTLY FAILS on systems where Japanese is not in locale
            testBase->AssertSuccess(rc, L"Insert should work regardless of locale");

            // Cleanup
            SQLFreeStmt(testBase->stmt, SQL_CLOSE);
            SQLExecDirect(testBase->stmt, (SQLWCHAR*)L"DROP TABLE test_locale", SQL_NTS);
        }

        // Test 8: Verify sizeof(wchar_t) is not hardcoded to 2
        TEST_METHOD(Test_No_Hardcoded_WChar_Size)
        {
            // Test that driver doesn't assume sizeof(wchar_t) == 2
            // This is tested indirectly through proper data conversion
            
            const SQLWCHAR* testStr = L"Test";
            SQLLEN strLen = SQL_NTS;

            // Bind as input parameter
            SQLRETURN rc = SQLPrepare(testBase->stmt, 
                (SQLWCHAR*)L"SELECT CAST(? AS VARCHAR(50)) FROM RDB$DATABASE", 
                SQL_NTS);
            testBase->AssertSuccess(rc, L"Failed to prepare");

            rc = SQLBindParameter(testBase->stmt, 1, SQL_PARAM_INPUT, 
                SQL_C_WCHAR, SQL_WVARCHAR, 50, 0, 
                (SQLPOINTER)testStr, 0, &strLen);
            testBase->AssertSuccess(rc, L"Failed to bind");

            rc = SQLExecute(testBase->stmt);
            testBase->AssertSuccess(rc, L"Failed to execute");

            rc = SQLFetch(testBase->stmt);
            testBase->AssertSuccess(rc, L"Failed to fetch");

            SQLWCHAR result[256];
            SQLLEN resultLen;

            rc = SQLGetData(testBase->stmt, 1, SQL_C_WCHAR, 
                result, sizeof(result), &resultLen);
            
            // CURRENTLY FAILS on Linux: sizeof(wchar_t) assumptions cause issues
            testBase->AssertSuccess(rc, L"GetData should work on all platforms");

            SQLFreeStmt(testBase->stmt, SQL_CLOSE);
        }

        // Test 9: Test SQL_C_WCHAR vs SQL_C_CHAR type indicators
        TEST_METHOD(Test_WChar_Type_Indicators)
        {
            SQLRETURN rc = SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"SELECT 'Test' FROM RDB$DATABASE", SQL_NTS);
            testBase->AssertSuccess(rc, L"Failed to execute");

            rc = SQLFetch(testBase->stmt);
            testBase->AssertSuccess(rc, L"Failed to fetch");

            // Test SQL_C_WCHAR binding
            SQLWCHAR wcharData[256];
            SQLLEN wcharLen;

            rc = SQLGetData(testBase->stmt, 1, SQL_C_WCHAR, 
                wcharData, sizeof(wcharData), &wcharLen);
            testBase->AssertSuccess(rc, L"SQL_C_WCHAR should work");

            // wcharLen should be in bytes
            Assert::IsTrue(wcharLen > 0 && wcharLen % 2 == 0, 
                L"Length for SQL_C_WCHAR should be in bytes (even number)");

            Logger::WriteMessage(("SQL_C_WCHAR data length: " + 
                std::to_string(wcharLen)).c_str());

            SQLFreeStmt(testBase->stmt, SQL_CLOSE);
        }

        // Test 10: Test error messages in Unicode
        TEST_METHOD(Test_Unicode_Error_Messages)
        {
            // Force an error
            SQLRETURN rc = SQLExecDirect(testBase->stmt, 
                (SQLWCHAR*)L"INVALID SQL STATEMENT", SQL_NTS);
            Assert::IsFalse(SQL_SUCCEEDED(rc), L"Expected error");

            // Get error using Unicode API
            SQLWCHAR sqlState[6];
            SQLINTEGER nativeError;
            SQLWCHAR messageText[1024];
            SQLSMALLINT textLength;

            rc = SQLGetDiagRecW(SQL_HANDLE_STMT, testBase->stmt, 1, 
                sqlState, &nativeError, messageText, 
                sizeof(messageText) / sizeof(SQLWCHAR), &textLength);

            testBase->AssertSuccess(rc, L"SQLGetDiagRecW should succeed");

            // Verify we got a proper message
            Assert::IsTrue(textLength > 0, L"Error message should not be empty");
            
            // CURRENTLY FAILS on Linux: Message conversion may be corrupted
            std::wstring msg(messageText);
            Assert::IsTrue(msg.length() > 0, L"Message should be valid UTF-16");

            Logger::WriteMessage(("Error message length: " + 
                std::to_string(textLength)).c_str());
        }
    };
}
