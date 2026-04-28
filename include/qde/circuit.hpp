#ifndef CIRCUIT_HPP_
#define CIRCUIT_HPP_

#include <cstdint>
#include <string>
#include <vector>

#include "qde/gate_registry.hpp"
#include "qde/operation.hpp"

namespace qde {

struct QubitRegister {
  std::string name;
  std::size_t size;
};

struct BitRegister {
  std::string name;
  std::size_t size;
};

class Circuit {
 public:
  Circuit(GateRegistry gate_registry,
          std::vector<QubitRegister> qubit_registers,
          std::vector<BitRegister> bit_registers,
          std::vector<Operation> operations)
      : gate_registry_(std::move(gate_registry)),
        qubit_registers_(std::move(qubit_registers)),
        bit_registers_(std::move(bit_registers)),
        operations_(std::move(operations)) {}

  const GateRegistry& gateRegistry() const noexcept { return gate_registry_; }
  const std::vector<QubitRegister>& qubitRegisters() const noexcept {
    return qubit_registers_;
  }
  const std::vector<BitRegister>& bitRegisters() const noexcept {
    return bit_registers_;
  }
  const std::vector<Operation>& operations() const noexcept {
    return operations_;
  }

 private:
  GateRegistry gate_registry_;
  std::vector<QubitRegister> qubit_registers_;
  std::vector<BitRegister> bit_registers_;
  std::vector<Operation> operations_;
};

}  // namespace qde

#endif  // CIRCUIT_HPP_