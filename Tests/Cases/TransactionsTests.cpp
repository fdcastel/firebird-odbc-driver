#include "pch.h"
#include "../Fixtures/TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace OdbcTests
{
    TEST_CLASS(TransactionsTests)
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

        TEST_METHOD(EndTranCommit)
        {
            // Turn off autocommit
            SQLRETURN rc = SQLSetConnectAttr(testBase->dbc, SQL_ATTR_AUTOCOMMIT, 
                (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
            testBase->AssertSuccessOrInfo(rc, L"Failed to turn off autocommit");

            // Execute a query (starts a transaction)
            rc = SQLExecDirect(testBase->stmt, (SQLWCHAR*)L"SELECT 1 FROM RDB$DATABASE", SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to execute query");

            // Commit the transaction
            rc = SQLEndTran(SQL_HANDLE_DBC, testBase->dbc, SQL_COMMIT);
            testBase->AssertSuccessOrInfo(rc, L"SQLEndTran(COMMIT) failed");

            Logger::WriteMessage("✓ Transaction committed successfully");

            // Restore autocommit
            rc = SQLSetConnectAttr(testBase->dbc, SQL_ATTR_AUTOCOMMIT, 
                (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
        }

        TEST_METHOD(EndTranRollback)
        {
            // Turn off autocommit
            SQLRETURN rc = SQLSetConnectAttr(testBase->dbc, SQL_ATTR_AUTOCOMMIT, 
                (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
            testBase->AssertSuccessOrInfo(rc, L"Failed to turn off autocommit");

            // Execute a query (starts a transaction)
            rc = SQLExecDirect(testBase->stmt, (SQLWCHAR*)L"SELECT 1 FROM RDB$DATABASE", SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to execute query");

            // Rollback the transaction
            rc = SQLEndTran(SQL_HANDLE_DBC, testBase->dbc, SQL_ROLLBACK);
            testBase->AssertSuccessOrInfo(rc, L"SQLEndTran(ROLLBACK) failed");

            Logger::WriteMessage("✓ Transaction rolled back successfully");

            // Restore autocommit
            rc = SQLSetConnectAttr(testBase->dbc, SQL_ATTR_AUTOCOMMIT, 
                (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
        }

        TEST_METHOD(AutocommitBehavior)
        {
            // Verify autocommit is on by default (or turn it on)
            SQLRETURN rc = SQLSetConnectAttr(testBase->dbc, SQL_ATTR_AUTOCOMMIT, 
                (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
            testBase->AssertSuccessOrInfo(rc, L"Failed to set autocommit on");

            // Execute a query - should auto-commit
            rc = SQLExecDirect(testBase->stmt, (SQLWCHAR*)L"SELECT 1 FROM RDB$DATABASE", SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to execute query with autocommit");

            Logger::WriteMessage("✓ Query executed with autocommit ON");

            // Turn autocommit off
            rc = SQLSetConnectAttr(testBase->dbc, SQL_ATTR_AUTOCOMMIT, 
                (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
            testBase->AssertSuccessOrInfo(rc, L"Failed to set autocommit off");

            // Execute a query - requires explicit commit
            rc = SQLExecDirect(testBase->stmt, (SQLWCHAR*)L"SELECT 2 FROM RDB$DATABASE", SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to execute query without autocommit");

            // Rollback to clean up
            rc = SQLEndTran(SQL_HANDLE_DBC, testBase->dbc, SQL_ROLLBACK);
            testBase->AssertSuccessOrInfo(rc, L"Failed to rollback");

            Logger::WriteMessage("✓ Query executed with autocommit OFF and rolled back");

            // Restore autocommit
            rc = SQLSetConnectAttr(testBase->dbc, SQL_ATTR_AUTOCOMMIT, 
                (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
        }

        TEST_METHOD(TransactionIsolation)
        {
            // Try to get current isolation level
            SQLUINTEGER isolation;
            SQLRETURN rc = SQLGetConnectAttr(testBase->dbc, SQL_ATTR_TXN_ISOLATION, &isolation, 0, NULL);
            
            if (SQL_SUCCEEDED(rc))
            {
                const char* levelName = "Unknown";
                switch (isolation)
                {
                case SQL_TXN_READ_UNCOMMITTED: levelName = "READ UNCOMMITTED"; break;
                case SQL_TXN_READ_COMMITTED: levelName = "READ COMMITTED"; break;
                case SQL_TXN_REPEATABLE_READ: levelName = "REPEATABLE READ"; break;
                case SQL_TXN_SERIALIZABLE: levelName = "SERIALIZABLE"; break;
                }

                char msg[128];
                sprintf_s(msg, "Current isolation level: %s", levelName);
                Logger::WriteMessage(msg);

                // Try to set isolation level
                rc = SQLSetConnectAttr(testBase->dbc, SQL_ATTR_TXN_ISOLATION, 
                    (SQLPOINTER)SQL_TXN_READ_COMMITTED, 0);
                
                if (SQL_SUCCEEDED(rc))
                {
                    Logger::WriteMessage("✓ Transaction isolation level set to READ COMMITTED");
                }
                else
                {
                    Logger::WriteMessage("⚠ Could not change isolation level (may require no active transaction)");
                }
            }
            else
            {
                Logger::WriteMessage("⚠ SQL_ATTR_TXN_ISOLATION not supported");
            }
        }
    };
}
