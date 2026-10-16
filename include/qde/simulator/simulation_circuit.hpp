#ifndef SIMULATOR_SIMULATION_CIRCUIT_HPP_
#define SIMULATOR_SIMULATION_CIRCUIT_HPP_

#include <cstddef>
#include <vector>

#include "qde/circuit.hpp"
#include "qde/simulator/compiled_operation.hpp"

namespace qde::simulator {

// Upper bound on simulated qubit count.
// This is irrelevant to limit enforced by memory constraints of host machine
inline constexpr std::size_t kMaxQubits = 32;

// Upper bound on the classical bit count
inline constexpr std::size_t kMaxBits = std::size_t{1} << 16;

class SimulationCircuit {
 public:
  // Operations are scheduled into layers as soon as possible (ASAP) without
  // breaking a dependency of the program order.
  //
  // Throws std::invalid_argument if the circuit has more than kMaxQubits
  // qubits or kMaxBits bits, an operation references a qubit or bit that does
  // not exist, a gate operation has no gate, the wrong number of qubits or a
  // repeated qubit, or a measurement has targets for only some of its qubits.
  explicit SimulationCircuit(const Circuit& circuit);

  [[nodiscard]] std::size_t QubitCount() const noexcept { return qubit_count_; }

  [[nodiscard]] std::size_t BitCount() const noexcept { return bit_count_; }

  // Never empty: a circuit without operations has a single empty layer.
  [[nodiscard]] const std::vector<std::vector<CompiledOperation>>& Layers()
      const noexcept {
    return layers_;
  }

 private:
  std::size_t qubit_count_{0};
  std::size_t bit_count_{0};
  std::vector<std::vector<CompiledOperation>> layers_;
};

}  // namespace qde::simulator

#endif  // SIMULATOR_SIMULATION_CIRCUIT_HPP_
