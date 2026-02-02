#pragma once

#include <sql.h>
#include <sqlext.h>
#include <string>

namespace OdbcTests
{
    /// <summary>
    /// Base class for all ODBC tests providing connection management and assertion helpers
    /// </summary>
    class TestBase
    {
    public:
        SQLHENV env;
        SQLHDBC dbc;
        SQLHSTMT stmt;
        std::string connectionString;

        /// <summary>
        /// Initialize test - allocate handles and connect to database
        /// Call this from [TestInitialize] methods
        /// </summary>
        void SetUp();

        /// <summary>
        /// Cleanup test - disconnect and free handles
        /// Call this from [TestCleanup] methods
        /// </summary>
        void TearDown();

        /// <summary>
        /// Assert that a SQL operation returned SQL_SUCCESS
        /// </summary>
        void AssertSuccess(SQLRETURN rc, const wchar_t* message = L"Operation failed");

        /// <summary>
        /// Assert that a SQL operation returned SQL_SUCCESS or SQL_SUCCESS_WITH_INFO
        /// </summary>
        void AssertSuccessOrInfo(SQLRETURN rc, const wchar_t* message = L"Operation failed");

        /// <summary>
        /// Assert that the SQLSTATE matches the expected value
        /// </summary>
        void AssertSqlState(SQLHANDLE handle, SQLSMALLINT handleType, const char* expectedState);

        /// <summary>
        /// Assert that the SQLSTATE starts with the expected prefix
        /// </summary>
        void AssertSqlStateStartsWith(SQLHANDLE handle, SQLSMALLINT handleType, const char* prefix);

        /// <summary>
        /// Get connection string from environment variable
        /// </summary>
        std::string GetConnectionString();

        /// <summary>
        /// Get diagnostic information from a handle
        /// </summary>
        std::string GetDiagnostics(SQLHANDLE handle, SQLSMALLINT handleType);

    public:
        TestBase();
        virtual ~TestBase();
    };
}
