#include <gtest/gtest.h>

#include <complex>
#include <cstddef>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "qde/simulator/internal/eigenvalues.hpp"

namespace qde::simulator::internal {

namespace {

using Matrix = std::vector<std::complex<double>>;

constexpr double kTol = 1e-12;

}  // namespace

TEST(HermitianEigenvaluesTest, EmptyMatrixHasNoEigenvalues) {
  EXPECT_TRUE(HermitianEigenvalues({}, 0).empty());
}

TEST(HermitianEigenvaluesTest, OneByOneMatrix) {
  const auto values = HermitianEigenvalues({{0.25, 0.0}}, 1);
  ASSERT_EQ(values.size(), 1U);
  EXPECT_NEAR(values[0], 0.25, kTol);
}

TEST(HermitianEigenvaluesTest, DiagonalMatrixIsSortedDescending) {
  const Matrix diagonal = {
      {0.1, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.6, 0.0},
      {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.3, 0.0},
  };
  const auto values = HermitianEigenvalues(diagonal, 3);
  ASSERT_EQ(values.size(), 3U);
  EXPECT_NEAR(values[0], 0.6, kTol);
  EXPECT_NEAR(values[1], 0.3, kTol);
  EXPECT_NEAR(values[2], 0.1, kTol);
}

TEST(HermitianEigenvaluesTest, ComplexOffDiagonalElements) {
  // The projector onto (|0> + i|1>)/√2 has eigenvalues 1 and 0.
  const Matrix projector = {{0.5, 0.0}, {0.0, -0.5}, {0.0, 0.5}, {0.5, 0.0}};
  const auto values = HermitianEigenvalues(projector, 2);
  ASSERT_EQ(values.size(), 2U);
  EXPECT_NEAR(values[0], 1.0, kTol);
  EXPECT_NEAR(values[1], 0.0, kTol);
}

TEST(HermitianEigenvaluesTest, NegativeEigenvalues) {
  // Pauli Y.
  const Matrix pauli_y = {{0.0, 0.0}, {0.0, -1.0}, {0.0, 1.0}, {0.0, 0.0}};
  const auto values = HermitianEigenvalues(pauli_y, 2);
  ASSERT_EQ(values.size(), 2U);
  EXPECT_NEAR(values[0], 1.0, kTol);
  EXPECT_NEAR(values[1], -1.0, kTol);
}

TEST(HermitianEigenvaluesTest, DenseMatrixWithDegenerateSpectrum) {
  // The 4x4 all-ones matrix has rank one: eigenvalues 4, 0, 0, 0.
  const Matrix ones(16, {1.0, 0.0});
  const auto values = HermitianEigenvalues(ones, 4);
  ASSERT_EQ(values.size(), 4U);
  EXPECT_NEAR(values[0], 4.0, kTol);
  for (std::size_t i = 1; i < 4; ++i) {
    EXPECT_NEAR(values[i], 0.0, kTol);
  }
}

TEST(HermitianEigenvaluesTest, PreservesTrace) {
  constexpr std::size_t dim = 6;
  Matrix matrix(dim * dim);
  double trace = 0.0;
  for (std::size_t i = 0; i < dim; ++i) {
    for (std::size_t j = i; j < dim; ++j) {
      const std::complex<double> element{
          1.0 / static_cast<double>(i + j + 1),
          i == j ? 0.0 : static_cast<double>(i) - static_cast<double>(j)};
      matrix[(i * dim) + j] = element;
      matrix[(j * dim) + i] = std::conj(element);
    }
    trace += matrix[(i * dim) + i].real();
  }
  const auto values = HermitianEigenvalues(matrix, dim);
  ASSERT_EQ(values.size(), dim);
  EXPECT_NEAR(std::accumulate(values.begin(), values.end(), 0.0), trace, 1e-10);
}

TEST(HermitianEigenvaluesTest, RejectsMatrixOfWrongSize) {
  const Matrix three_elements(3);
  EXPECT_THROW(static_cast<void>(HermitianEigenvalues(three_elements, 2)),
               std::invalid_argument);
  EXPECT_THROW(static_cast<void>(HermitianEigenvalues(three_elements, 0)),
               std::invalid_argument);
}

}  // namespace qde::simulator::internal
