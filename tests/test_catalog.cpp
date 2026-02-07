// tests/test_catalog.cpp — Catalog function tests (SQLTables, SQLColumns, etc.)

#include "test_helpers.h"

class CatalogTest : public OdbcConnectedTest {
protected:
    void SetUp() override {
        OdbcConnectedTest::SetUp();
        if (::testing::Test::IsSkipped()) return;

        table_ = std::make_unique<TempTable>(this, "ODBC_TEST_CATALOG",
            "ID INTEGER NOT NULL PRIMARY KEY, "
            "NAME VARCHAR(50) NOT NULL, "
            "AMOUNT NUMERIC(10,2)"
        );
    }

    void TearDown() override {
        table_.reset();
        OdbcConnectedTest::TearDown();
    }

    std::unique_ptr<TempTable> table_;
};

TEST_F(CatalogTest, SQLTablesFindsTestTable) {
    SQLRETURN ret = SQLTables(hStmt,
        NULL, 0,           // catalog
        NULL, 0,           // schema
        (SQLCHAR*)"ODBC_TEST_CATALOG", SQL_NTS,   // table name
        (SQLCHAR*)"TABLE", SQL_NTS);               // table type
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLTables failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Should find at least one row
    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "Table not found in catalog";

    // Get the table name from column 3 (TABLE_NAME)
    SQLCHAR tableName[128] = {};
    SQLLEN ind = 0;
    ret = SQLGetData(hStmt, 3, SQL_C_CHAR, tableName, sizeof(tableName), &ind);
    ASSERT_TRUE(SQL_SUCCEEDED(ret));
    EXPECT_STREQ((char*)tableName, "ODBC_TEST_CATALOG");
}

TEST_F(CatalogTest, SQLColumnsReturnsCorrectTypes) {
    SQLRETURN ret = SQLColumns(hStmt,
        NULL, 0,           // catalog
        NULL, 0,           // schema
        (SQLCHAR*)"ODBC_TEST_CATALOG", SQL_NTS,   // table
        (SQLCHAR*)"%", SQL_NTS);                   // all columns
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLColumns failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    int columnCount = 0;
    bool foundId = false, foundName = false, foundAmount = false;

    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        columnCount++;

        SQLCHAR colName[128] = {};
        SQLLEN colNameInd = 0;
        SQLGetData(hStmt, 4, SQL_C_CHAR, colName, sizeof(colName), &colNameInd);  // COLUMN_NAME

        SQLSMALLINT dataType = 0;
        SQLLEN dataTypeInd = 0;
        SQLGetData(hStmt, 5, SQL_C_SSHORT, &dataType, 0, &dataTypeInd);  // DATA_TYPE

        SQLSMALLINT nullable = 0;
        SQLLEN nullableInd = 0;
        SQLGetData(hStmt, 11, SQL_C_SSHORT, &nullable, 0, &nullableInd);  // NULLABLE

        if (strcmp((char*)colName, "ID") == 0) {
            foundId = true;
            EXPECT_EQ(dataType, SQL_INTEGER);
            EXPECT_EQ(nullable, SQL_NO_NULLS);
        } else if (strcmp((char*)colName, "NAME") == 0) {
            foundName = true;
            EXPECT_TRUE(dataType == SQL_VARCHAR || dataType == SQL_WVARCHAR);
            EXPECT_EQ(nullable, SQL_NO_NULLS);
        } else if (strcmp((char*)colName, "AMOUNT") == 0) {
            foundAmount = true;
            EXPECT_TRUE(dataType == SQL_NUMERIC || dataType == SQL_DECIMAL);
            EXPECT_EQ(nullable, SQL_NULLABLE);
        }
    }

    EXPECT_EQ(columnCount, 3);
    EXPECT_TRUE(foundId) << "Column ID not found";
    EXPECT_TRUE(foundName) << "Column NAME not found";
    EXPECT_TRUE(foundAmount) << "Column AMOUNT not found";
}

TEST_F(CatalogTest, SQLPrimaryKeys) {
    SQLRETURN ret = SQLPrimaryKeys(hStmt,
        NULL, 0,
        NULL, 0,
        (SQLCHAR*)"ODBC_TEST_CATALOG", SQL_NTS);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLPrimaryKeys failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    ret = SQLFetch(hStmt);
    ASSERT_TRUE(SQL_SUCCEEDED(ret)) << "No primary key found";

    SQLCHAR colName[128] = {};
    SQLLEN ind = 0;
    SQLGetData(hStmt, 4, SQL_C_CHAR, colName, sizeof(colName), &ind);  // COLUMN_NAME
    EXPECT_STREQ((char*)colName, "ID");
}

TEST_F(CatalogTest, SQLGetTypeInfo) {
    SQLRETURN ret = SQLGetTypeInfo(hStmt, SQL_ALL_TYPES);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLGetTypeInfo failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    int typeCount = 0;
    bool foundInteger = false, foundVarchar = false;

    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        typeCount++;

        SQLCHAR typeName[128] = {};
        SQLLEN typeNameInd = 0;
        SQLGetData(hStmt, 1, SQL_C_CHAR, typeName, sizeof(typeName), &typeNameInd);

        SQLSMALLINT dataType = 0;
        SQLLEN dataTypeInd = 0;
        SQLGetData(hStmt, 2, SQL_C_SSHORT, &dataType, 0, &dataTypeInd);

        if (dataType == SQL_INTEGER) foundInteger = true;
        if (dataType == SQL_VARCHAR) foundVarchar = true;
    }

    EXPECT_GT(typeCount, 5) << "Expected at least 5 type entries";
    EXPECT_TRUE(foundInteger) << "INTEGER type not reported";
    EXPECT_TRUE(foundVarchar) << "VARCHAR type not reported";
}

TEST_F(CatalogTest, SQLStatistics) {
    SQLRETURN ret = SQLStatistics(hStmt,
        NULL, 0,
        NULL, 0,
        (SQLCHAR*)"ODBC_TEST_CATALOG", SQL_NTS,
        SQL_INDEX_ALL, SQL_QUICK);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLStatistics failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Should find at least the PK index
    int indexCount = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        indexCount++;
    }
    EXPECT_GE(indexCount, 1) << "Expected at least one index (PK)";
}

TEST_F(CatalogTest, SQLSpecialColumns) {
    SQLRETURN ret = SQLSpecialColumns(hStmt, SQL_BEST_ROWID,
        NULL, 0,
        NULL, 0,
        (SQLCHAR*)"ODBC_TEST_CATALOG", SQL_NTS,
        SQL_SCOPE_SESSION, SQL_NULLABLE);
    ASSERT_TRUE(SQL_SUCCEEDED(ret))
        << "SQLSpecialColumns failed: " << GetOdbcError(SQL_HANDLE_STMT, hStmt);

    // Primary key column should be returned as best row identifier
    int found = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        found++;
    }
    EXPECT_GE(found, 1);
}
