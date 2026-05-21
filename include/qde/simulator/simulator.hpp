#ifndef SIMULATOR_SIMULATOR_HPP_
#define SIMULATOR_SIMULATOR_HPP_

#include <memory>
#include <vector>

#include "qde/simulator/simulation_circuit.hpp"
#include "qde/simulator/simulation_state.hpp"

namespace qde {

class NoiseModel;

class Simulator {
 public:
  // Run the full simulation and return one SimulationState per layer.
  // Index 0 is the initial state (all qubits |0>, no gates applied yet).
  [[nodiscard]] static std::vector<SimulationState> Run(
      const SimulationCircuit& circuit);

  // Run and return only the final state.
  // More efficient when intermediate states are not needed.
  [[nodiscard]] static SimulationState RunFinal(
      const SimulationCircuit& circuit);

  // Attach a noise model applied after every gate. Pass nullptr to disable.
  void SetNoiseModel(const std::shared_ptr<const NoiseModel>& model);

  [[nodiscard]] bool HasNoiseModel() const noexcept {
    return noise_model_ != nullptr;
  }

 private:
  std::shared_ptr<const NoiseModel> noise_model_;
};

}  // namespace qde

#endif  // SIMULATOR_SIMULATOR_HPP_
