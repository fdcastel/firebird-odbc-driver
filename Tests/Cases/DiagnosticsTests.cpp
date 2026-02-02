#include "pch.h"
#include "../Fixtures/TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace OdbcTests
{
    TEST_CLASS(DiagnosticsTests)
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

        TEST_METHOD(GetDiagRecBasic)
        {
            // Force an error by executing invalid SQL
            SQLRETURN rc = SQLExecDirect(testBase->stmt, (SQLWCHAR*)L"SELECT * FROM NONEXISTENT_TABLE", SQL_NTS);
            Assert::IsFalse(SQL_SUCCEEDED(rc), L"Expected query to fail");

            // Get diagnostic record
            SQLWCHAR sqlState[6];
            SQLINTEGER nativeError;
            SQLWCHAR messageText[256];
            SQLSMALLINT textLength;

            rc = SQLGetDiagRec(SQL_HANDLE_STMT, testBase->stmt, 1, sqlState, &nativeError,
                messageText, sizeof(messageText)/sizeof(SQLWCHAR), &textLength);

            testBase->AssertSuccess(rc, L"SQLGetDiagRec failed");
            Assert::IsTrue(textLength > 0, L"Message text should not be empty");

            // Convert to narrow string for logging
            std::wstring wstate(sqlState);
            std::string state(wstate.begin(), wstate.end());

            char msg[512];
            sprintf_s(msg, "✓ SQLSTATE: %s, Native: %d", state.c_str(), (int)nativeError);
            Logger::WriteMessage(msg);
        }

        TEST_METHOD(GetDiagFieldRowColumn)
        {
            // Prepare a query
            SQLRETURN rc = SQLPrepare(testBase->stmt, (SQLWCHAR*)L"SELECT 1 FROM RDB$DATABASE", SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to prepare");

            // Try to get diagnostic field SQL_DIAG_NUMBER
            SQLINTEGER diagNumber;
            rc = SQLGetDiagField(SQL_HANDLE_STMT, testBase->stmt, 0, SQL_DIAG_NUMBER, 
                &diagNumber, 0, NULL);

            if (SQL_SUCCEEDED(rc))
            {
                char msg[64];
                sprintf_s(msg, "✓ SQL_DIAG_NUMBER: %d", (int)diagNumber);
                Logger::WriteMessage(msg);
            }
            else
            {
                Logger::WriteMessage("⚠ SQL_DIAG_NUMBER not available (no errors)");
            }

            // Try SQL_DIAG_ROW_NUMBER (only valid after certain errors)
            SQLLEN rowNumber;
            rc = SQLGetDiagField(SQL_HANDLE_STMT, testBase->stmt, 1, SQL_DIAG_ROW_NUMBER, 
                &rowNumber, 0, NULL);
            
            // May return SQL_NO_DATA if no diagnostic records
            if (rc == SQL_NO_DATA)
            {
                Logger::WriteMessage("✓ SQL_DIAG_ROW_NUMBER: SQL_NO_DATA (expected with no errors)");
            }
            else if (SQL_SUCCEEDED(rc))
            {
                Logger::WriteMessage("✓ SQL_DIAG_ROW_NUMBER retrieved");
            }
        }

        TEST_METHOD(DiagNoData)
        {
            // Clear any previous errors by allocating a new statement
            SQLHSTMT cleanStmt;
            SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, testBase->dbc, &cleanStmt);
            testBase->AssertSuccess(rc, L"Failed to allocate statement");

            // Try to get diagnostic when there are no errors
            SQLWCHAR sqlState[6];
            SQLINTEGER nativeError;
            SQLWCHAR messageText[256];
            SQLSMALLINT textLength;

            rc = SQLGetDiagRec(SQL_HANDLE_STMT, cleanStmt, 1, sqlState, &nativeError,
                messageText, sizeof(messageText)/sizeof(SQLWCHAR), &textLength);

            Assert::AreEqual((int)SQL_NO_DATA, (int)rc, L"Expected SQL_NO_DATA for clean statement");

            SQLFreeHandle(SQL_HANDLE_STMT, cleanStmt);

            Logger::WriteMessage("✓ SQLGetDiagRec returns SQL_NO_DATA when no diagnostics");
        }

        TEST_METHOD(DiagNumberField)
        {
            // Force an error
            SQLRETURN rc = SQLExecDirect(testBase->stmt, (SQLWCHAR*)L"INVALID SQL SYNTAX", SQL_NTS);
            Assert::IsFalse(SQL_SUCCEEDED(rc), L"Expected query to fail");

            // Get diagnostic number with NULL StringLengthPtr
            SQLINTEGER diagNumber;
            rc = SQLGetDiagField(SQL_HANDLE_STMT, testBase->stmt, 0, SQL_DIAG_NUMBER, 
                &diagNumber, 0, NULL);

            testBase->AssertSuccess(rc, L"SQLGetDiagField failed");
            Assert::IsTrue(diagNumber > 0, L"Should have at least one diagnostic record");

            // Now get it with StringLengthPtr
            SQLSMALLINT length;
            rc = SQLGetDiagField(SQL_HANDLE_STMT, testBase->stmt, 0, SQL_DIAG_NUMBER, 
                &diagNumber, 0, &length);

            testBase->AssertSuccess(rc, L"SQLGetDiagField with length failed");

            char msg[64];
            sprintf_s(msg, "✓ Diagnostic records: %d", (int)diagNumber);
            Logger::WriteMessage(msg);
        }

        TEST_METHOD(MultipleDiagRecords)
        {
            // Try to create multiple errors (may not be possible with all drivers)
            // Force an error
            SQLRETURN rc = SQLExecDirect(testBase->stmt, (SQLWCHAR*)L"SELECT * FROM DOES_NOT_EXIST", SQL_NTS);
            Assert::IsFalse(SQL_SUCCEEDED(rc), L"Expected query to fail");

            // Try to get multiple diagnostic records
            int recordCount = 0;
            for (SQLSMALLINT i = 1; i <= 10; i++)
            {
                SQLWCHAR sqlState[6];
                SQLINTEGER nativeError;
                SQLWCHAR messageText[256];
                SQLSMALLINT textLength;

                rc = SQLGetDiagRec(SQL_HANDLE_STMT, testBase->stmt, i, sqlState, &nativeError,
                    messageText, sizeof(messageText)/sizeof(SQLWCHAR), &textLength);

                if (rc == SQL_NO_DATA)
                    break;

                if (SQL_SUCCEEDED(rc))
                    recordCount++;
            }

            Assert::IsTrue(recordCount > 0, L"Should have at least one diagnostic record");

            char msg[64];
            sprintf_s(msg, "✓ Retrieved %d diagnostic record(s)", recordCount);
            Logger::WriteMessage(msg);
        }
    };
}
