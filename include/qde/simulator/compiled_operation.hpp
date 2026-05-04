#ifndef SIMULATOR_COMPILED_OPERATION_HPP_
#define SIMULATOR_COMPILED_OPERATION_HPP_

#include <cstdint>
#include <memory>
#include <vector>

#include "qde/gate_definition.hpp"
#include "qde/operation.hpp"

namespace qde {

struct CompiledOperation {
  OperationType type = OperationType::kGate;
  std::shared_ptr<const GateDefinition> gate;  // non-null if type == Gate
  std::vector<double> gate_params;
  std::vector<std::size_t> qubits;
  std::vector<std::size_t> measure_target;
};

}  // namespace qde

#endif  // SIMULATOR_COMPILED_OPERATION_HPP_