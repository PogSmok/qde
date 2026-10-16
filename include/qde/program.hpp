#ifndef PROGRAM_HPP_
#define PROGRAM_HPP_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "qde/circuit.hpp"
#include "qde/gate_definition.hpp"

namespace qde {

// ---------------------------------------------------------------------------
// A Circuit is a fixed list of operations.
// A Program is an OpenQASM program that is run instruction by instruction,
// against a QuantumBackend that hold the quantum state and answers its
// measurements.
//
// This allows to simulate circuit with runtime-dependent control flow.
// ---------------------------------------------------------------------------

class QuantumBackend {
 public:
  virtual ~QuantumBackend() = default;

  // Appends `count` qubits in the |0> state.
  virtual void AppendQubits(std::size_t count) = 0;

  virtual void ApplyGate(const GateDefinition& gate,
                         const std::vector<double>& params,
                         const std::vector<std::size_t>& qubits) = 0;

  virtual bool Measure(std::size_t qubit) = 0;

  virtual void Reset(std::size_t qubit) = 0;
};

struct ProgramError {
  std::size_t line;    // 1-indexed
  std::size_t column;  // 0-indexed
  std::string message;
};

struct ProgramResult {
  // Empty if the run completed.
  std::vector<ProgramError> errors;

  // The classical registers of the program and their final values per register.
  std::vector<BitRegister> bit_registers;
  std::vector<std::uint8_t> classical_bits;
};

class Program {
 public:
  virtual ~Program() = default;

  // Safe to call concurrently.
  [[nodiscard]] virtual ProgramResult Run(QuantumBackend& backend) const = 0;
};

}  // namespace qde

#endif  // PROGRAM_HPP_
