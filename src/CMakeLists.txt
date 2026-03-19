file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS "*.cpp")
list(REMOVE_ITEM LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

add_library(qde_lib STATIC ${LIB_SOURCES})

target_include_directories(qde_lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(qde_lib
    PUBLIC
        Qt6::Core
        Qt6::Widgets
)

qt_add_executable(qde main.cpp)

target_link_libraries(qde
    PRIVATE
        qde_lib
)