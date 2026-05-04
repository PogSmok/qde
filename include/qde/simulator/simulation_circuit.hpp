#ifndef SIMULATION_CIRCUIT_HPP_
#define SIMULATION_CIRCUIT_HPP_

#include <cstddef>
#include <vector>

#include "qde/circuit.hpp"
#include "qde/simulator/compiled_operation.hpp"

namespace qde {

class SimulationCircuit {
 public:
  SimulationCircuit(const Circuit& circuit);

  [[nodiscard]] std::size_t qubitCount() const noexcept { return qubit_count_; }

  [[nodiscard]] std::size_t bitCount() const noexcept { return bit_count_; }
  
  [[nodiscard]] const std::vector<std::vector<CompiledOperation>>& layers() 
      const noexcept { return layers_; }

 private:
  std::size_t qubit_count_;
  std::size_t bit_count_;

  std::vector<std::vector<CompiledOperation>> layers_; // ASAP scheduling
};

}  // namespace qde

#endif  // SIMULATION_CIRCUIT_HPP_