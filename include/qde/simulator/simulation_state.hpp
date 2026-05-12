#ifndef SIMULATOR_SIMULATION_STATE_HPP_
#define SIMULATOR_SIMULATION_STATE_HPP_

#include <complex>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace qde {

// ---- Notation ---------------------------------------------------------------
//
//  n    — number of qubits (qubit_count)
//
//  dim  — dimension of the Hilbert space: dim = 2^n
//
//  ρ    — density matrix (rho): a dim×dim complex Hermitian matrix with
//           trace 1 that fully describes a quantum state, pure or mixed.
//
//  Storage: row-major flat vector of length dim².
//           Element (i, j) is at index  i * dim + j.
//           Diagonal element ρ[i][i] = probability of measuring basis state i.
//
// -----------------------------------------------------------------------------

using DensityMatrix = std::vector<std::complex<double>>;

struct SimulationState {
  // Index of the layer that produced this state (0 = initial state).
  std::size_t layer = 0;
  std::size_t qubit_count = 0;
  std::size_t bit_count = 0;

  DensityMatrix density_matrix;

  // Diagonal of ρ in the computational basis: probability[i] = ρ[i][i].
  // Size dim. Stored separately for convenience.
  std::vector<double> basis_probabilities;

  // Eigenvalues of ρ sorted descending. Size dim.
  std::vector<double> eigenvalues;

  // Classical register values after any measurements in this layer.
  // Indexed by flat bit index matching bit_count.
  std::vector<std::uint8_t> classical_bits;

  // Gate fidelity relative to the ideal (noise-free) evolution for this layer.
  // 1.0 = perfect (no noise), 0.0 = orthogonal.
  double gate_fidelity = 1.0;
};

}  // namespace qde

#endif  // SIMULATOR_SIMULATION_STATE_HPP_
