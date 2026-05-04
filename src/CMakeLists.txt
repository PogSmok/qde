# QDE Library
set(LIB_SOURCES
    gate_registry.cpp
    parser.cpp
    qubit_allocator.cpp
)

add_library(qde_lib STATIC ${LIB_SOURCES})

target_include_directories(qde_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(qde_lib
    PRIVATE
        qasm3_parser
)

# QDE GUI Library
set(LIB_GUI_SOURCES
    gui/main_window.cpp
    gui/app_controller.cpp
    gui/quantum_circuit_view.cpp
    gui/text_editor.cpp
    gui/code_editor.cpp
    gui/syntax_highlighter.cpp
    gui/text_document.cpp
    # explicitly list Q_OBJECT headers:
    ${PROJECT_SOURCE_DIR}/include/qde/gui/app_controller.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/gui/main_window.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/gui/quantum_circuit_view.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/gui/text_editor.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/gui/code_editor.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/gui/text_document.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/gui/syntax_highlighter.hpp
)

add_library(qde_gui STATIC ${LIB_GUI_SOURCES})
set_target_properties(qde_gui PROPERTIES AUTOMOC ON AUTORCC ON AUTOUIC ON)

target_include_directories(qde_gui
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/gui
)

target_link_libraries(qde_gui
    PUBLIC
        qde_lib
        Qt6::Gui
        Qt6::Core
        Qt6::Widgets
)

# QDE Simulation Library
set(LIB_SIM_SOURCES
    simulator/simulation_circuit.cpp
)

add_library(qde_sim STATIC ${LIB_SIM_SOURCES})

target_include_directories(qde_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(qde_sim
    PRIVATE
        qde_lib
)


qt_add_executable(qde main.cpp)

target_link_libraries(qde
    PRIVATE
        qde_lib
        qde_gui
)

if (WIN32)
    # Use windeployqt tool to create self-contained windows application binary
    if (NOT WINDEPLOYQT_EXECUTABLE)
        message(FATAL_ERROR "windeployqt not found.\nThe tool was not found in"
                "the Qt installation directory linked to this project."
                "\nEnsure your CMAKE_PREFIX_PATH points to a valid Qt installation"
                "and that the 'bin' directory contains windeployqt.exe.")
    endif ()

    get_target_property(_qmake_executable Qt::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            --no-compiler-runtime
            --verbose 0
            "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Deploying Qt dependencies..."
    )
elseif (APPLE)
    # macdeployqt logic could go here for macOS users
    message(STATUS "macOS detected: windeployqt skipped.")

else ()
    # Linux typically uses RPATH or AppImage tools
    message(STATUS "Linux/Unix detected: windeployqt skipped.")
endif ()
