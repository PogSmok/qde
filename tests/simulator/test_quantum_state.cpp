#include <gtest/gtest.h>

#include <cmath>
#include <complex>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "qde/gate_registry.hpp"
#include "qde/simulator/internal/complex_math.hpp"
#include "qde/simulator/internal/quantum_state.hpp"

namespace qde::simulator::internal {

namespace {

constexpr double kTol = 1e-12;

// ρ[row][col] of either representation.
Amplitude DensityElement(const QuantumState& state, std::size_t row,
                         std::size_t col) {
  const std::vector<Amplitude>& data(state.Data());
  if (state.IsPure()) {
    return data[row] * std::conj(data[col]);
  }
  return data[(row << state.QubitCount()) + col];
}

}  // namespace

class QuantumStateTest : public ::testing::Test {
 protected:
  [[nodiscard]] std::vector<Amplitude> Matrix(
      const std::string& gate, const std::vector<double>& params = {}) const {
    return registry_.Find(gate)->Matrix(params);
  }

  // Puts qubits 0 and 1 in the Bell state (|00> + |11>)/√2.
  void MakeBellPair(QuantumState& state) const {
    state.ApplyGate(Matrix("h"), {0});
    state.ApplyGate(Matrix("cx"), {0, 1});
  }

 private:
  GateRegistry registry_{GateRegistry::WithBuiltins()};
};

// ---- Construction -----------------------------------------------------------

TEST_F(QuantumStateTest, StartsInAllZerosState) {
  const QuantumState state(3);
  EXPECT_EQ(state.QubitCount(), 3U);
  EXPECT_TRUE(state.IsPure());
  ASSERT_EQ(state.Data().size(), 8U);
  EXPECT_EQ(state.Data()[0], Amplitude(1.0, 0.0));
  for (std::size_t i = 1; i < 8; ++i) {
    EXPECT_EQ(state.Data()[i], Amplitude(0.0, 0.0));
  }
}

TEST_F(QuantumStateTest, ZeroQubitStateIsASingleAmplitude) {
  const QuantumState state(0);
  ASSERT_EQ(state.Data().size(), 1U);
  EXPECT_EQ(state.Probabilities(), std::vector<double>{1.0});
}

TEST_F(QuantumStateTest, RejectsQubitCountThatCannotBeIndexed) {
  EXPECT_THROW(QuantumState(64), std::length_error);
}

TEST_F(QuantumStateTest, ReleaseDataHandsOverTheBuffer) {
  QuantumState state(2);
  const std::vector<Amplitude> data(std::move(state).ReleaseData());
  EXPECT_EQ(data.size(), 4U);
}

// ---- Gates ------------------------------------------------------------------

TEST_F(QuantumStateTest, SingleQubitGateActsOnItsQubitOnly) {
  QuantumState state(2);
  state.ApplyGate(Matrix("x"), {1});
  const auto probabilities = state.Probabilities();
  EXPECT_NEAR(probabilities[0b10], 1.0, kTol);
}

TEST_F(QuantumStateTest, FirstGateQubitIsMostSignificantMatrixBit) {
  // cx with control = qubit 1, target = qubit 0.
  QuantumState state(2);
  state.ApplyGate(Matrix("x"), {1});
  state.ApplyGate(Matrix("cx"), {1, 0});
  EXPECT_NEAR(state.Probabilities()[0b11], 1.0, kTol);

  // The control is clear here, so nothing happens.
  QuantumState idle(2);
  idle.ApplyGate(Matrix("x"), {0});
  idle.ApplyGate(Matrix("cx"), {1, 0});
  EXPECT_NEAR(idle.Probabilities()[0b01], 1.0, kTol);
}

TEST_F(QuantumStateTest, ThreeQubitGateOnNonAdjacentQubits) {
  // ccx with controls 3 and 1 flips qubit 2 only when both are set.
  QuantumState state(4);
  state.ApplyGate(Matrix("x"), {3});
  state.ApplyGate(Matrix("ccx"), {3, 1, 2});
  EXPECT_NEAR(state.Probabilities()[0b1000], 1.0, kTol);
  state.ApplyGate(Matrix("x"), {1});
  state.ApplyGate(Matrix("ccx"), {3, 1, 2});
  EXPECT_NEAR(state.Probabilities()[0b1110], 1.0, kTol);
}

TEST_F(QuantumStateTest, GatesPreserveNorm) {
  QuantumState state(3);
  state.ApplyGate(Matrix("u3", {0.3, 0.9, 1.7}), {0});
  state.ApplyGate(Matrix("ry", {1.1}), {2});
  state.ApplyGate(Matrix("cx"), {0, 2});
  state.ApplyGate(Matrix("ccx"), {2, 0, 1});
  double norm = 0.0;
  for (const double p : state.Probabilities()) {
    norm += p;
  }
  EXPECT_NEAR(norm, 1.0, kTol);
}

TEST_F(QuantumStateTest, DensityMatrixEvolutionMatchesStateVector) {
  const auto evolve = [this](QuantumState& state) {
    state.ApplyGate(Matrix("u3", {0.3, 0.9, 1.7}), {1});
    state.ApplyGate(Matrix("cx"), {1, 2});
    state.ApplyGate(Matrix("rx", {0.8}), {0});
    state.ApplyGate(Matrix("ccx"), {2, 0, 1});
    state.ApplyGate(Matrix("rz", {0.4}), {2});
    state.ApplyGate(Matrix("cx"), {0, 1});
  };

  QuantumState pure(3);
  evolve(pure);

  QuantumState mixed(3);
  mixed.ConvertToDensityMatrix();
  ASSERT_FALSE(mixed.IsPure());
  evolve(mixed);

  for (std::size_t row = 0; row < 8; ++row) {
    for (std::size_t col = 0; col < 8; ++col) {
      const Amplitude expected = DensityElement(pure, row, col);
      const Amplitude actual = DensityElement(mixed, row, col);
      EXPECT_NEAR(actual.real(), expected.real(), kTol);
      EXPECT_NEAR(actual.imag(), expected.imag(), kTol);
    }
  }
}

TEST_F(QuantumStateTest, RejectsInvalidGates) {
  QuantumState state(2);
  EXPECT_THROW(state.ApplyGate(Matrix("h"), {}), std::invalid_argument);
  EXPECT_THROW(state.ApplyGate(Matrix("h"), {2}), std::invalid_argument);
  EXPECT_THROW(state.ApplyGate(Matrix("cx"), {0, 0}), std::invalid_argument);
  EXPECT_THROW(state.ApplyGate(Matrix("cx"), {0}), std::invalid_argument);
  EXPECT_THROW(state.ApplyGate(Matrix("ccx"), {0, 1, 2}),
               std::invalid_argument);
  // A rejected gate leaves the state untouched.
  EXPECT_NEAR(state.Probabilities()[0], 1.0, kTol);
}

// ---- Measurement ------------------------------------------------------------

TEST_F(QuantumStateTest, SampleSelectsMeasurementOutcome) {
  QuantumState low(1);
  low.ApplyGate(Matrix("h"), {0});
  EXPECT_EQ(low.Measure(0, 0.25), 0);
  EXPECT_NEAR(low.Probabilities()[0], 1.0, kTol);

  QuantumState high(1);
  high.ApplyGate(Matrix("h"), {0});
  EXPECT_EQ(high.Measure(0, 0.75), 1);
  EXPECT_NEAR(high.Probabilities()[1], 1.0, kTol);
}

TEST_F(QuantumStateTest, MeasurementNeverSelectsImpossibleOutcome) {
  QuantumState zero(1);
  EXPECT_EQ(zero.Measure(0, 0.999999), 0);

  QuantumState one(1);
  one.ApplyGate(Matrix("x"), {0});
  EXPECT_EQ(one.Measure(0, 0.0), 1);
}

TEST_F(QuantumStateTest, MeasuringOneHalfOfBellPairCollapsesTheOther) {
  QuantumState state(2);
  MakeBellPair(state);
  EXPECT_EQ(state.Measure(0, 0.9), 1);
  EXPECT_TRUE(state.IsPure());
  EXPECT_NEAR(state.Probabilities()[0b11], 1.0, kTol);
  EXPECT_EQ(state.Measure(1, 0.1), 1);
}

TEST_F(QuantumStateTest, MeasurementCollapsesDensityMatrix) {
  QuantumState state(2);
  MakeBellPair(state);
  state.ConvertToDensityMatrix();
  EXPECT_EQ(state.Measure(1, 0.1), 0);
  EXPECT_NEAR(DensityElement(state, 0, 0).real(), 1.0, kTol);
  for (std::size_t i = 1; i < 16; ++i) {
    EXPECT_NEAR(std::abs(state.Data()[i]), 0.0, kTol);
  }
}

TEST_F(QuantumStateTest, MeasureRejectsMissingQubit) {
  QuantumState state(2);
  EXPECT_THROW(state.Measure(2, 0.5), std::out_of_range);
}

// ---- Reset ------------------------------------------------------------------

TEST_F(QuantumStateTest, ResetOfProductStateStaysPure) {
  // qubit 0 in |->, qubit 1 in |+>.
  QuantumState state(2);
  state.ApplyGate(Matrix("x"), {0});
  state.ApplyGate(Matrix("h"), {0});
  state.ApplyGate(Matrix("h"), {1});
  ASSERT_TRUE(state.TryReset(0));
  EXPECT_TRUE(state.IsPure());
  const auto probabilities = state.Probabilities();
  EXPECT_NEAR(probabilities[0b00], 0.5, kTol);
  EXPECT_NEAR(probabilities[0b10], 0.5, kTol);
}

TEST_F(QuantumStateTest, ResetOfQubitInOneState) {
  QuantumState state(2);
  state.ApplyGate(Matrix("x"), {0});
  state.ApplyGate(Matrix("x"), {1});
  ASSERT_TRUE(state.TryReset(1));
  EXPECT_NEAR(state.Probabilities()[0b01], 1.0, kTol);
}

TEST_F(QuantumStateTest, ResetOfEntangledQubitIsRefusedWhilePure) {
  QuantumState state(2);
  MakeBellPair(state);
  const std::vector<Amplitude> before(state.Data());
  EXPECT_FALSE(state.TryReset(0));
  EXPECT_TRUE(state.IsPure());
  EXPECT_EQ(state.Data(), before);
}

TEST_F(QuantumStateTest, ResetOfEntangledQubitMixesDensityMatrix) {
  QuantumState state(2);
  MakeBellPair(state);
  state.ConvertToDensityMatrix();
  ASSERT_TRUE(state.TryReset(0));
  // ρ = |0><0| on qubit 0, maximally mixed on qubit 1.
  EXPECT_NEAR(DensityElement(state, 0b00, 0b00).real(), 0.5, kTol);
  EXPECT_NEAR(DensityElement(state, 0b10, 0b10).real(), 0.5, kTol);
  EXPECT_NEAR(std::abs(DensityElement(state, 0b00, 0b10)), 0.0, kTol);
  EXPECT_NEAR(std::abs(DensityElement(state, 0b01, 0b01)), 0.0, kTol);
  EXPECT_NEAR(std::abs(DensityElement(state, 0b11, 0b11)), 0.0, kTol);
}

TEST_F(QuantumStateTest, TryResetRejectsMissingQubit) {
  QuantumState state(1);
  EXPECT_THROW(static_cast<void>(state.TryReset(1)), std::out_of_range);
}

// ---- Density matrix ---------------------------------------------------------

TEST_F(QuantumStateTest, ConvertToDensityMatrixBuildsOuterProduct) {
  QuantumState state(1);
  state.ApplyGate(Matrix("h"), {0});
  state.ApplyGate(Matrix("s"), {0});  // (|0> + i|1>)/√2
  state.ConvertToDensityMatrix();
  ASSERT_EQ(state.Data().size(), 4U);
  EXPECT_NEAR(state.Data()[0].real(), 0.5, kTol);
  EXPECT_NEAR(state.Data()[1].imag(), -0.5, kTol);
  EXPECT_NEAR(state.Data()[2].imag(), 0.5, kTol);
  EXPECT_NEAR(state.Data()[3].real(), 0.5, kTol);
}

TEST_F(QuantumStateTest, ConvertToDensityMatrixIsIdempotent) {
  QuantumState state(1);
  state.ConvertToDensityMatrix();
  const std::vector<Amplitude> once(state.Data());
  state.ConvertToDensityMatrix();
  EXPECT_EQ(state.Data(), once);
}

TEST_F(QuantumStateTest, ProbabilitiesAreTheDiagonalOfTheDensityMatrix) {
  QuantumState state(2);
  MakeBellPair(state);
  const auto pure_probabilities = state.Probabilities();
  state.ConvertToDensityMatrix();
  EXPECT_EQ(state.Probabilities().size(), 4U);
  for (std::size_t i = 0; i < 4; ++i) {
    EXPECT_NEAR(state.Probabilities()[i], pure_probabilities[i], kTol);
  }
}

}  // namespace qde::simulator::internal
