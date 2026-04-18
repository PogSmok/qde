# QDE Library
set(LIB_SOURCES
    parser.cpp
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
    gui/text_document.cpp
    # explicitly list Q_OBJECT headers:
    ${PROJECT_SOURCE_DIR}/include/qde/gui/app_controller.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/gui/main_window.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/gui/quantum_circuit_view.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/gui/text_editor.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/gui/code_editor.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/gui/text_document.hpp
)

add_library(qde_gui STATIC ${LIB_GUI_SOURCES})
set_target_properties(qde_gui PROPERTIES AUTOMOC ON AUTORCC ON AUTOUIC ON)

target_include_directories(qde_gui
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(qde_gui
    PUBLIC
        qde_lib
        Qt6::Gui
        Qt6::Core
        Qt6::Widgets
)


qt_add_executable(qde main.cpp)

target_link_libraries(qde
    PRIVATE
        qde_lib
        qde_gui
)
