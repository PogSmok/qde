# Contributing to QDE

First off, thank you for considering contributing to QDE! It's people like you that make open source such a great
community.

## 1. Getting Started

* Make sure you have a [GitHub account](https://github.com/signup/free).
* Fork the repository on GitHub.
* This project uses **Qt6** and **C++17**. Ensure you have the Qt6 SDK and the appropriate compiler (e.g., GCC, Clang,
  or MSVC) installed before building.
* It's recommended to configure clang-tidy and clang-format tools (use .clang-tidy and .clang-format configs present in
  the repository) as project follows and
  enforces [Google C++ Styleguide](https://google.github.io/styleguide/cppguide.html)
* To build OpenQASM parser and lexer you will need to download ANTL4 java target
  from https://www.antlr.org/download.html (look for Complete ANTLR 4.13.2 Java binaries jar). The .jar file location
  should be passed to build tools via `ANTLR4_JAR_LOCATION` environment variable (for example inside user's
  CMakePresets.json)

## 2. Making Changes

* Create a feature branch from the `develop` branch (`git checkout -b feature/my-new-feature`).
* Write clear, concise commit messages.
* Ensure your code matches the existing style and formatting (Project follows and
  enforces [Google C++ Styleguide](https://google.github.io/styleguide/cppguide.html)).
* Test your changes locally to ensure they do not break existing functionality.
* If your code adds new functionality remember to write appropriate
  tests ([GTests](https://google.github.io/googletest/)).

## 3. Submitting a Pull Request

* Push your changes to your fork.
* Open a Pull Request (PR) against our `develop` branch.
* Reference any related issues in your PR description (e.g., "Fixes #123").
* A maintainer will review your code. We may request changes before merging.

## 4. License

By contributing, you agree that your contributions will be licensed under its GNU General Public License (GPL) v3.