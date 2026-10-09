#ifndef SIMULATOR_SIMULATOR_HPP_
#define SIMULATOR_SIMULATOR_HPP_

#include <cstdint>
#include <memory>
#include <vector>

#include "qde/simulator/simulation_circuit.hpp"
#include "qde/simulator/simulation_state.hpp"

namespace qde::simulator {

class NoiseModel;

// Simulates a SimulationCircuit layer by layer, starting from |0...0>.
//
// A state costs 2^n amplitudes while it is pure and 4^n once it is mixed, so
// the largest circuit that runs is bounded by installed memory as well as by
// kMaxQubits.
//
// Measurement outcomes are random. The overloads without a seed draw one from
// the operating system, so two runs of a circuit that measures a superposition
// may differ; the overloads taking a seed are reproducible.
//
// Run and RunFinal are safe to call concurrently.
class Simulator {
 public:
  // Runs the circuit and returns one state per layer plus the initial one:
  // element i is the state after the first i layers.
  // Throws std::runtime_error if the states do not fit in memory.
  [[nodiscard]] static std::vector<SimulationState> Run(
      const SimulationCircuit& circuit);
  [[nodiscard]] static std::vector<SimulationState> Run(
      const SimulationCircuit& circuit, std::uint64_t seed);

  // Runs the circuit and returns only the final state. Needs the memory of a
  // single state, however many layers the circuit has.
  // Throws std::runtime_error if the state does not fit in memory.
  [[nodiscard]] static SimulationState RunFinal(
      const SimulationCircuit& circuit);
  [[nodiscard]] static SimulationState RunFinal(
      const SimulationCircuit& circuit, std::uint64_t seed);

  // Stores a noise model, or clears it with nullptr.
  // TODO: noise is not simulated yet, so Run and RunFinal ignore the model.
  void SetNoiseModel(const std::shared_ptr<const NoiseModel>& model);

  [[nodiscard]] bool HasNoiseModel() const noexcept {
    return noise_model_ != nullptr;
  }

 private:
  std::shared_ptr<const NoiseModel> noise_model_;
};

}  // namespace qde::simulator

#endif  // SIMULATOR_SIMULATOR_HPP_
