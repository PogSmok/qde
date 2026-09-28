cmake_minimum_required(VERSION 3.21)

set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)

project(qde
    VERSION 0.1.0
    DESCRIPTION "Quantum Development Environment"
    LANGUAGES CXX
)

# -------------------------
# C++ standard
# -------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -------------------------
# Global compiler options (also applied to fetched dependencies)
# -------------------------
add_compile_options(
    # Exceptions are required by the ANTLR runtime.
    # MSVC and clang-cl (MSVC-compatible frontend)
    $<$<CXX_COMPILER_ID:MSVC>:/EHsc>

    # Clang in GNU mode (Linux/macOS)
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<BOOL:${MSVC}>>>:-fexceptions>

    # GCC
    $<$<CXX_COMPILER_ID:GNU>:-fexceptions>

    # Sources are UTF-8
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

# -------------------------
# Qt
# -------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)

include(FetchContent)

# -------------------------
# GoogleTest
# -------------------------
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG 52eb8108c5bdec04579160ae17225d66034bd723 # v1.17.0
)

# -------------------------
# ANTLR runtime
# -------------------------
set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    antlr4_runtime
    URL https://www.antlr.org/download/antlr4-cpp-runtime-4.13.2-source.zip
    URL_HASH SHA256=0ed13668906e86dbc0dcddf30fdee68c10203dea4e83852b4edb810821bee3c4
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(antlr4_runtime)
unset(BUILD_SHARED_LIBS CACHE)

FetchContent_MakeAvailable(googletest)

# -------------------------
# Isolate third-party targets from clang-tidy and warnings
# -------------------------
foreach(dep antlr4_static antlr4_shared gtest gtest_main)
    if(TARGET ${dep})
        set_target_properties(${dep} PROPERTIES CXX_CLANG_TIDY "")

        # Suppress all compiler warnings inside external dependencies
        target_compile_options(${dep} PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/w>
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-w>
        )

        # Promote existing include paths to SYSTEM includes
        # This prevents warnings from leaking into project build
        get_target_property(_incs ${dep} INTERFACE_INCLUDE_DIRECTORIES)
        if(_incs)
            target_include_directories(${dep} SYSTEM INTERFACE ${_incs})
        endif()
    endif()
endforeach()

# -------------------------
# Patch ANTLR runtime target AFTER it exists
# -------------------------
foreach(_antlr_target antlr4_static antlr4_shared)
    if(TARGET ${_antlr_target})
      # non-GNU compilers do not transitively include <chrono>
      target_compile_options(${_antlr_target} PRIVATE
          $<$<BOOL:${MSVC}>:/FIchrono>
          $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<BOOL:${MSVC}>>>:-include chrono>
      )

      # ANTLR4 defaults to static CRT (/MT), mismatching the dynamic CRT (/MD) used elsewhere
      set_target_properties(${_antlr_target} PROPERTIES
          MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
      )
    endif()
endforeach()

function(qde_enable_warnings target)
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
    )
endfunction()

add_subdirectory(grammar)
add_subdirectory(src)

if(PROJECT_IS_TOP_LEVEL)
    enable_testing()
    add_subdirectory(tests)
endif()
