file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_executable(qde_tests ${TEST_SOURCES})

target_include_directories(qde_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(qde_tests
    PRIVATE
        qde_lib
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(qde_tests)