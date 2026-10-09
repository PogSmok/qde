#ifndef SIMULATOR_INTERNAL_QUANTUM_STATE_HPP_
#define SIMULATOR_INTERNAL_QUANTUM_STATE_HPP_

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "qde/simulator/internal/complex_math.hpp"

namespace qde::simulator::internal {

// The state of an n-qubit register.
//
// The state is kept as a state vector ψ (2^n amplitudes) for as long as it is
// pure, and as a row-major density matrix ρ (4^n elements) once it is mixed.
// Unitary gates and measurements never mix a pure state, so the expensive
// representation is entered only through ConvertToDensityMatrix().
//
// Qubit k is bit k of a basis state index. The class does not check whether
// its buffers fit in memory; callers do that with RequireMemory() before
// constructing or converting a state.
class QuantumState {
 public:
  // The state |0...0>. Throws std::length_error if 2^qubit_count amplitudes
  // cannot be indexed.
  explicit QuantumState(std::size_t qubit_count);

  [[nodiscard]] std::size_t QubitCount() const noexcept { return qubit_count_; }

  [[nodiscard]] bool IsPure() const noexcept { return pure_; }

  // ψ while pure, row-major ρ once mixed.
  [[nodiscard]] const std::vector<Amplitude>& Data() const noexcept {
    return data_;
  }

  // Hands the buffer returned by Data() to the caller.
  [[nodiscard]] std::vector<Amplitude> ReleaseData() && noexcept {
    return std::move(data_);
  }

  // Probability of each computational basis state, i.e. the diagonal of ρ.
  [[nodiscard]] std::vector<double> Probabilities() const;

  // Applies a unitary to `qubits`: ψ' = Uψ, or ρ' = UρU† once mixed.
  //
  // `matrix` is the row-major 2^k × 2^k unitary for k = qubits.size(), with
  // qubits[0] as the most significant bit of its row and column index.
  // Throws std::invalid_argument if the matrix has the wrong size, or a qubit
  // is out of range or repeated. Unitarity is not checked.
  void ApplyGate(const std::vector<Amplitude>& matrix,
                 const std::vector<std::size_t>& qubits);

  // Measures `qubit` in the computational basis, collapses the state onto the
  // outcome and returns it (0 or 1).
  //
  // `sample` is a uniform random number in [0, 1) that selects the outcome;
  // taking it as a parameter keeps measurement deterministic for callers.
  // Throws std::out_of_range if the qubit does not exist.
  std::uint8_t Measure(std::size_t qubit, double sample);

  // Resets `qubit` to |0> by tracing it out.
  //
  // Resetting a qubit that is entangled with the rest of a pure register
  // gives a mixed state. In that case the state is left untouched and false
  // is returned; call ConvertToDensityMatrix() and reset again. Always
  // succeeds on a mixed state.
  // Throws std::out_of_range if the qubit does not exist.
  [[nodiscard]] bool TryReset(std::size_t qubit);

  // Replaces ψ by ρ = |ψ><ψ|. Does nothing if the state is already mixed.
  // Throws std::length_error if 4^n elements cannot be indexed.
  void ConvertToDensityMatrix();

 private:
  [[nodiscard]] std::size_t Dim() const noexcept {
    return std::size_t{1} << qubit_count_;
  }

  void CheckQubit(std::size_t qubit) const;
  [[nodiscard]] bool TryResetPure(std::size_t qubit);
  void ResetMixed(std::size_t qubit);

  std::size_t qubit_count_;
  bool pure_{true};
  std::vector<Amplitude> data_;
};

}  // namespace qde::simulator::internal

#endif  // SIMULATOR_INTERNAL_QUANTUM_STATE_HPP_
