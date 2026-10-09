#include "qde/simulator/simulator.hpp"

#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <new>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "qde/operation.hpp"
#include "qde/simulator/compiled_operation.hpp"
#include "qde/simulator/internal/complex_math.hpp"
#include "qde/simulator/internal/eigenvalues.hpp"
#include "qde/simulator/internal/memory_guard.hpp"
#include "qde/simulator/internal/quantum_state.hpp"
#include "qde/simulator/simulation_circuit.hpp"
#include "qde/simulator/simulation_state.hpp"

namespace qde::simulator {

namespace {

using internal::Amplitude;
using internal::QuantumState;
using internal::RequireMemory;
using internal::SaturatingAdd;
using internal::SaturatingMul;

// |Tr(ρ²) - 1| below this is treated as a pure state.
constexpr double kPurityEps = 1e-12;

// Everything a run mutates while it steps through the circuit.
struct Execution {
  QuantumState state;
  std::vector<std::uint8_t> classical_bits;
  std::mt19937_64 rng;
};

std::uint64_t RandomSeed() {
  std::random_device device;
  const std::uint64_t high = device();
  return (high << 32) | device();
}

// Eigenvalues of the density matrix ρ, sorted descending.
std::vector<double> DensityMatrixEigenvalues(const DensityMatrix& rho,
                                             std::size_t dim) {
  // Tr(ρ²) is the squared Frobenius norm of a Hermitian matrix. A pure state
  // has the single non-zero eigenvalue 1, which spares the O(dim³) solver.
  double purity = 0.0;
  for (const Amplitude& element : rho) {
    purity += std::norm(element);
  }
  if (std::abs(purity - 1.0) < kPurityEps) {
    std::vector<double> eigenvalues(dim, 0.0);
    eigenvalues[0] = 1.0;
    return eigenvalues;
  }
  return internal::HermitianEigenvalues(rho, dim);
}

// Takes the state by value so that the caller chooses between copying it and
// handing over its buffer.
SimulationState MakeSnapshot(QuantumState state,
                             const std::vector<std::uint8_t>& classical_bits,
                             std::size_t layer) {
  SimulationState snapshot;
  snapshot.layer = layer;
  snapshot.qubit_count = state.QubitCount();
  snapshot.bit_count = classical_bits.size();
  snapshot.classical_bits = classical_bits;

  if (state.IsPure()) {
    snapshot.state_vector = std::move(state).ReleaseData();
  } else {
    snapshot.mixed_eigenvalues = DensityMatrixEigenvalues(
        state.Data(), std::size_t{1} << state.QubitCount());
    snapshot.density_matrix = std::move(state).ReleaseData();
  }
  return snapshot;
}

// Throws unless a run over n qubits fits in memory while the state stays pure
void RequireStateVectorMemory(std::size_t n, std::uint64_t copies) {
  RequireMemory(n, SaturatingMul(SaturatingAdd(copies, 1), sizeof(Amplitude)),
                "the state vector of " + std::to_string(n) + " qubits");
}

// Throws unless the density matrix of n qubits fits in memory together with
// its copy for the eigenvalue solver and one copy per remaining snapshot.
void RequireDensityMatrixMemory(std::size_t n,
                                std::uint64_t snapshots_remaining) {
  RequireMemory(
      2 * n,
      SaturatingMul(SaturatingAdd(2, snapshots_remaining), sizeof(Amplitude)),
      "the density matrix of " + std::to_string(n) +
          " qubits (resetting an entangled qubit makes the state mixed)");
}

// Resets `qubit`, switching to a density matrix if the reset mixes the state.
// `snapshots_remaining` is the number of copies of the state still to be
// taken, which have to fit in memory next to it.
void Reset(QuantumState& state, std::size_t qubit,
           std::uint64_t snapshots_remaining) {
  if (state.TryReset(qubit)) {
    return;
  }

  RequireDensityMatrixMemory(state.QubitCount(), snapshots_remaining);
  state.ConvertToDensityMatrix();
  static_cast<void>(state.TryReset(qubit));  // cannot fail on a mixed state
}

// Measures every qubit of `op` into its target bit.
void Measure(const CompiledOperation& op, Execution& execution) {
  std::uniform_real_distribution<double> uniform{0.0, 1.0};
  for (std::size_t i = 0; i < op.qubits.size(); i++) {
    const std::uint8_t outcome =
        execution.state.Measure(op.qubits[i], uniform(execution.rng));
    // A measurement without a target bit still collapses the state.
    if (i < op.measure_target.size()) {
      execution.classical_bits[op.measure_target[i]] = outcome;
    }
  }
}

// Runs `op` unless its classical condition is not met.
void Apply(const CompiledOperation& op, Execution& execution,
           std::uint64_t snapshots_remaining) {
  if (op.condition) {
    const auto [bit, expected] = *op.condition;
    if (execution.classical_bits[bit] != expected) {
      return;
    }
  }

  switch (op.type) {
    case OperationType::kGate:
      execution.state.ApplyGate(op.gate->Matrix(op.gate_params), op.qubits);
      break;
    case OperationType::kMeasure:
      Measure(op, execution);
      break;
    case OperationType::kReset:
      for (const std::size_t qubit : op.qubits) {
        Reset(execution.state, qubit, snapshots_remaining);
      }
      break;
    case OperationType::kBarrier:
      break;
  }
}

// Returns the state before every layer followed by the final state if
// `save_all`; otherwise only the final state.
// `seed` fixes the measurement outcomes.
std::vector<SimulationState> Simulate(const SimulationCircuit& circuit,
                                      bool save_all, std::uint64_t seed) {
  const std::size_t n = circuit.QubitCount();
  const std::size_t layer_count = circuit.Layers().size();

  const std::uint64_t copies = save_all ? layer_count : 0;
  RequireStateVectorMemory(n, copies);

  Execution execution{QuantumState(n),
                      std::vector<std::uint8_t>(circuit.BitCount(), 0),
                      std::mt19937_64(seed)};

  std::vector<SimulationState> states;
  states.reserve(static_cast<std::size_t>(copies) + 1);

  for (std::size_t layer = 0; layer < layer_count; layer++) {
    if (save_all) {
      states.push_back(
          MakeSnapshot(execution.state, execution.classical_bits, layer));
    }
    // Snapshots still to be copied once this layer has run.
    const std::uint64_t snapshots_remaining =
        save_all ? layer_count - layer - 1 : 0;
    for (const CompiledOperation& op : circuit.Layers()[layer]) {
      Apply(op, execution, snapshots_remaining);
    }
  }

  states.push_back(MakeSnapshot(std::move(execution.state),
                                execution.classical_bits, layer_count));
  return states;
}

// Reports allocation failure as a runtime error
std::vector<SimulationState> SimulateChecked(const SimulationCircuit& circuit,
                                             bool save_all,
                                             std::uint64_t seed) {
  try {
    return Simulate(circuit, save_all, seed);
  } catch (const std::bad_alloc&) {
    throw std::runtime_error("Simulator: out of memory while simulating " +
                             std::to_string(circuit.QubitCount()) + " qubits");
  }
}

}  // namespace

void Simulator::SetNoiseModel(const std::shared_ptr<const NoiseModel>& model) {
  noise_model_ = model;
}

std::vector<SimulationState> Simulator::Run(const SimulationCircuit& circuit) {
  return Run(circuit, RandomSeed());
}

std::vector<SimulationState> Simulator::Run(const SimulationCircuit& circuit,
                                            std::uint64_t seed) {
  return SimulateChecked(circuit, true, seed);
}

SimulationState Simulator::RunFinal(const SimulationCircuit& circuit) {
  return RunFinal(circuit, RandomSeed());
}

SimulationState Simulator::RunFinal(const SimulationCircuit& circuit,
                                    std::uint64_t seed) {
  std::vector<SimulationState> states(SimulateChecked(circuit, false, seed));
  return std::move(states.back());
}

}  // namespace qde::simulator
