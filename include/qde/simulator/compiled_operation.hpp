#ifndef SIMULATOR_COMPILED_OPERATION_HPP_
#define SIMULATOR_COMPILED_OPERATION_HPP_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "qde/gate_definition.hpp"
#include "qde/operation.hpp"

namespace qde::simulator {

struct CompiledCondition {
  std::vector<std::size_t> bit_indices;

  // The truth table of the condition: truth[i] is its value when the bits
  // read as the binary number i, with bit_indices[0] as the least significant
  // digit.
  //
  // For example, if bit_indices refer to {c[0], c[1]} and
  // truth = {0, 1, 0, 0}, the condition is c[0] AND NOT c[1]
  std::vector<std::uint8_t> truth;
};

// An Operation whose register references have been resolved to flat indices.
struct CompiledOperation {
  OperationType type{OperationType::kGate};
  std::shared_ptr<const GateDefinition> gate;  // non-null if type == Gate
  std::vector<double> gate_params;
  // Flat qubit indices; for a gate, in the order of its matrix's qubits.
  std::vector<std::size_t> qubit_indices;
  // Flat index of the bit receiving the outcome of measuring
  // qubit_indices[i], or the value of assign_sources[i].
  std::vector<std::size_t> measure_target_indices;
  // If set, the operation runs only while the condition holds.
  std::optional<CompiledCondition> condition;
  // For an assignment: the value written to measure_target_indices[i].
  std::vector<CompiledCondition> assign_sources;
};

}  // namespace qde::simulator

#endif  // SIMULATOR_COMPILED_OPERATION_HPP_
