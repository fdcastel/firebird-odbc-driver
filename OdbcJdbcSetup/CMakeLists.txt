# OdbcJdbcSetup library CMakeLists.txt

set(ODBCJDBCSETUP_SOURCES
    CommonUtil.cpp
    DsnDialog.cpp
    OdbcJdbcSetup.cpp
    ServiceClient.cpp
    ServiceTabBackup.cpp
    ServiceTabChild.cpp
    ServiceTabCtrl.cpp
    ServiceTabRepair.cpp
    ServiceTabRestore.cpp
    ServiceTabStatistics.cpp
    ServiceTabUsers.cpp
    Setup.cpp
    UserDialog.cpp
    UsersTabChild.cpp
    UsersTabMemberShips.cpp
    UsersTabRoles.cpp
    UsersTabUsers.cpp
)

set(ODBCJDBCSETUP_HEADERS
    CommonUtil.h
    DsnDialog.h
    OdbcJdbcSetup.h
    ServiceClient.h
    ServiceTabBackup.h
    ServiceTabChild.h
    ServiceTabCtrl.h
    ServiceTabRepair.h
    ServiceTabRestore.h
    ServiceTabStatistics.h
    ServiceTabUsers.h
    Setup.h
    UserDialog.h
    UsersTabChild.h
    UsersTabMemberShips.h
    UsersTabRoles.h
    UsersTabUsers.h
    resource.h
)

# Create OdbcJdbcSetup library
add_library(OdbcJdbcSetup SHARED ${ODBCJDBCSETUP_SOURCES} ${ODBCJDBCSETUP_HEADERS})

# Set library output name
set_target_properties(OdbcJdbcSetup PROPERTIES OUTPUT_NAME "OdbcJdbcS")

# Link with required libraries
target_link_libraries(OdbcJdbcSetup PRIVATE
    IscDbc
    odbc32
    odbccp32
    comctl32
    comdlg32
)

# Set the .def file for exports
set_target_properties(OdbcJdbcSetup PROPERTIES 
    LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/OdbcJdbcSetup.def"
)

# Installation
install(TARGETS OdbcJdbcSetup
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
