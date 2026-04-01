if(DEFINED ANTLR4_JAR_LOCATION)
    find_package(Java COMPONENTS Runtime REQUIRED)
    set(ANTLR4_COMMAND ${Java_JAVA_EXECUTABLE} -jar ${ANTLR4_JAR_LOCATION})
else()
    find_program(ANTLR4_EXECUTABLE antlr4 REQUIRED
        DOC "ANTLR4 tool for generating parser sources"
    )
    set(ANTLR4_COMMAND ${ANTLR4_EXECUTABLE})
endif()

set(ANTLR_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${ANTLR_GEN_DIR})

set(ANTLR_SOURCES
    ${ANTLR_GEN_DIR}/qasm3Lexer.cpp
    ${ANTLR_GEN_DIR}/qasm3Parser.cpp
)

set(ANTLR_OUTPUTS
    ${ANTLR_SOURCES}
    ${ANTLR_GEN_DIR}/qasm3Lexer.h
    ${ANTLR_GEN_DIR}/qasm3Parser.h
    ${ANTLR_GEN_DIR}/qasm3BaseVisitor.h
    ${ANTLR_GEN_DIR}/qasm3Visitor.h
)

add_custom_command(
    OUTPUT ${ANTLR_OUTPUTS}
    COMMAND ${ANTLR4_COMMAND}
            -Dlanguage=Cpp -visitor -no-listener
            -o ${ANTLR_GEN_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/qasm3Lexer.g4
            ${CMAKE_CURRENT_SOURCE_DIR}/qasm3Parser.g4
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/qasm3Lexer.g4
        ${CMAKE_CURRENT_SOURCE_DIR}/qasm3Parser.g4
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating OpenQASM 3.0 C++ parser"
    VERBATIM
)

add_library(qasm3_parser STATIC ${ANTLR_SOURCES})
target_link_libraries(qasm3_parser PUBLIC antlr4_static)

target_include_directories(qasm3_parser PUBLIC  ${ANTLR_GEN_DIR})
target_include_directories(qasm3_parser PRIVATE ${antlr4_runtime_SOURCE_DIR}/runtime/src)
