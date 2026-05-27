# QDE: Quantum Development Environment

[![CI](https://github.com/PogSmok/qde/actions/workflows/ci.yml/badge.svg)](https://github.com/PogSmok/qde/actions/workflows/ci.yml)
[![CodeQL](https://github.com/PogSmok/qde/actions/workflows/codeql.yml/badge.svg)](https://github.com/PogSmok/qde/actions/workflows/codeql.yml)
[![codecov](https://codecov.io/gh/PogSmok/qde/branch/develop/graph/badge.svg)](https://codecov.io/gh/PogSmok/qde)


![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg?style=flat&logo=c%2B%2B)
![Qt6](https://img.shields.io/badge/Qt-6.x-41CD52.svg?style=flat&logo=qt)
![CMake](https://img.shields.io/badge/CMake-3.21%2B-064F8C.svg?style=flat&logo=cmake)
![ANTLR4](https://img.shields.io/badge/ANTLR-4.13.2-red.svg?style=flat)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

QDE (Quantum Development Environment) is an open-source, lightweight Integrated Development Environment (IDE) tailored
for quantum computing workflows. Built with C++17 and Qt6, it features an advanced OpenQASM parser and lexer to deliver
a robust environment for writing, analyzing, and simulating quantum circuits.

---

## Features

* **OpenQASM Support:** Full parsing and lexical analysis for OpenQASM source files using an integrated ANTLR4 engine.
* **Modern GUI:** A clean, responsive interface powered by the modern Qt6 framework.
* **Local Verification Engines:** Structural tools designed for quantum grammar parsing and simulation workflows.
* **Developer-Centric Pipeline:** Built-in testing, strict code style enforcement, and cross-platform CMake presets.

---

## Prerequisites

Before installing or building QDE, ensure your local environment contains the following tools:

### Frameworks & Dependencies

* **C++ Compiler:** A compiler supporting **C++17** (e.g., GCC, Clang, or MSVC).
* **Framework:** Qt6 SDK (6.5+ recommended).
* **Build System:** CMake version **3.21 or higher**.
* **Java Runtime:** Java Runtime Environment (JRE) **Version 11 or higher** (required by the ANTLR4 code generation
  tool).
* **Code Formatting:** `clang-format` (v22) and `clang-tidy` (v22) for formatting enforcement.

---

## Getting Started & Local Setup

### 1. Clone the Repository

```bash
git clone https://github.com/PogSmok/qde.git
cd qde
```

### 2. Configure ANTLR4

QDE utilizes ANTLR 4.13.2 to compile OpenQASM grammars.

1. Download the Complete ANTLR 4.13.2 Java binaries jar from [antlr.org](https://www.antlr.org/download.html).
2. Save the `.jar` to a directory of your choosing.

### 3. Set up Your Local Environment Presets

Create a file named `CMakeUserPresets.json` at the root directory of the project. This file handles your unique local
paths and is ignored by Git. Use the following baseline format adjusted to your system pathing:

> **Platform Note on CMake Generators:**
>
> The selection of your build system generator depends completely on your chosen toolchain and OS. On Linux and macOS,
> the generator typically maps to `"Unix Makefiles"` or `"Ninja"`. On Windows using MSVC, it defaults to IDE formats such
> as `"Visual Studio 17 2022"`. Ensure the `"generator"` value matches your specific toolchain environment.

#### Option A: Windows (MinGW toolchain)

```JSON
{
  "version": 3,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 21,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "local",
      "inherits": "base",
      "generator": "MinGW Makefiles",
      "environment": {
        "ANTLR4_TOOLS_ANTLR_VERSION": "4.13.2"
      },
      "cacheVariables": {
        "CMAKE_CXX_COMPILER": "C:/path/to/your/compiler/g++.exe",
        "CMAKE_PREFIX_PATH": "C:/path/to/your/Qt/6.x.x/mingw_64",
        "ANTLR4_JAR_LOCATION": "C:/path/to/your/antlr-4.13.2-complete.jar"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "local",
      "configurePreset": "local",
      "jobs": 2
    }
  ],
  "testPresets": [
    {
      "name": "local",
      "configurePreset": "local",
      "output": {
        "outputOnFailure": true
      },
      "execution": {
        "jobs": 2
      }
    }
  ]
}
```

#### Option B: Cross-Platform (Linux, macOS, or Windows MSVC using Ninja/Makefiles)

```json
{
  "version": 3,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 21,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "local",
      "inherits": "base",
      "generator": "Ninja",
      "environment": {
        "ANTLR4_TOOLS_ANTLR_VERSION": "4.13.2"
      },
      "cacheVariables": {
        "CMAKE_PREFIX_PATH": "/path/to/your/Qt/6.x.x/gcc_64",
        "ANTLR4_JAR_LOCATION": "/path/to/your/antlr-4.13.2-complete.jar"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "local",
      "configurePreset": "local",
      "jobs": 2
    }
  ],
  "testPresets": [
    {
      "name": "local",
      "configurePreset": "local",
      "output": {
        "outputOnFailure": true
      },
      "execution": {
        "jobs": 2
      }
    }
  ]
}
```

### 4. Build the Project

Once the presets file is constructed, execute your CMake sequence:

```Bash
# Configure the project structure
cmake --preset local

# Compile the target binaries
cmake --build --preset local
```

## Running the Test Suite

We use GoogleTest ([GTest]((https://google.github.io/googletest/))) for test automation. To execute test verification
builds locally:

``` Bash
ctest --preset local
```

## Contributing

Contributions are what make the open-source community an amazing place to learn, inspire, and create.

* **Review the guidelines**: Please read our [**CONTRIBUTING.md**](CONTRIBUTING.md) for detailed information regarding
  our Gitflow branching structure, code formatting requirements, and issue-first workflows.
* **Code of Conduct**: We ask all contributors to follow the behavioral rules defined in our [**CODE_OF_CONDUCT.md
  **](CODE_OF_CONDUCT.md).
* **Security**: If you find a security vulnerability, do not open a public issue. Refer to [**SECURITY.md
  **](SECURITY.md) to report it privately.

## License

Distributed under the GNU General Public License v3 (GPLv3). See the [**LICENSE**](LICENSE) file for more details.

Due to dependencies on the Qt6 open-source framework, downstream distributions must comply strictly with the copyleft
obligations mandated by the GPLv3 license agreements.