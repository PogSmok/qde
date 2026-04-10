cmake_minimum_required(VERSION 3.21)

# CMP0135: use download timestamp for FetchContent archives, not extraction tim
set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)

project(qde
    VERSION 0.1.0
    DESCRIPTION "Quantum Development Environment"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

include(FetchContent)

# ANTLR C++ runtime
FetchContent_Declare(
    antlr4_runtime
    URL      https://www.antlr.org/download/antlr4-cpp-runtime-4.13.2-source.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# GoogleTest
set(gtest_force_shared_crt ON  CACHE BOOL "" FORCE) # avoids CRT mismatch on MSVC
set(BUILD_GMOCK            OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST          OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0
)

set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "" FORCE) # disable ANTLR4 internal tests from polluting CTest
FetchContent_MakeAvailable(antlr4_runtime googletest)

add_subdirectory(grammar)
add_subdirectory(src)

if(PROJECT_IS_TOP_LEVEL)
    enable_testing()
    add_subdirectory(tests)
endif()
