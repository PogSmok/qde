#ifndef OPERATION_HPP_
#define OPERATION_HPP_

#include <cstdint>
#include <memory>
#include <vector>

#include "qde/gate_definition.hpp"

namespace qde {

struct QubitReference {
  std::uint8_t reg;    // index into Circuit::qubitRegisters()
  std::uint8_t qubit;  // index within that register
};

struct BitReference {
  std::uint8_t reg;  // index into Circuit::bitRegisters()
  std::uint8_t bit;  // index within that register
};

enum class OperationType : std::uint8_t {
  Gate,     // unitary gate application
  Measure,  // measurement, writes result to measure_target
  Reset,    // reset qubit to |0>
  Barrier,  // prevents gate reordering across this point
};

struct Operation {
  OperationType type = OperationType::Gate;
  std::shared_ptr<const GateDefinition> gate;  // non-null if type == Gate
  std::vector<double> gate_params;
  std::vector<QubitReference> qubits;
  std::vector<BitReference> measure_target;
};

}  // namespace qde

#endif  // OPERATION_HPP_