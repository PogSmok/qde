#include "qde/qubit_allocator.hpp"

#include <string>
#include <unordered_map>
#include <vector>

#include "qde/circuit.hpp"

namespace qde {

// TODO allocate against actual constrained hardware config
// this will also change function signature to also take const T& config
std::unordered_map<std::string, std::size_t> QubitAllocator::Allocate(
    const Circuit& circuit) {
  std::unordered_map<std::string, std::size_t> mapping;

  const std::vector<QubitRegister>& qregs = circuit.QubitRegisters();
  std::size_t id = 0;

  for (const QubitRegister& qreg : qregs) {
    for (std::size_t i = 0; i < qreg.size; i++) {
      mapping[qreg.name + "[" + std::to_string(i) + "]"] = id;
      id++;
    }
  }

  return mapping;
}

}  // namespace qde