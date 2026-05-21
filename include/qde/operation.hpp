#ifndef OPERATION_HPP_
#define OPERATION_HPP_

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "qde/gate_definition.hpp"

namespace qde {

struct QubitReference {
  std::uint8_t reg;    // index into Circuit::QubitRegisters()
  std::uint8_t qubit;  // index within that register
};

struct BitReference {
  std::uint8_t reg;  // index into Circuit::BitRegisters()
  std::uint8_t bit;  // index within that register
};

enum class OperationType : std::uint8_t {
  kGate,     // unitary gate application
  kMeasure,  // measurement, writes result to measure_target
  kReset,    // reset qubit to |0>
  kBarrier,  // prevents gate reordering across this point
};

struct Operation {
  OperationType type = OperationType::kGate;
  std::shared_ptr<const GateDefinition> gate;  // non-null if type == Gate
  std::vector<double> gate_params;
  std::vector<QubitReference> qubits;
  std::vector<BitReference> measure_target;
  // if set, the operation is skipped unless classical_bits[bit] == expected
  std::optional<std::pair<BitReference, std::uint8_t>> condition;
};

}  // namespace qde

#endif  // OPERATION_HPP_