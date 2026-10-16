#ifndef CIRCUIT_HPP_
#define CIRCUIT_HPP_

#include <algorithm>
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

  [[nodiscard]] const GateRegistry& GetRegistry() const noexcept {
    return gate_registry_;
  }
  [[nodiscard]] const std::vector<QubitRegister>& QubitRegisters()
      const noexcept {
    return qubit_registers_;
  }
  [[nodiscard]] const std::vector<BitRegister>& BitRegisters() const noexcept {
    return bit_registers_;
  }

  // Can contain non-flat blocks
  [[nodiscard]] const std::vector<Operation>& Operations() const noexcept {
    return operations_;
  }

  [[nodiscard]] std::vector<Operation> FlatOperations() const {
    std::vector<Operation> flat;
    AppendFlat(operations_, flat);
    return flat;
  }

  [[nodiscard]] bool IsFlat() const {
    return std::none_of(operations_.begin(), operations_.end(),
                        [](const Operation& operation) {
                          return operation.type == OperationType::kBlock;
                        });
  }

 private:
  static void AppendFlat(const std::vector<Operation>& operations,
                         std::vector<Operation>& flat) {
    for (const Operation& operation : operations) {
      if (operation.type == OperationType::kBlock) {
        AppendFlat(operation.body, flat);
      } else {
        flat.push_back(operation);
      }
    }
  }

  GateRegistry gate_registry_;
  std::vector<QubitRegister> qubit_registers_;
  std::vector<BitRegister> bit_registers_;
  std::vector<Operation> operations_;
};

}  // namespace qde

#endif  // CIRCUIT_HPP_