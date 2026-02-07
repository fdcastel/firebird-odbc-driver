cmake_minimum_required(VERSION 3.15)

project(firebird-odbc-driver VERSION 3.0.0 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_TESTING "Build tests" ON)
option(BUILD_SETUP "Build setup/configuration library" OFF)

# Platform-specific settings
if(WIN32)
    add_definitions(-DWIN32 -D_WINDOWS -D_USRDLL)
    if(MSVC)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
        # Use static runtime for static builds
        if(NOT BUILD_SHARED_LIBS)
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        endif()
    endif()
else()
    # Linux/Unix
    add_definitions(-DUNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Find required packages
if(WIN32)
    # On Windows, ODBC is typically part of the system
    set(ODBC_FOUND TRUE)
else()
    # On Unix-like systems, find ODBC (unixODBC)
    find_package(ODBC REQUIRED)
    if(ODBC_FOUND)
        include_directories(${ODBC_INCLUDE_DIRS})
    endif()
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/IscDbc
    ${CMAKE_CURRENT_SOURCE_DIR}/FBClient.Headers
    ${CMAKE_CURRENT_SOURCE_DIR}/Headers
)

# Add subdirectories
add_subdirectory(IscDbc)

# Main ODBC driver library sources
set(ODBCJDBC_SOURCES
    ConnectDialog.cpp
    DescRecord.cpp
    Main.cpp
    MainUnicode.cpp
    MbsAndWcs.cpp
    OdbcConnection.cpp
    OdbcConvert.cpp
    OdbcDateTime.cpp
    OdbcDesc.cpp
    OdbcEnv.cpp
    OdbcError.cpp
    OdbcObject.cpp
    OdbcStatement.cpp
    Utf16Convert.cpp
)

# Add Windows-specific transaction support files if ATL is available
# These require Visual Studio with ATL installed
if(WIN32 AND MSVC)
    # Try to find if ATL is available (optional)
    include(CheckIncludeFileCXX)
    check_include_file_cxx("atlbase.h" HAVE_ATL)
    
    if(HAVE_ATL)
        list(APPEND ODBCJDBC_SOURCES
            ResourceManagerSink.cpp
            SafeEnvThread.cpp
            TransactionResourceAsync.cpp
        )
        message(STATUS "ATL found - including Windows transaction support")
    else()
        list(APPEND ODBCJDBC_SOURCES
            AtlStubs.cpp
        )
        message(STATUS "ATL not found - building without Windows transaction support")
    endif()
endif()

set(ODBCJDBC_HEADERS
    ConnectDialog.h
    DescRecord.h
    InfoItems.h
    Main.h
    OdbcConnection.h
    OdbcConvert.h
    OdbcDateTime.h
    OdbcDesc.h
    OdbcEnv.h
    OdbcError.h
    OdbcJdbc.h
    OdbcObject.h
    OdbcStatement.h
    ResourceManagerSink.h
    SafeEnvThread.h
    SecurityPassword.h
    SetupAttributes.h
    TemplateConvert.h
    TransactionResourceAsync.h
    Utf16Convert.h
    WriteBuildNo.h
    resource.h
)

# Create the main ODBC driver library
add_library(OdbcFb ${ODBCJDBC_SOURCES} ${ODBCJDBC_HEADERS})

# Set library output name
if(WIN32)
    set_target_properties(OdbcFb PROPERTIES OUTPUT_NAME "FirebirdODBC")
else()
    set_target_properties(OdbcFb PROPERTIES OUTPUT_NAME "OdbcFb")
endif()

# Link with IscDbc library
target_link_libraries(OdbcFb PRIVATE IscDbc)

# Platform-specific linking
if(WIN32)
    target_link_libraries(OdbcFb PRIVATE 
        odbc32 
        odbccp32 
        legacy_stdio_definitions
        Version
    )
    
    # Set the .def file for exports
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        # 64-bit
        set_target_properties(OdbcFb PROPERTIES LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/OdbcJdbc.def")
    else()
        # 32-bit
        set_target_properties(OdbcFb PROPERTIES LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/OdbcJdbc.def")
    endif()
else()
    target_link_libraries(OdbcFb PRIVATE 
        ${ODBC_LIBRARIES}
        odbcinst
        dl
        pthread
    )
endif()

# Setup library (optional)
if(BUILD_SETUP AND WIN32)
    add_subdirectory(OdbcJdbcSetup)
endif()

# Testing
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation rules
install(TARGETS OdbcFb
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install headers (optional, for development)
install(FILES ${ODBCJDBC_HEADERS}
    DESTINATION include/firebird-odbc
)
