set(LIB_SOURCES
    parser.cpp
)

add_library(qde_lib STATIC ${LIB_SOURCES})
set_target_properties(qde_lib PROPERTIES AUTOMOC ON AUTORCC ON AUTOUIC ON)

target_include_directories(qde_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(qde_lib
    PUBLIC
        Qt6::Core
        Qt6::Widgets
    # TODO: move back to PRIVATE once main.cpp no longer needs ANTLR directly
        qasm3_parser
)

qt_add_executable(qde main.cpp)

target_link_libraries(qde
    PRIVATE
        qde_lib
)
