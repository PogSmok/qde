set(TEST_SOURCES
    test_syntax_error_listener.cpp
    test_parse_result.cpp
    test_parser.cpp
    test_gate_definition.cpp
    test_gate_registry.cpp
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

set(GUI_TEST_SOURCES
    gui/test_main.cpp
    gui/test_text_document.cpp
    gui/test_code_editor.cpp
    gui/test_text_editor.cpp
    gui/test_app_controller.cpp
    gui/test_quantum_circuit_view.cpp
    gui/test_main_window.cpp
    gui/test_main_window.cpp
)

if(GUI_TEST_SOURCES)
    qt_add_executable(qde_gui_tests ${GUI_TEST_SOURCES})
    set_target_properties(qde_gui_tests PROPERTIES AUTOMOC ON)

    target_include_directories(qde_gui_tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    target_link_libraries(qde_gui_tests
        PRIVATE
            qde_gui
            Qt6::Test
            Qt6::Gui
            Qt6::Widgets
            GTest::gtest
    )
    
    if(WIN32)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${Qt6_DIR}/../../../bin" "${Qt6_DIR}/bin" "$ENV{QT_DIR}/bin")
        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET qde_gui_tests POST_BUILD
                COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-compiler-runtime --no-translations --dir "$<TARGET_FILE_DIR:qde_gui_tests>" "$<TARGET_FILE:qde_gui_tests>"
                COMMENT "Running windeployqt for qde_gui_tests..."
            )
        endif()
    endif()

    if(MINGW)
        target_link_options(qde_gui_tests PRIVATE -static-libgcc -static-libstdc++ -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive)
    endif()

    gtest_discover_tests(qde_gui_tests
        DISCOVERY_TIMEOUT 60
        DISCOVERY_MODE PRE_TEST
    )
endif()