# IscDbc library CMakeLists.txt

set(ISCDBC_SOURCES
    Attachment.cpp
    BinaryBlob.cpp
    Blob.cpp
    DateTime.cpp
    EnvShare.cpp
    extodbc.cpp
    IscArray.cpp
    IscBlob.cpp
    IscCallableStatement.cpp
    IscColumnKeyInfo.cpp
    IscColumnPrivilegesResultSet.cpp
    IscColumnsResultSet.cpp
    IscConnection.cpp
    IscCrossReferenceResultSet.cpp
    IscDatabaseMetaData.cpp
    IscIndexInfoResultSet.cpp
    IscMetaDataResultSet.cpp
    IscOdbcStatement.cpp
    IscPreparedStatement.cpp
    IscPrimaryKeysResultSet.cpp
    IscProcedureColumnsResultSet.cpp
    IscProceduresResultSet.cpp
    IscResultSet.cpp
    IscResultSetMetaData.cpp
    IscSpecialColumnsResultSet.cpp
    IscSqlType.cpp
    IscStatement.cpp
    IscStatementMetaData.cpp
    IscTablePrivilegesResultSet.cpp
    IscTablesResultSet.cpp
    IscUserEvents.cpp
    JString.cpp
    LinkedList.cpp
    LoadFbClientDll.cpp
    Lock.cpp
    MultibyteConvert.cpp
    Mutex.cpp
    Parameter.cpp
    ParameterEvent.cpp
    Parameters.cpp
    ParametersEvents.cpp
    ServiceManager.cpp
    Sqlda.cpp
    SQLError.cpp
    SqlTime.cpp
    Stream.cpp
    TimeStamp.cpp
    TypesResultSet.cpp
    Value.cpp
    Values.cpp
)

set(ISCDBC_HEADERS
    Attachment.h
    BinaryBlob.h
    BinToHexStr.h
    Blob.h
    Connection.h
    DateTime.h
    Engine.h
    EnvShare.h
    Error.h
    IscArray.h
    IscBlob.h
    IscCallableStatement.h
    IscColumnKeyInfo.h
    IscColumnPrivilegesResultSet.h
    IscColumnsResultSet.h
    IscConnection.h
    IscCrossReferenceResultSet.h
    IscDatabaseMetaData.h
    IscDbc.h
    IscHeadSqlVar.h
    IscIndexInfoResultSet.h
    IscMetaDataResultSet.h
    IscOdbcStatement.h
    IscPreparedStatement.h
    IscPrimaryKeysResultSet.h
    IscProcedureColumnsResultSet.h
    IscProceduresResultSet.h
    IscResultSet.h
    IscResultSetMetaData.h
    IscSpecialColumnsResultSet.h
    IscSqlType.h
    IscStatement.h
    IscStatementMetaData.h
    IscTablePrivilegesResultSet.h
    IscTablesResultSet.h
    IscUserEvents.h
    JavaType.h
    JString.h
    LinkedList.h
    ListParamTransaction.h
    LoadFbClientDll.h
    Lock.h
    Mlist.h
    MultibyteConvert.h
    Mutex.h
    Parameter.h
    ParameterEvent.h
    Parameters.h
    ParametersEvents.h
    Properties.h
    ServiceManager.h
    Sqlda.h
    SQLError.h
    SQLException.h
    SqlTime.h
    Stream.h
    TimeStamp.h
    Types.h
    TypesResultSet.h
    Value.h
    Values.h
    resource.h
)

# Create IscDbc library
add_library(IscDbc STATIC ${ISCDBC_SOURCES} ${ISCDBC_HEADERS})

# Include directories
target_include_directories(IscDbc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FIREBIRD_INCLUDE_DIR}
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(IscDbc PRIVATE WIN32 _WINDOWS)
    target_link_libraries(IscDbc PRIVATE ws2_32 advapi32)
else()
    target_compile_definitions(IscDbc PRIVATE UNIX)
    target_link_libraries(IscDbc PRIVATE dl pthread)
endif()
