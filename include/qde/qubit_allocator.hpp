#ifndef QUBIT_ALLOCATOR_HPP_
#define QUBIT_ALLOCATOR_HPP_

#include <string>
#include <unordered_map>

#include "qde/circuit.hpp"

namespace qde {

class QubitAllocator {
 public:
  static std::unordered_map<std::string, std::size_t> Allocate(
      const Circuit& circuit);
};

}  // namespace qde

#endif  // QUBIT_ALLOCATOR_HPP_