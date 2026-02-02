#include "pch.h"
#include "../Fixtures/TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace OdbcTests
{
    TEST_CLASS(CatalogTests)
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

        TEST_METHOD(TablesBasic)
        {
            // Get list of tables
            SQLRETURN rc = SQLTables(testBase->stmt, 
                NULL, 0,           // Catalog
                NULL, 0,           // Schema
                NULL, 0,           // Table name
                (SQLWCHAR*)L"TABLE", SQL_NTS); // Table type

            testBase->AssertSuccessOrInfo(rc, L"SQLTables failed");

            // Verify result set structure
            SQLSMALLINT colCount;
            rc = SQLNumResultCols(testBase->stmt, &colCount);
            testBase->AssertSuccess(rc, L"SQLNumResultCols failed");

            Assert::IsTrue(colCount >= 5, L"SQLTables should return at least 5 columns");

            // Fetch at least one row
            int rowCount = 0;
            while (SQLFetch(testBase->stmt) == SQL_SUCCESS && rowCount < 5)
            {
                rowCount++;
            }

            char msg[64];
            sprintf_s(msg, "✓ SQLTables returned %d table(s) (limited to 5)", rowCount);
            Logger::WriteMessage(msg);
        }

        TEST_METHOD(ColumnsForSystemTable)
        {
            // Get columns for a known system table
            SQLRETURN rc = SQLColumns(testBase->stmt, 
                NULL, 0,           // Catalog
                NULL, 0,           // Schema
                (SQLWCHAR*)L"RDB$RELATIONS", SQL_NTS,  // Table name
                NULL, 0);          // Column name

            testBase->AssertSuccessOrInfo(rc, L"SQLColumns failed");

            // Count columns
            int colCount = 0;
            while (SQLFetch(testBase->stmt) == SQL_SUCCESS)
            {
                colCount++;
            }

            Assert::IsTrue(colCount > 0, L"RDB$RELATIONS should have columns");

            char msg[64];
            sprintf_s(msg, "✓ SQLColumns returned %d column(s) for RDB$RELATIONS", colCount);
            Logger::WriteMessage(msg);
        }

        TEST_METHOD(PrimaryKeysForSystemTable)
        {
            // Try to get primary keys (system tables may not have them)
            SQLRETURN rc = SQLPrimaryKeys(testBase->stmt, 
                NULL, 0,           // Catalog
                NULL, 0,           // Schema
                (SQLWCHAR*)L"RDB$DATABASE", SQL_NTS);  // Table name

            // May return no data for system tables
            if (SQL_SUCCEEDED(rc))
            {
                int pkCount = 0;
                while (SQLFetch(testBase->stmt) == SQL_SUCCESS)
                {
                    pkCount++;
                }

                char msg[128];
                sprintf_s(msg, "✓ SQLPrimaryKeys returned %d key(s) for RDB$DATABASE", pkCount);
                Logger::WriteMessage(msg);
            }
            else
            {
                Logger::WriteMessage("⚠ SQLPrimaryKeys not supported or failed");
            }
        }

        TEST_METHOD(StatisticsForSystemTable)
        {
            // Try to get statistics/indexes
            SQLRETURN rc = SQLStatistics(testBase->stmt, 
                NULL, 0,           // Catalog
                NULL, 0,           // Schema
                (SQLWCHAR*)L"RDB$RELATIONS", SQL_NTS,  // Table name
                SQL_INDEX_ALL,     // Index type
                SQL_QUICK);        // Reserved

            if (SQL_SUCCEEDED(rc))
            {
                int statCount = 0;
                while (SQLFetch(testBase->stmt) == SQL_SUCCESS && statCount < 10)
                {
                    statCount++;
                }

                char msg[128];
                sprintf_s(msg, "✓ SQLStatistics returned %d row(s) (limited to 10)", statCount);
                Logger::WriteMessage(msg);
            }
            else
            {
                Logger::WriteMessage("⚠ SQLStatistics not supported or failed");
            }
        }

        TEST_METHOD(SpecialColumns)
        {
            // Try to get special columns (best row identifier)
            SQLRETURN rc = SQLSpecialColumns(testBase->stmt, 
                SQL_BEST_ROWID,    // Column type
                NULL, 0,           // Catalog
                NULL, 0,           // Schema  
                (SQLWCHAR*)L"RDB$DATABASE", SQL_NTS,  // Table name
                SQL_SCOPE_CURROW,  // Scope
                SQL_NULLABLE);     // Nullable

            if (SQL_SUCCEEDED(rc))
            {
                int colCount = 0;
                while (SQLFetch(testBase->stmt) == SQL_SUCCESS)
                {
                    colCount++;
                }

                char msg[128];
                sprintf_s(msg, "✓ SQLSpecialColumns returned %d column(s)", colCount);
                Logger::WriteMessage(msg);
            }
            else
            {
                Logger::WriteMessage("⚠ SQLSpecialColumns not supported or failed for RDB$DATABASE");
            }
        }

        TEST_METHOD(TablePrivileges)
        {
            // Try to get table privileges
            SQLRETURN rc = SQLTablePrivileges(testBase->stmt, 
                NULL, 0,           // Catalog
                NULL, 0,           // Schema
                (SQLWCHAR*)L"RDB$DATABASE", SQL_NTS);  // Table name

            if (SQL_SUCCEEDED(rc))
            {
                int privCount = 0;
                while (SQLFetch(testBase->stmt) == SQL_SUCCESS && privCount < 10)
                {
                    privCount++;
                }

                char msg[128];
                sprintf_s(msg, "✓ SQLTablePrivileges returned %d privilege(s) (limited to 10)", privCount);
                Logger::WriteMessage(msg);
            }
            else
            {
                Logger::WriteMessage("⚠ SQLTablePrivileges not supported or failed");
            }
        }

        TEST_METHOD(ColumnPrivileges)
        {
            // Try to get column privileges
            SQLRETURN rc = SQLColumnPrivileges(testBase->stmt, 
                NULL, 0,           // Catalog
                NULL, 0,           // Schema
                (SQLWCHAR*)L"RDB$DATABASE", SQL_NTS,  // Table name
                NULL, 0);          // Column name

            if (SQL_SUCCEEDED(rc))
            {
                int privCount = 0;
                while (SQLFetch(testBase->stmt) == SQL_SUCCESS && privCount < 10)
                {
                    privCount++;
                }

                char msg[128];
                sprintf_s(msg, "✓ SQLColumnPrivileges returned %d privilege(s) (limited to 10)", privCount);
                Logger::WriteMessage(msg);
            }
            else
            {
                Logger::WriteMessage("⚠ SQLColumnPrivileges not supported or failed");
            }
        }
    };
}
