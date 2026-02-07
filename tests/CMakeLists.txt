# Tests CMakeLists.txt

include(FetchContent)

# Fetch Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Enable CTest
include(GoogleTest)

# Test executable
add_executable(firebird_odbc_tests
    test_connection.cpp
    test_main.cpp
    test_null_handles.cpp
)

# Link with Google Test and the ODBC library
target_link_libraries(firebird_odbc_tests
    PRIVATE
        gtest
        gtest_main
        OdbcFb
        IscDbc
)

# Include directories
target_include_directories(firebird_odbc_tests PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/IscDbc
    ${CMAKE_SOURCE_DIR}/Headers
)

# Platform-specific settings
if(WIN32)
    target_link_libraries(firebird_odbc_tests PRIVATE
        odbc32
        odbccp32
    )
else()
    target_link_libraries(firebird_odbc_tests PRIVATE
        ${ODBC_LIBRARIES}
    )
endif()

# Use simple add_test instead of gtest_discover_tests to avoid build-time test discovery
add_test(NAME firebird_odbc_tests COMMAND firebird_odbc_tests)

# Ensure the test can find DLLs (GTest, driver) at runtime
if(WIN32)
    # Add the GTest DLL directory and the driver DLL directory to PATH for CTest
    set_tests_properties(firebird_odbc_tests PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:gtest>;$<TARGET_FILE_DIR:OdbcFb>;$ENV{PATH}"
    )
endif()

# Copy the driver DLL next to the test executable so LoadLibrary finds it
add_custom_command(TARGET firebird_odbc_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:OdbcFb>
        $<TARGET_FILE_DIR:firebird_odbc_tests>
    COMMENT "Copying driver DLL next to test executable"
)
