# Security Policy

## Supported Versions

QDE is in early development (0.x). Security fixes are made on the `develop` branch and shipped in the next release;
older releases do not receive backports.

| Version        | Supported |
|----------------|-----------|
| `develop`      | Yes       |
| latest release | Yes       |
| older releases | No        |

## Reporting a Vulnerability

**Please do not report security vulnerabilities through public GitHub issues.**

We use GitHub's Private Vulnerability Reporting. If you discover a security vulnerability, please report it privately by
clicking the **Security** tab in this repository, then clicking **Advisories**, and finally clicking **Report a
vulnerability**.

Please include:

* the affected version or commit,
* a description of the issue and its impact,
* steps to reproduce, ideally with a minimal OpenQASM file or other input that triggers it.

## What to Expect

* We will confirm the issue, work on a fix privately with you, and agree on a disclosure date.
* Once a fix is released, we publish a GitHub Security Advisory and credit the reporter unless they prefer otherwise.

## Scope

QDE is a desktop application that reads OpenQASM source files and its own JSON configuration files. It does not open
network connections at runtime. Issues of particular interest include:

* crashes, hangs or memory-safety bugs triggered by a crafted OpenQASM file or configuration file,
* unsafe handling of file paths when opening or saving files,
* vulnerabilities in the build or CI pipeline (e.g. dependency or workflow tampering).

Bugs in third-party dependencies (Qt, ANTLR, GoogleTest) should be reported upstream; let us know as well if QDE is
affected so we can update the dependency.