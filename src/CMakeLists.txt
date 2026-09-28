# -------------------------
# QDE core library: parser and circuit model
# -------------------------
add_library(qde_lib STATIC
    gate_registry.cpp
    parser.cpp
    qubit_allocator.cpp
)

target_include_directories(qde_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(qde_lib
    PRIVATE
        qasm3_parser
)

qde_enable_warnings(qde_lib)

# -------------------------
# QDE simulation library
# -------------------------
find_package(OpenMP REQUIRED)

add_library(qde_sim STATIC
    simulator/simulation_circuit.cpp
    simulator/simulator.cpp
)

target_link_libraries(qde_sim
    PUBLIC
        qde_lib
    PRIVATE
        OpenMP::OpenMP_CXX
)

# /openmp:llvm upgrades MSVC from OpenMP 2.0 to 3.1, enabling collapse clause.
if(MSVC)
    target_compile_options(qde_sim PRIVATE /openmp:llvm)
endif()

qde_enable_warnings(qde_sim)

# -------------------------
# QDE GUI library
# -------------------------
add_library(qde_gui STATIC
    gui/app_controller.cpp
    gui/code_editor.cpp
    gui/config_manager.cpp
    gui/main_window.cpp
    gui/quantum_circuit_view.cpp
    gui/syntax_highlighter.cpp
    gui/text_document.cpp
    gui/text_editor.cpp
    # Q_OBJECT headers live outside this directory, list explicitly
    ${PROJECT_SOURCE_DIR}/include/qde/gui/app_controller.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/gui/code_editor.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/gui/config_manager.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/gui/main_window.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/gui/quantum_circuit_view.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/gui/syntax_highlighter.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/gui/text_document.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/gui/text_editor.hpp
)

set_target_properties(qde_gui PROPERTIES AUTOMOC ON AUTORCC ON AUTOUIC ON)

target_link_libraries(qde_gui
    PUBLIC
        qde_lib
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

qde_enable_warnings(qde_gui)

# -------------------------
# QDE application
# -------------------------
qt_add_executable(qde main.cpp)

target_link_libraries(qde
    PRIVATE
        qde_lib
        qde_gui
        qde_sim
)

qde_enable_warnings(qde)

if(WIN32)
    # Use windeployqt to copy the Qt runtime next to qde.exe, producing a
    # self-contained Windows application directory.
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(NOT WINDEPLOYQT_EXECUTABLE)
        message(FATAL_ERROR "windeployqt not found.\n"
                "The tool was not found in the Qt installation linked to this "
                "project. Ensure CMAKE_PREFIX_PATH points to a valid Qt "
                "installation whose 'bin' directory contains windeployqt.exe.")
    endif()

    add_custom_command(TARGET qde POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --no-compiler-runtime
                --verbose 0
                "$<TARGET_FILE:qde>"
        COMMENT "Deploying Qt dependencies..."
    )
endif()
