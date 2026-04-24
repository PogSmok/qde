#ifndef OPERATION_HPP_
#define OPERATION_HPP_

#include <cstdint>
#include <memory>
#include <vector>

#include "qde/gate_definition.hpp"

namespace qde {

struct QubitReference {
  std::uint8_t reg;   // index into Circuit::qubitRegisters()
  std::uint8_t qubit; // index within that register
};

struct Operation {
  std::shared_ptr<const GateDefinition> gate;
  std::vector<double> gate_params;
  std::vector<QubitReference> qubits;
};

}  // namespace qde

#endif  // OPERATION_HPP_