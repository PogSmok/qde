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

// An Operation whose register references have been resolved to flat indices.
struct CompiledOperation {
  OperationType type{OperationType::kGate};
  std::shared_ptr<const GateDefinition> gate;  // non-null if type == Gate
  std::vector<double> gate_params;
  // Flat qubit indices; for a gate, in the order of its matrix's qubits.
  std::vector<std::size_t> qubits;
  // Flat bit index receiving the outcome of measuring qubits[i].
  std::vector<std::size_t> measure_target;
  // If set, the operation runs only while classical bit `first` == `second`.
  std::optional<std::pair<std::size_t, std::uint8_t>> condition;
};

}  // namespace qde::simulator

#endif  // SIMULATOR_COMPILED_OPERATION_HPP_
