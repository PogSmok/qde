set(TEST_SOURCES
    test_syntax_error_listener.cpp
)

if(TEST_SOURCES)
    add_executable(qde_tests ${TEST_SOURCES})

    target_include_directories(qde_tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    target_link_libraries(qde_tests
        PRIVATE
            qasm3_parser
            GTest::gtest_main
    )

    if(MINGW)
        target_link_options(qde_tests PRIVATE -static-libgcc -static-libstdc++ -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive)
    endif()

    include(GoogleTest)
    gtest_discover_tests(qde_tests
        DISCOVERY_TIMEOUT 60
        DISCOVERY_MODE PRE_TEST
    )
endif()
