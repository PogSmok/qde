set(TEST_SOURCES
    # Add test source files here explicitly, e.g.:
    # test_gate_type.cpp
    # test_parser.cpp
)

if(TEST_SOURCES)
    add_executable(qde_tests ${TEST_SOURCES})

    target_include_directories(qde_tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    target_link_libraries(qde_tests
        PRIVATE
            qde_lib
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(qde_tests
        DISCOVERY_TIMEOUT 60
    )
endif()
