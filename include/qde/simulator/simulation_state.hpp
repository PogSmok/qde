#ifndef SIMULATOR_SIMULATION_STATE_HPP_
#define SIMULATOR_SIMULATION_STATE_HPP_

#include <complex>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace qde::simulator {

// ---- Notation ---------------------------------------------------------------
//
//  n    — number of qubits (qubit_count)
//
//  dim  — dimension of the Hilbert space: dim = 2^n
//
//  ψ    — state vector (psi): dim complex amplitudes describing a pure state.
//           Amplitude of basis state i is at index i; qubit k is bit k of i.
//
//  ρ    — density matrix (rho): a dim×dim complex Hermitian matrix with
//           trace 1 that fully describes a quantum state, pure or mixed.
//
//  Storage: row-major flat vector of length dim².
//           Element (i, j) is at index  i * dim + j.
//           Diagonal element ρ[i][i] = probability of measuring basis state i.
//
// -----------------------------------------------------------------------------

using StateVector = std::vector<std::complex<double>>;
using DensityMatrix = std::vector<std::complex<double>>;

struct SimulationState {
  // Index of the layer that produced this state (0 = initial state).
  std::size_t layer{0};
  std::size_t qubit_count{0};
  std::size_t bit_count{0};

  // Exactly one of the two representations is populated. A state stays a
  // state vector (dim amplitudes) for as long as it is pure, and only becomes
  // a density matrix (dim² elements) once an operation makes it mixed.
  StateVector state_vector;      // size dim if pure, empty otherwise
  DensityMatrix density_matrix;  // size dim² if mixed, empty otherwise

  // Eigenvalues of ρ sorted descending, stored only for a mixed state: size
  // dim if mixed, empty otherwise. Use Eigenvalues() to cover both cases.
  std::vector<double> mixed_eigenvalues;

  // Classical register values after any measurements in this layer.
  // Indexed by flat bit index matching bit_count.
  std::vector<std::uint8_t> classical_bits;

  // Gate fidelity relative to the ideal (noise-free) evolution for this layer.
  // 1.0 = perfect (no noise), 0.0 = orthogonal.
  double gate_fidelity{1.0};

  [[nodiscard]] bool IsPure() const noexcept { return !state_vector.empty(); }

  // Dimension of the Hilbert space, 2^qubit_count.
  [[nodiscard]] std::size_t Dim() const noexcept {
    return std::size_t{1} << qubit_count;
  }

  // Probability of measuring basis state `index`, i.e. ρ[index][index].
  [[nodiscard]] double Probability(std::size_t index) const {
    if (IsPure()) {
      return std::norm(state_vector[index]);
    }
    return density_matrix[(index << qubit_count) + index].real();
  }

  // Probability of every basis state. Allocates Dim() doubles.
  [[nodiscard]] std::vector<double> Probabilities() const {
    std::vector<double> probabilities(Dim());
    for (std::size_t i = 0; i < probabilities.size(); ++i) {
      probabilities[i] = Probability(i);
    }
    return probabilities;
  }

  // Eigenvalues of ρ sorted descending: 1 followed by zeros for a pure state.
  // Allocates Dim() doubles.
  [[nodiscard]] std::vector<double> Eigenvalues() const {
    if (!IsPure()) {
      return mixed_eigenvalues;
    }
    std::vector<double> eigenvalues(Dim(), 0.0);
    eigenvalues[0] = 1.0;
    return eigenvalues;
  }

  // ρ[row][col], whichever representation is populated.
  [[nodiscard]] std::complex<double> DensityElement(std::size_t row,
                                                    std::size_t col) const {
    if (IsPure()) {
      return state_vector[row] * std::conj(state_vector[col]);
    }
    return density_matrix[(row << qubit_count) + col];
  }
};

}  // namespace qde::simulator

#endif  // SIMULATOR_SIMULATION_STATE_HPP_
