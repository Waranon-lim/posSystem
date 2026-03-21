# Download GoogleTest automatically (no manual install needed)
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# Prevent GoogleTest from overriding our compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Collect all test .cpp files
file(GLOB TEST_FILES *.cpp)

# Only build the test target if there are test files
if(TEST_FILES)
    add_executable(pos_tests ${TEST_FILES})
    target_link_libraries(pos_tests
        pos_lib
        GTest::gtest_main
    )
    target_include_directories(pos_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
    include(GoogleTest)
    gtest_discover_tests(pos_tests)
endif()