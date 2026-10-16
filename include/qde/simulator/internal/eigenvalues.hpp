#ifndef SIMULATOR_INTERNAL_EIGENVALUES_HPP_
#define SIMULATOR_INTERNAL_EIGENVALUES_HPP_

#include <complex>
#include <cstddef>
#include <vector>

namespace qde::simulator::internal {

// Eigenvalues of a Hermitian matrix, sorted descending.
//
// `matrix` is row-major with dim * dim elements; only Hermitian input gives
// meaningful results. Runs in O(dim³) time and O(dim²) extra memory.
// Throws std::invalid_argument if the matrix does not have dim * dim elements.
[[nodiscard]] std::vector<double> HermitianEigenvalues(
    const std::vector<std::complex<double>>& matrix, std::size_t dim);

}  // namespace qde::simulator::internal

#endif  // SIMULATOR_INTERNAL_EIGENVALUES_HPP_
