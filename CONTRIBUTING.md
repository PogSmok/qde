# Contributing to QDE

First off, thank you for considering contributing to QDE! It's people like you that make open source such a great
community.

---

## 1. Important Rules and Workflow

### Code of Conduct

By participating in this project, you agree to abide by our [**Code of Conduct**](CODE_OF_CONDUCT.md). Please report any
unacceptable behavior privately to the project maintainers.

### Security Vulnerabilities

**Do not report security vulnerabilities via public GitHub issues.** Please refer to our [**Security Policy
**](SECURITY.md) to report vulnerabilities privately.

### Issue-First Workflow

Before starting work on any non-trivial contribution (new features, major refactors, architectural changes), **you must
first open an issue** to discuss your proposed changes. This prevents duplicated effort and ensures your work aligns
with QDE's goals.

* *Exception:* Trivial changes like fixing typos or fixing minor documentation errors do not require an issue and can be
  submitted directly as a Pull Request.

### How to Report a Bug

If you find a bug in QDE, please submit a formal bug report to help us fix it. Before opening a new issue, please search
the existing issues to ensure the bug hasn't already been reported.

When opening a bug report, please use our [**Bug Report Template**](.github/ISSUE_TEMPLATE/bug_report.md) and provide
the following detailed information:

1. **Clear Summary:** Use a descriptive title prefixed with `[BUG]`, for example:
   `[BUG] Application crashes when loading empty OpenQASM file`.
2. **Steps to Reproduce:** Provide a step-by-step list of exactly what you did to trigger the bug. If applicable,
   include a minimal, self-contained snippet of code or file that reproduces the issue.
3. **Expected vs. Actual Behavior:** Clearly explain what you expected to happen versus what actually happened.
4. **Environment Details:** Since QDE relies on specific system setups, always include:
    * **Operating System:** (e.g., Windows 11, Ubuntu 24.04, macOS Sequoia)
    * **Qt Version:** (e.g., Qt 6.11.0)
    * **Compiler & Build System:** (e.g., MinGW 13.1.0, CMake 3.25)
    * **Build type:** (e.g. release, debug)
    * **ANTLR Version:** (e.g., 4.13.2)
5. **Additional context:** Paste any relevant error logs, terminal stack traces, or screenshots that help illustrate the
   problem.

## How to Request a Feature

We are always looking for ideas to expand QDE's capabilities! If you have a suggestion for a new feature, utility, or
improvement, please submit a feature request issue.

Before opening a new request, please check our existing issues
and [GitHub Discussions](https://github.com/PogSmok/qde/discussions) to see if someone else has already suggested it.

When opening a feature request, please use our [**Feature Request Template**](.github/ISSUE_TEMPLATE/feature_request.md)
and include the following details:

1. **Clear Summary:** Use a descriptive title prefixed with `[FEATURE]`, for example:
   `[FEATURE] Add support for custom gate visualization in the UI`.
2. **Problem Statement:** Describe the core problem you are trying to solve or the limitation you are encountering. (
   e.g., *"I am trying to do X, but the current system makes me do Y instead..."*).
3. **Proposed Solution:** Provide a clear and concise description of the feature or capability you want added. Explain
   how you envision it working within QDE.
4. **Alternatives Considered:** Briefly mention any workarounds or alternative solutions you have tried or considered,
   and why a built-in feature would be better.
5. **Additional Context:** If applicable, include mockups, wireframes, code snippets, or links to relevant open
   standards (like specific OpenQASM specifications) that help explain your idea.

### Asking Questions

If you get stuck during setup, have questions about the codebase, or want to pitch an idea before writing an issue,
please use our [**GitHub Discussions**](https://github.com/PogSmok/qde/discussions) tab instead of opening a bug report.

---

## 2. Getting Started & Prerequisites

To build and contribute to QDE, your development environment must meet the following requirements:

* **GitHub Account:** Ensure you have a [GitHub account](https://github.com/signup/free) and have forked the
  repository. ([Only signed commits will be accepted](https://docs.github.com/en/authentication/managing-commit-signature-verification/signing-commits))
* **C++ Compiler:** A compiler supporting **C++17** (GCC, Clang, or MSVC).
* **Framework:** Qt6 SDK installed.
* **Build System:** CMake version **3.21 or higher**.
* **Java Runtime:** Java Runtime Environment (JRE) **Version 11 or higher** (required by ANTLR4).
* **Linter & Formatter (Mandatory):** `clang-format` version **22** and `clang-tidy` version **22**.
  > **Note:** Code formatting is strictly enforced. Our CI pipeline will automatically fail any Pull Request that does
  not match the rules defined in `.clang-format` and `.clang-tidy` (following
  the [Google C++ Styleguide](https://google.github.io/styleguide/cppguide.html)).

### ANTLR4 Java Target Setup

To build the OpenQASM parser and lexer, you need the complete ANTLR Java binaries:

1. Download the `Complete ANTLR 4.13.2 Java binaries jar` from [antlr.org](https://www.antlr.org/download.html).
2. Save it locally. You will need to provide its path to CMake as shown below.

### Setting up `CMakeUserPresets.json`

To configure your local environment paths without affecting other developers, copy the following template into a file
named `CMakeUserPresets.json` at the root of your project directory (this file is ignored by Git):

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

---

## 3. How to Build, Test, and Format

### Configuring and Building

Once your `CMakeUserPresets.json` is set up, you can configure and build using the following commands:

```bash
# Configure the project using your local preset
cmake --preset local

# Build the project
cmake --build --preset local
```

### Running Tests Locally

We use [GoogleTest \(GTest\)](https://google.github.io/googletest/) for testing. If you write or modify features, you
must verify your changes pass tests locally before pushing:

```bash
# Run the test suite via CTest
ctest --preset local
```

---

## 4. Making Changes & Branching Strategy

We follow a strict Gitflow branching model. All contributions must target the develop branch.

### Naming Conventions

Always base your working branch off `develop` and prefix it based on the work type:

* `feature/` for new capabilities (e.g., `feature/add-cnot-syntax`)
* `fix/` (for forks of `develop`) or `hotfix/` (reserved for fixes on `master` branch) for bug fixes (e.g.,
  `fix/phase-calculation`)
* `release/` for forks of `develop` that are getting ready for merging with `master`, only bug fixes, documentation
  generation and other release-oriented tasks should happen on this branch.

### Commit Messages & Layout

Commits must match our defined [**commit format template**](.gitmessage). To ensure consistency, configure your local
repository to use our Git message template:

```bash
git config commit.template .gitmessage
```

### Before Pushing

* Ensure your code matches the existing style and formatting (Project follows and
  enforces [Google C++ Styleguide](https://google.github.io/styleguide/cppguide.html)).
* Test your changes locally to ensure they do not break existing functionality.
* If your code adds new functionality remember to write appropriate
  tests ([GTests](https://google.github.io/googletest/)).

---

## 5. Submitting a Pull Request

* Push your changes to your fork.
* Open a Pull Request (PR) against our `develop` branch.
* A [default PR template](.github/pull_request_template.md) will automatically populate. You must fill it out
  completely, explaining the why of the change and confirming items on the checklist.
* Reference any related issues in your PR description (e.g., "Fixes #123").

> **NOTE** Before requesting the review, make sure all CI jobs pass.
* A maintainer will review your code. We may request changes before merging.

---

## 6. License

By contributing, you agree that your contributions will be licensed
under [GNU General Public License (GPL) v3](LICENSE).