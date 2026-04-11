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

# QDE QT Library
set(LIB_QT_SOURCES
    qt/main_window.cpp
    qt/app_controller.cpp
    qt/quantum_circuit_view.cpp
    qt/text_editor.cpp
    # explicitly list Q_OBJECT headers:
    ${PROJECT_SOURCE_DIR}/include/qde/qt/app_controller.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/qt/main_window.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/qt/quantum_circuit_view.hpp
    ${PROJECT_SOURCE_DIR}/include/qde/qt/text_editor.hpp
)

add_library(qde_qt STATIC ${LIB_QT_SOURCES})
set_target_properties(qde_qt PROPERTIES AUTOMOC ON AUTORCC ON AUTOUIC ON)

target_include_directories(qde_qt
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(qde_qt
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
        qde_qt
)
