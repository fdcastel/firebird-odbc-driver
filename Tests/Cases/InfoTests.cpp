#include "pch.h"
#include "../Fixtures/TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace OdbcTests
{
    TEST_CLASS(InfoTests)
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

        TEST_METHOD(DriverOdbcVersion)
        {
            SQLCHAR version[32];
            SQLSMALLINT length;
            SQLRETURN rc = SQLGetInfo(testBase->dbc, SQL_DRIVER_ODBC_VER, version, sizeof(version), &length);

            testBase->AssertSuccessOrInfo(rc, L"SQLGetInfo(SQL_DRIVER_ODBC_VER) failed");
            Assert::IsTrue(length > 0, L"Version string is empty");

            std::string msg = "Driver ODBC Version: ";
            msg += (char*)version;
            Logger::WriteMessage(msg.c_str());

            // Should be at least "03.00"
            Assert::IsTrue(strcmp((char*)version, "03.00") >= 0, L"Driver should support ODBC 3.0 or higher");
        }

        TEST_METHOD(DbmsNameAndVersion)
        {
            SQLCHAR name[128];
            SQLRETURN rc = SQLGetInfo(testBase->dbc, SQL_DBMS_NAME, name, sizeof(name), nullptr);
            testBase->AssertSuccessOrInfo(rc, L"SQL_DBMS_NAME failed");

            std::string dbmsName = (char*)name;
            Logger::WriteMessage(("DBMS Name: " + dbmsName).c_str());

            Assert::IsTrue(dbmsName.find("Firebird") != std::string::npos ||
                          dbmsName.find("InterBase") != std::string::npos,
                L"DBMS name should contain 'Firebird' or 'InterBase'");

            // Get version
            SQLCHAR version[128];
            rc = SQLGetInfo(testBase->dbc, SQL_DBMS_VER, version, sizeof(version), nullptr);
            testBase->AssertSuccessOrInfo(rc, L"SQL_DBMS_VER failed");

            Logger::WriteMessage(("DBMS Version: " + std::string((char*)version)).c_str());
        }

        TEST_METHOD(DriverName)
        {
            SQLCHAR name[256];
            SQLSMALLINT length;
            SQLRETURN rc = SQLGetInfo(testBase->dbc, SQL_DRIVER_NAME, name, sizeof(name), &length);

            testBase->AssertSuccessOrInfo(rc, L"SQL_DRIVER_NAME failed");
            Assert::IsTrue(length > 0, L"Driver name is empty");

            std::string driverName = (char*)name;
            Logger::WriteMessage(("Driver Name: " + driverName).c_str());

            Assert::IsTrue(driverName.find("Firebird") != std::string::npos ||
                          driverName.find("ODBC") != std::string::npos,
                L"Driver name should contain 'Firebird' or 'ODBC'");
        }

        TEST_METHOD(SqlConformance)
        {
            SQLUINTEGER conformance;
            SQLRETURN rc = SQLGetInfo(testBase->dbc, SQL_SQL_CONFORMANCE, &conformance, sizeof(conformance), nullptr);

            testBase->AssertSuccessOrInfo(rc, L"SQL_SQL_CONFORMANCE failed");

            const char* level = "Unknown";
            switch (conformance)
            {
            case SQL_SC_SQL92_ENTRY: level = "SQL-92 Entry"; break;
            case SQL_SC_FIPS127_2_TRANSITIONAL: level = "FIPS 127-2 Transitional"; break;
            case SQL_SC_SQL92_FULL: level = "SQL-92 Full"; break;
            case SQL_SC_SQL92_INTERMEDIATE: level = "SQL-92 Intermediate"; break;
            }

            std::string msg = "SQL Conformance Level: ";
            msg += level;
            Logger::WriteMessage(msg.c_str());
        }

        TEST_METHOD(IdentifierQuoteChar)
        {
            SQLCHAR quoteChar[8];
            SQLSMALLINT length;
            SQLRETURN rc = SQLGetInfo(testBase->dbc, SQL_IDENTIFIER_QUOTE_CHAR, quoteChar, sizeof(quoteChar), &length);

            testBase->AssertSuccessOrInfo(rc, L"SQL_IDENTIFIER_QUOTE_CHAR failed");

            std::string msg = "Identifier Quote Character: [";
            msg += (char*)quoteChar;
            msg += "]";
            Logger::WriteMessage(msg.c_str());

            // Firebird typically uses double quotes
            Assert::IsTrue(length > 0, L"Quote character should not be empty");
        }

        TEST_METHOD(MaxConnections)
        {
            SQLUSMALLINT maxConn;
            SQLRETURN rc = SQLGetInfo(testBase->dbc, SQL_MAX_DRIVER_CONNECTIONS, &maxConn, sizeof(maxConn), nullptr);

            testBase->AssertSuccessOrInfo(rc, L"SQL_MAX_DRIVER_CONNECTIONS failed");

            char msg[64];
            sprintf_s(msg, "Max Driver Connections: %d", (int)maxConn);
            Logger::WriteMessage(msg);
        }

        TEST_METHOD(NullInfoValuePtr)
        {
            // Call SQLGetInfo with NULL InfoValuePtr to get required buffer length
            SQLSMALLINT length;
            SQLRETURN rc = SQLGetInfo(testBase->dbc, SQL_DRIVER_NAME, nullptr, 0, &length);

            // Should return SQL_SUCCESS and provide the required length
            testBase->AssertSuccessOrInfo(rc, L"SQLGetInfo with NULL pointer failed");
            Assert::IsTrue(length > 0, L"Length should be provided for NULL InfoValuePtr");

            char msg[64];
            sprintf_s(msg, "Driver name requires %d bytes", (int)length);
            Logger::WriteMessage(msg);
        }
    };
}
