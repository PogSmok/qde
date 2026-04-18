set(TEST_SOURCES
    test_syntax_error_listener.cpp
    test_parse_result.cpp
    test_parser.cpp
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
            qasm3_parser
            GTest::gtest_main
    )

    if(MINGW)
        target_link_options(qde_tests PRIVATE -static-libgcc -static-libstdc++ -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive)
    endif()

    include(GoogleTest)
    gtest_discover_tests(qde_tests
        DISCOVERY_TIMEOUT 60
        # PRE_TEST: defer discovery to ctest runtime; avoids DLL path issues on Windows during build
        DISCOVERY_MODE PRE_TEST
    )
endif()

set(QT_TEST_SOURCES
    test_main.cpp
    test_text_document.cpp
    test_code_editor.cpp
    test_text_editor.cpp
    test_app_controller.cpp
    test_quantum_circuit_view.cpp
)

if(QT_TEST_SOURCES)
    qt_add_executable(qde_qt_tests ${QT_TEST_SOURCES})
    set_target_properties(qde_qt_tests PROPERTIES AUTOMOC ON)

    target_include_directories(qde_qt_tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    target_link_libraries(qde_qt_tests
        PRIVATE
            qde_qt
            Qt6::Test
            Qt6::Gui
            Qt6::Widgets
            GTest::gtest
    )
    
    if(WIN32)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${Qt6_DIR}/../../../bin" "${Qt6_DIR}/bin" "$ENV{QT_DIR}/bin")
        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET qde_qt_tests POST_BUILD
                COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-compiler-runtime --no-translations --dir "$<TARGET_FILE_DIR:qde_qt_tests>" "$<TARGET_FILE:qde_qt_tests>"
                COMMENT "Running windeployqt for qde_qt_tests..."
            )
        endif()
    endif()

    if(MINGW)
        target_link_options(qde_qt_tests PRIVATE -static-libgcc -static-libstdc++ -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive)
    endif()

    gtest_discover_tests(qde_qt_tests
        DISCOVERY_TIMEOUT 60
        DISCOVERY_MODE PRE_TEST
    )
endif()