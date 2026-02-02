#include "pch.h"
#include "../Fixtures/TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace OdbcTests
{
    TEST_CLASS(ConnectAttrsTests)
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

        TEST_METHOD(DriverConnectFullString)
        {
            // Connection already established in SetUp
            Assert::IsTrue(testBase->dbc != SQL_NULL_HDBC, L"Connection handle should be valid");

            // Verify we can execute a simple query
            SQLRETURN rc = SQLExecDirect(testBase->stmt, (SQLWCHAR*)L"SELECT 1 FROM RDB$DATABASE", SQL_NTS);
            testBase->AssertSuccessOrInfo(rc, L"Failed to execute simple query");

            Logger::WriteMessage("✓ Connected successfully using connection string");
        }

        TEST_METHOD(AutocommitOnOff)
        {
            SQLRETURN rc;

            // Get current autocommit mode
            SQLINTEGER autocommit;
            rc = SQLGetConnectAttr(testBase->dbc, SQL_ATTR_AUTOCOMMIT, &autocommit, 0, NULL);
            testBase->AssertSuccessOrInfo(rc, L"Failed to get autocommit attribute");

            Logger::WriteMessage(autocommit == SQL_AUTOCOMMIT_ON ? 
                "Current autocommit: ON" : "Current autocommit: OFF");

            // Try to toggle autocommit
            SQLINTEGER newMode = (autocommit == SQL_AUTOCOMMIT_ON) ? 
                SQL_AUTOCOMMIT_OFF : SQL_AUTOCOMMIT_ON;

            rc = SQLSetConnectAttr(testBase->dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)(SQLULEN)newMode, 0);
            testBase->AssertSuccessOrInfo(rc, L"Failed to set autocommit attribute");

            // Verify it changed
            SQLINTEGER verifyMode;
            rc = SQLGetConnectAttr(testBase->dbc, SQL_ATTR_AUTOCOMMIT, &verifyMode, 0, NULL);
            testBase->AssertSuccessOrInfo(rc, L"Failed to verify autocommit attribute");
            
            Assert::AreEqual((int)newMode, (int)verifyMode, L"Autocommit mode not changed");

            Logger::WriteMessage("✓ Autocommit mode toggled successfully");
        }

        TEST_METHOD(ConnectionDeadReadOnly)
        {
            SQLRETURN rc;
            SQLINTEGER dead;

            // SQL_ATTR_CONNECTION_DEAD should be read-only
            rc = SQLGetConnectAttr(testBase->dbc, SQL_ATTR_CONNECTION_DEAD, &dead, 0, NULL);
            testBase->AssertSuccessOrInfo(rc, L"Failed to get SQL_ATTR_CONNECTION_DEAD");

            Assert::AreEqual((int)SQL_CD_FALSE, (int)dead, L"Connection should not be dead");

            Logger::WriteMessage("✓ SQL_ATTR_CONNECTION_DEAD returns SQL_CD_FALSE for active connection");
        }
    };
}
