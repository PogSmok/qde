<p align="center">
    <img src="resources/app_logo_1024x512.png" alt="QDE Logo" width="75%" />
</p>

# QDE: Quantum Development Environment

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

[![CI](https://github.com/PogSmok/qde/actions/workflows/ci.yml/badge.svg)](https://github.com/PogSmok/qde/actions/workflows/ci.yml)
[![CodeQL](https://github.com/PogSmok/qde/actions/workflows/codeql.yml/badge.svg)](https://github.com/PogSmok/qde/actions/workflows/codeql.yml)
[![codecov](https://codecov.io/gh/PogSmok/qde/branch/develop/graph/badge.svg)](https://codecov.io/gh/PogSmok/qde)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/12988/badge)](https://www.bestpractices.dev/projects/12988)

QDE is a desktop environment for writing OpenQASM, seeing the circuit, and inspecting the simulated quantum state on
your own machine.

---

## What it is for

Use QDE when you want to check a circuit and its amplitudes without leaving the editor and without sending the program
to a remote backend.

The window is split into three panes: the OpenQASM source on top, the circuit diagram below it, and the simulation
results beside the circuit.

---

## Capabilities

### OpenQASM editor

- **Full OpenQASM 3 Support**. Write and open OpenQASM 3 programs (`.qasm`, `.qasm2`).
- **Live Syntax Validation**. The editor highlights the language and reports when a program
does not parse, so the circuit and the simulation stay tied to a valid source file.

### Circuit view

- Automatically translates valid programs into clear circuit diagrams.
- Display qubit and classical registers, measurements and gates, including user defined gates.

### Simulation results

After a successful parse, QDE runs a local simulation of the circuit and shows the computational-basis state in either
of two views:

- **Probabilities** — the probability of each basis state.
- **Statevector** — amplitudes, with relative phase shown by color.

---

## The interface

<p align="center">
    <img src="docs/images/interface.png" alt="QDE Interface" width="75%"/>
</p>

---

**Editor and parse feedback**

<p align="center">
    <img src="docs/images/qasm_editor_example.png" alt="OpenQASM editor" width="50%"/>
</p>

---

**Circuit diagram**
<p align="center">
    <img src="docs/images/circuit_view_example.png" alt="Circuit diagram of qubits, gates, and measurements" width="50%"/>
</p>

---

**Probabilities and phase**

<p align="center">
    <img src="docs/images/probabilities_view_example.png" alt="Simulation results probabilities" width="40%" />
    <img src="docs/images/statevector_example.png" alt="Simulation results statevector" width="40%"/>
</p>

---

## How a session works

1. **Write or Open**: Open an existing `.qasm` file or write a program in the editor.
2. **Parse & Render**: When the program parses successfully, the circuit diagram updates automatically.
3. **Simulate**: The local simulator runs on parsed circuit instantly.
4. **Inspect**: Analyze results via basis probabilities, or switch to the statevector view to see amplitudes and
   relative phases.

---

## A short example

A Bell pair:

```qasm
OPENQASM 3.0;
qubit[2] q;
h q[0];
cx q[0], q[1];
```

#### Expected results

The probability view should show equal weight on `|00⟩` and `|11⟩`, and none on `|01⟩` or `|10⟩`.
<p align="center">
    <img src="docs/images/bell_example_prob.png" alt="Simulation results probabilities" width="40%" />
</p>

---

## Roadmap & Planned features

- [ ] **Step Simulation**:  Advance the circuit one operation at a time and inspect the state after each step.
- [ ] **Export Options**: Export circuit diagrams (SVG/PNG) and simulation data (JSON/CSV).

---

## For developers

Building from source, tests, and the contribution workflow are documented in [CONTRIBUTING.md](CONTRIBUTING.md).

- [Code of Conduct](CODE_OF_CONDUCT.md)

- [Security policy](SECURITY.md)

---

Distributed under the GNU General Public License v3. See [LICENSE](LICENSE).
