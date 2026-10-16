include(GoogleTest)

function(qde_link_static_mingw_runtime target)
    if(MINGW)
        target_link_options(${target} PRIVATE
            -static-libgcc -static-libstdc++
            -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive
        )
    endif()
endfunction()

# -------------------------
# Core and simulator tests
# -------------------------
add_executable(qde_tests
    test_gate_definition.cpp
    test_gate_registry.cpp
    test_parse_result.cpp
    test_parser.cpp
    test_qubit_allocator.cpp
    test_syntax_error_listener.cpp
    simulator/test_eigenvalues.cpp
    simulator/test_memory_guard.cpp
    simulator/test_quantum_state.cpp
    simulator/test_simulation_circuit.cpp
    simulator/test_simulator.cpp
)

target_link_libraries(qde_tests
    PRIVATE
        qde_lib
        qde_sim
        qasm3_parser
        GTest::gtest_main
)

qde_enable_warnings(qde_tests)
qde_link_static_mingw_runtime(qde_tests)

gtest_discover_tests(qde_tests
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# -------------------------
# GUI tests
# -------------------------
qt_add_executable(qde_gui_tests
    gui/test_main.cpp
    gui/test_app_controller.cpp
    gui/test_code_editor.cpp
    gui/test_config.cpp
    gui/test_config_key.cpp
    gui/test_config_manager.cpp
    gui/test_main_window.cpp
    gui/test_quantum_circuit_view.cpp
    gui/test_simulation_results_view.cpp
    gui/test_syntax_highlighter.cpp
    gui/test_text_document.cpp
    gui/test_text_editor.cpp
    gui/test_token_patterns.cpp
    gui/test_view_helpers.cpp
)
set_target_properties(qde_gui_tests PROPERTIES AUTOMOC ON)

target_link_libraries(qde_gui_tests
    PRIVATE
        qde_gui
        qde_sim
        Qt6::Test
        Qt6::Gui
        Qt6::Widgets
        GTest::gtest
)

qde_enable_warnings(qde_gui_tests)
qde_link_static_mingw_runtime(qde_gui_tests)

if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${Qt6_DIR}/../../../bin" "${Qt6_DIR}/bin" "$ENV{QT_DIR}/bin")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET qde_gui_tests POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-compiler-runtime --no-translations --dir "$<TARGET_FILE_DIR:qde_gui_tests>" "$<TARGET_FILE:qde_gui_tests>"
            COMMENT "Running windeployqt for qde_gui_tests..."
        )
    endif()
endif()

gtest_discover_tests(qde_gui_tests
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)
