#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

#include "qde/circuit.hpp"
#include "qde/gate_registry.hpp"
#include "qde/operation.hpp"
#include "qde/simulator/simulation_circuit.hpp"
#include "qde/simulator/simulation_state.hpp"
#include "qde/simulator/simulator.hpp"

#include "circuit_builders.hpp"

namespace qde::simulator {

namespace {

using test_util::BarrierOp;
using test_util::GateOp;
using test_util::If;
using test_util::MakeCircuit;
using test_util::MeasureOp;
using test_util::ResetOp;

constexpr double kTol = 1e-9;
constexpr double kPi = 3.14159265358979323846;

}  // namespace

class SimulatorTest : public ::testing::Test {
 protected:
  GateRegistry registry_{GateRegistry::WithBuiltins()};
};

// ---- Initial state ----------------------------------------------------------

TEST_F(SimulatorTest, InitialStateIsAllZeros) {
  const auto states =
      Simulator::Run(SimulationCircuit(MakeCircuit({{"q", 2}}, {}, {})));
  const auto& s = states[0];
  EXPECT_EQ(s.layer, 0U);
  EXPECT_NEAR(s.Probability(0), 1.0, kTol);
  for (std::size_t i = 1; i < 4; ++i) {
    EXPECT_NEAR(s.Probability(i), 0.0, kTol);
  }
}

TEST_F(SimulatorTest, InitialStateDensityMatrixHasSingleOne) {
  const auto states =
      Simulator::Run(SimulationCircuit(MakeCircuit({{"q", 1}}, {}, {})));
  const auto& s = states[0];
  EXPECT_NEAR(s.DensityElement(0, 0).real(), 1.0, kTol);
  EXPECT_NEAR(s.DensityElement(0, 1).real(), 0.0, kTol);
  EXPECT_NEAR(s.DensityElement(1, 0).real(), 0.0, kTol);
  EXPECT_NEAR(s.DensityElement(1, 1).real(), 0.0, kTol);
}

// ---- Single qubit gates -----------------------------------------------------

TEST_F(SimulatorTest, HGateProducesPlusState) {
  const auto op = GateOp(registry_, "h", {{0, 0}});
  const auto sc = SimulationCircuit(MakeCircuit({{"q", 1}}, {}, {op}));
  const auto states = Simulator::Run(sc);
  const auto& s = states.back();

  // |+> = (|0>+|1>)/√2 → ρ = [[0.5, 0.5],[0.5, 0.5]]
  EXPECT_NEAR(s.Probability(0), 0.5, kTol);
  EXPECT_NEAR(s.Probability(1), 0.5, kTol);
  EXPECT_NEAR(s.DensityElement(0, 0).real(), 0.5, kTol);
  EXPECT_NEAR(s.DensityElement(0, 1).real(), 0.5, kTol);
  EXPECT_NEAR(s.DensityElement(1, 0).real(), 0.5, kTol);
  EXPECT_NEAR(s.DensityElement(1, 1).real(), 0.5, kTol);
}

TEST_F(SimulatorTest, HHIsIdentity) {
  const auto sc =
      SimulationCircuit(MakeCircuit({{"q", 1}}, {},
                                    {
                                        GateOp(registry_, "h", {{0, 0}}),
                                        GateOp(registry_, "h", {{0, 0}}),
                                    }));
  const auto s = Simulator::RunFinal(sc);
  EXPECT_NEAR(s.Probability(0), 1.0, kTol);
  EXPECT_NEAR(s.Probability(1), 0.0, kTol);
}

TEST_F(SimulatorTest, XGateFlipsQubit) {
  const auto sc =
      SimulationCircuit(MakeCircuit({{"q", 1}}, {},
                                    {
                                        GateOp(registry_, "x", {{0, 0}}),
                                    }));
  const auto s = Simulator::RunFinal(sc);
  EXPECT_NEAR(s.Probability(0), 0.0, kTol);
  EXPECT_NEAR(s.Probability(1), 1.0, kTol);
}

// ---- Two qubit gates --------------------------------------------------------

TEST_F(SimulatorTest, CXOnZeroStateNoChange) {
  // |00> with CX (control=q0, target=q1): control=0, no flip.
  const auto sc = SimulationCircuit(
      MakeCircuit({{"q", 2}}, {},
                  {
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),
                  }));
  const auto s = Simulator::RunFinal(sc);
  EXPECT_NEAR(s.Probability(0), 1.0, kTol);
}

TEST_F(SimulatorTest, BellStateCreation) {
  // H on q0, then CX(q0, q1) → (|00>+|11>)/√2
  // q0 = qubit 0 = bit 0 of index → |11> = index 3
  const auto sc = SimulationCircuit(
      MakeCircuit({{"q", 2}}, {},
                  {
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),
                  }));
  const auto s = Simulator::RunFinal(sc);
  EXPECT_NEAR(s.Probability(0), 0.5, kTol);
  EXPECT_NEAR(s.Probability(1), 0.0, kTol);
  EXPECT_NEAR(s.Probability(2), 0.0, kTol);
  EXPECT_NEAR(s.Probability(3), 0.5, kTol);
}

// ---- Eigenvalues / purity ---------------------------------------------------

TEST_F(SimulatorTest, PureStateHasUnitEigenvalue) {
  // Any pure state has exactly one eigenvalue = 1, rest = 0.
  const auto sc =
      SimulationCircuit(MakeCircuit({{"q", 1}}, {},
                                    {
                                        GateOp(registry_, "h", {{0, 0}}),
                                    }));
  const auto s = Simulator::RunFinal(sc);
  ASSERT_EQ(s.Eigenvalues().size(), 2U);
  EXPECT_NEAR(s.Eigenvalues()[0], 1.0, kTol);
  EXPECT_NEAR(s.Eigenvalues()[1], 0.0, kTol);
}

TEST_F(SimulatorTest, EigenvaluesSumToOne) {
  const auto sc = SimulationCircuit(
      MakeCircuit({{"q", 2}}, {},
                  {
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),
                  }));
  const auto s = Simulator::RunFinal(sc);
  const std::vector<double> eigenvalues = s.Eigenvalues();
  const double sum =
      std::accumulate(eigenvalues.begin(), eigenvalues.end(), 0.0);
  EXPECT_NEAR(sum, 1.0, kTol);
}

TEST_F(SimulatorTest, PurityOfPureStateIsOne) {
  const auto sc = SimulationCircuit(
      MakeCircuit({{"q", 2}}, {},
                  {
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),
                  }));
  const auto s = Simulator::RunFinal(sc);
  double purity = 0.0;
  for (double e : s.Eigenvalues()) {
    purity += e * e;
  }
  EXPECT_NEAR(purity, 1.0, kTol);
}

TEST_F(SimulatorTest, MixedStateEigenvaluesSurviveUnitaries) {
  const auto sc = SimulationCircuit(
      MakeCircuit({{"q", 3}}, {},
                  {
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),
                      ResetOp({{0, 0}}),
                      GateOp(registry_, "ry", {{0, 0}}, {0.7}),
                      GateOp(registry_, "rx", {{0, 1}}, {1.1}),
                      GateOp(registry_, "u3", {{0, 2}}, {0.3, 0.9, 1.7}),
                      GateOp(registry_, "cx", {{0, 0}, {0, 2}}),
                      GateOp(registry_, "cx", {{0, 1}, {0, 0}}),
                      GateOp(registry_, "rz", {{0, 0}}, {0.4}),
                      GateOp(registry_, "h", {{0, 2}}),
                      GateOp(registry_, "ry", {{0, 1}}, {1.3}),
                      GateOp(registry_, "cx", {{0, 2}, {0, 1}}),
                  }));
  const auto s = Simulator::RunFinal(sc);
  ASSERT_EQ(s.Eigenvalues().size(), 8U);
  EXPECT_NEAR(s.Eigenvalues()[0], 0.5, kTol);
  EXPECT_NEAR(s.Eigenvalues()[1], 0.5, kTol);
  for (std::size_t i = 2; i < 8; ++i) {
    EXPECT_NEAR(s.Eigenvalues()[i], 0.0, kTol);
  }
}

// ---- Measurement ------------------------------------------------------------

TEST_F(SimulatorTest, MeasureDefiniteZeroStateGivesZero) {
  // Qubit in |0>, measure → classical bit must be 0.
  const auto sc =
      SimulationCircuit(MakeCircuit({{"q", 1}}, {{"c", 1}},
                                    {
                                        MeasureOp({{0, 0}}, {{0, 0}}),
                                    }));
  const auto s = Simulator::RunFinal(sc);
  EXPECT_EQ(s.classical_bits[0], 0);
  EXPECT_NEAR(s.Probability(0), 1.0, kTol);
}

TEST_F(SimulatorTest, MeasureDefiniteOneStateGivesOne) {
  // X gate puts qubit in |1>, measure → classical bit must be 1.
  const auto sc =
      SimulationCircuit(MakeCircuit({{"q", 1}}, {{"c", 1}},
                                    {
                                        GateOp(registry_, "x", {{0, 0}}),
                                        MeasureOp({{0, 0}}, {{0, 0}}),
                                    }));
  const auto s = Simulator::RunFinal(sc);
  EXPECT_EQ(s.classical_bits[0], 1);
  EXPECT_NEAR(s.Probability(1), 1.0, kTol);
}

TEST_F(SimulatorTest, MeasurementCollapsesState) {
  // After measuring a superposition, state is collapsed (purity = 1).
  const auto sc =
      SimulationCircuit(MakeCircuit({{"q", 1}}, {{"c", 1}},
                                    {
                                        GateOp(registry_, "h", {{0, 0}}),
                                        MeasureOp({{0, 0}}, {{0, 0}}),
                                    }));
  const auto s = Simulator::RunFinal(sc);
  // Outcome is 0 or 1 — either way the state must be a definite basis state.
  const double p0 = s.Probability(0);
  const double p1 = s.Probability(1);
  EXPECT_TRUE(p0 > 1.0 - kTol || p1 > 1.0 - kTol);
  EXPECT_NEAR(p0 + p1, 1.0, kTol);
}

// ---- Reset ------------------------------------------------------------------

TEST_F(SimulatorTest, ResetFromOneReturnsToZero) {
  const auto sc =
      SimulationCircuit(MakeCircuit({{"q", 1}}, {},
                                    {
                                        GateOp(registry_, "x", {{0, 0}}),
                                        ResetOp({{0, 0}}),
                                    }));
  const auto s = Simulator::RunFinal(sc);
  EXPECT_NEAR(s.Probability(0), 1.0, kTol);
  EXPECT_NEAR(s.Probability(1), 0.0, kTol);
}

TEST_F(SimulatorTest, ResetFromSuperpositionReturnsToZero) {
  const auto sc =
      SimulationCircuit(MakeCircuit({{"q", 1}}, {},
                                    {
                                        GateOp(registry_, "h", {{0, 0}}),
                                        ResetOp({{0, 0}}),
                                    }));
  const auto s = Simulator::RunFinal(sc);
  EXPECT_NEAR(s.Probability(0), 1.0, kTol);
  EXPECT_NEAR(s.Probability(1), 0.0, kTol);
}

// ---- Layer count / API ------------------------------------------------------

TEST_F(SimulatorTest, RunReturnsOneStatePerLayerPlusInitial) {
  const auto sc =
      SimulationCircuit(MakeCircuit({{"q", 1}}, {},
                                    {
                                        GateOp(registry_, "h", {{0, 0}}),
                                        GateOp(registry_, "x", {{0, 0}}),
                                    }));
  const auto states = Simulator::Run(sc);
  // Two gates → two layers + one initial state.
  EXPECT_EQ(states.size(), sc.Layers().size() + 1);
}

TEST_F(SimulatorTest, RunFinalMatchesLastStateOfRun) {
  const auto sc =
      SimulationCircuit(MakeCircuit({{"q", 1}}, {},
                                    {
                                        GateOp(registry_, "h", {{0, 0}}),
                                    }));
  const auto all = Simulator::Run(sc);
  const auto final = Simulator::RunFinal(sc);
  ASSERT_EQ(all.back().Dim(), final.Dim());
  for (std::size_t i = 0; i < final.Dim(); ++i) {
    EXPECT_NEAR(all.back().Probability(i), final.Probability(i), kTol);
  }
}

TEST_F(SimulatorTest, LayerIndicesAreSequential) {
  const auto sc =
      SimulationCircuit(MakeCircuit({{"q", 1}}, {},
                                    {
                                        GateOp(registry_, "h", {{0, 0}}),
                                        GateOp(registry_, "h", {{0, 0}}),
                                    }));
  const auto states = Simulator::Run(sc);
  for (std::size_t i = 0; i < states.size(); ++i) {
    EXPECT_EQ(states[i].layer, i);
  }
}

// ---- Conditional gates ------------------------------------------------------

TEST_F(SimulatorTest, ConditionalGateFiresWhenConditionMet) {
  // X puts qubit in |1>, measure -> c[0]=1, conditional X flips back to |0>.
  const auto sc = SimulationCircuit(
      MakeCircuit({{"q", 1}}, {{"c", 1}},
                  {
                      GateOp(registry_, "x", {{0, 0}}),
                      MeasureOp({{0, 0}}, {{0, 0}}),
                      // if (c[0]==1) x q[0]
                      If({0, 0}, 1, GateOp(registry_, "x", {{0, 0}})),
                  }));
  const auto s = Simulator::RunFinal(sc);
  EXPECT_NEAR(s.Probability(0), 1.0, kTol);
  EXPECT_EQ(s.classical_bits[0], 1);
}

TEST_F(SimulatorTest, ConditionalGateSkippedWhenConditionNotMet) {
  // Qubit stays in |0>, measure -> c[0]=0, conditional X (if c==1) is skipped.
  const auto sc = SimulationCircuit(
      MakeCircuit({{"q", 1}}, {{"c", 1}},
                  {
                      MeasureOp({{0, 0}}, {{0, 0}}),
                      // if (c[0]==1) x q[0]  — condition not met, must stay |0>
                      If({0, 0}, 1, GateOp(registry_, "x", {{0, 0}})),
                  }));
  const auto s = Simulator::RunFinal(sc);
  EXPECT_NEAR(s.Probability(0), 1.0, kTol);
  EXPECT_EQ(s.classical_bits[0], 0);
}

TEST_F(SimulatorTest, ConditionOnIdleQubitSeesEarlierMeasurement) {
  // The conditional X acts on q[1], which nothing else touches. It must still
  // run after the measurement of q[0] that sets c[0] = 1.
  const auto sc = SimulationCircuit(
      MakeCircuit({{"q", 2}}, {{"c", 1}},
                  {
                      GateOp(registry_, "x", {{0, 0}}),
                      MeasureOp({{0, 0}}, {{0, 0}}),
                      If({0, 0}, 1, GateOp(registry_, "x", {{0, 1}})),
                  }));
  const auto s = Simulator::RunFinal(sc);
  EXPECT_EQ(s.classical_bits[0], 1);
  EXPECT_NEAR(s.Probability(0b11), 1.0, kTol);
}

// ---- Seeding ----------------------------------------------------------------

namespace {

// H on every qubit, then measure every qubit into its own bit.
Circuit BuildCoinFlips(const GateRegistry& reg, std::uint8_t qubits) {
  std::vector<Operation> ops;
  ops.reserve(std::size_t{2} * qubits);
  for (std::uint8_t q = 0; q < qubits; ++q) {
    ops.push_back(GateOp(reg, "h", {{0, q}}));
  }
  for (std::uint8_t q = 0; q < qubits; ++q) {
    ops.push_back(MeasureOp({{0, q}}, {{0, q}}));
  }
  return MakeCircuit({{"q", qubits}}, {{"c", qubits}}, std::move(ops));
}

}  // namespace

TEST_F(SimulatorTest, SameSeedReproducesMeasurements) {
  const SimulationCircuit sc(BuildCoinFlips(registry_, 12));
  const auto first = Simulator::RunFinal(sc, 1234);
  const auto second = Simulator::RunFinal(sc, 1234);
  EXPECT_EQ(first.classical_bits, second.classical_bits);
  EXPECT_EQ(Simulator::Run(sc, 1234).back().classical_bits,
            first.classical_bits);
}

TEST_F(SimulatorTest, SeedSelectsMeasurementOutcome) {
  // Over 64 seeds a fair coin shows both faces (fails with probability 2^-63).
  const SimulationCircuit sc(BuildCoinFlips(registry_, 1));
  bool saw_zero = false;
  bool saw_one = false;
  for (std::uint64_t seed = 0; seed < 64; ++seed) {
    const bool one = Simulator::RunFinal(sc, seed).classical_bits[0] == 1;
    saw_zero = saw_zero || !one;
    saw_one = saw_one || one;
  }
  EXPECT_TRUE(saw_zero);
  EXPECT_TRUE(saw_one);
}

// ---- Teleportation ----------------------------------------------------------

namespace {

// Compute the 2x2 reduced density matrix of a single qubit by tracing out
// all other qubits.  Returns {rho[0][0], rho[0][1], rho[1][0], rho[1][1]}.
std::array<std::complex<double>, 4> PartialTrace(const SimulationState& state,
                                                 std::size_t qubit) {
  const std::size_t d = std::size_t{1} << state.qubit_count;
  const std::size_t bit = std::size_t{1} << qubit;
  std::array<std::complex<double>, 4> out{};
  for (std::size_t bg = 0; bg < d; ++bg) {
    if ((bg & bit) != 0U) {
      continue;
    }
    for (std::size_t a = 0; a < 2; ++a) {
      for (std::size_t b = 0; b < 2; ++b) {
        out[(a * 2) + b] +=
            state.DensityElement(bg | (a << qubit), bg | (b << qubit));
      }
    }
  }
  return out;
}

Circuit BuildTeleportationCircuit(const GateRegistry& reg) {
  return MakeCircuit(
      {{"q", 3}}, {{"c1", 1}, {"c2", 1}},
      {
          // Prepare state to teleport on q[0]: u3(pi/4, pi/2, pi/4)
          GateOp(reg, "u3", {{0, 0}}, {kPi / 4, kPi / 2, kPi / 4}),
          // Bell pair on q[1]/q[2]
          GateOp(reg, "h", {{0, 1}}),
          GateOp(reg, "cx", {{0, 1}, {0, 2}}),
          BarrierOp({{0, 0}, {0, 1}, {0, 2}}),
          // Bell measurement
          GateOp(reg, "cx", {{0, 0}, {0, 1}}),
          GateOp(reg, "h", {{0, 0}}),
          BarrierOp({{0, 0}, {0, 1}, {0, 2}}),
          // measure q[1]->c1, q[0]->c2
          MeasureOp({{0, 1}}, {{0, 0}}),
          MeasureOp({{0, 0}}, {{1, 0}}),
          BarrierOp({{0, 2}}),
          // if (c1==1) x q[2]
          If({0, 0}, 1, GateOp(reg, "x", {{0, 2}})),
          // if (c2==1) z q[2]
          If({1, 0}, 1, GateOp(reg, "z", {{0, 2}})),
      });
}

}  // namespace

TEST_F(SimulatorTest, TeleportationPreservesState) {
  // After teleportation q[2] must hold u3(pi/4, pi/2, pi/4)|0>.
  // Reduced density matrix of that state:
  //   rho[0][0] = cos²(pi/8),  rho[1][1] = sin²(pi/8)
  //   rho[0][1] = -i * sin(pi/8)*cos(pi/8)
  const double c2 = std::cos(kPi / 8) * std::cos(kPi / 8);
  const double s2 = std::sin(kPi / 8) * std::sin(kPi / 8);
  const double sc = std::sin(kPi / 8) * std::cos(kPi / 8);

  const auto s = Simulator::RunFinal(
      SimulationCircuit(BuildTeleportationCircuit(registry_)));

  const auto rho_q2 = PartialTrace(s, 2);

  EXPECT_NEAR(rho_q2[0].real(), c2, kTol);   // rho[0][0]
  EXPECT_NEAR(rho_q2[3].real(), s2, kTol);   // rho[1][1]
  EXPECT_NEAR(rho_q2[1].real(), 0.0, kTol);  // rho[0][1] real
  EXPECT_NEAR(rho_q2[1].imag(), -sc, kTol);  // rho[0][1] imag
  EXPECT_NEAR(rho_q2[2].imag(), +sc, kTol);  // rho[1][0] imag
}

TEST_F(SimulatorTest, TeleportationYieldsPureState) {
  const auto s = Simulator::RunFinal(
      SimulationCircuit(BuildTeleportationCircuit(registry_)));
  // Teleportation is lossless — global state must remain pure.
  EXPECT_NEAR(s.Eigenvalues()[0], 1.0, kTol);
  EXPECT_NEAR(s.Eigenvalues()[1], 0.0, kTol);
}

// ---- GHZ state --------------------------------------------------------------

TEST_F(SimulatorTest, GHZStateHasCorrectProbabilities) {
  // H q[0]; CX(q[0],q[1]); CX(q[1],q[2]) → (|000⟩+|111⟩)/√2
  // In our encoding: qubit k = bit k → |111⟩ = index 7.
  const auto sc = SimulationCircuit(
      MakeCircuit({{"q", 3}}, {},
                  {
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),
                      GateOp(registry_, "cx", {{0, 1}, {0, 2}}),
                  }));
  const auto s = Simulator::RunFinal(sc);

  EXPECT_NEAR(s.Probability(0), 0.5, kTol);
  EXPECT_NEAR(s.Probability(7), 0.5, kTol);
  for (std::size_t i = 1; i < 7; ++i) {
    EXPECT_NEAR(s.Probability(i), 0.0, kTol);
  }
}

TEST_F(SimulatorTest, GHZStateIsPure) {
  const auto sc = SimulationCircuit(
      MakeCircuit({{"q", 3}}, {},
                  {
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),
                      GateOp(registry_, "cx", {{0, 1}, {0, 2}}),
                  }));
  const auto s = Simulator::RunFinal(sc);
  EXPECT_NEAR(s.Eigenvalues()[0], 1.0, kTol);
  EXPECT_NEAR(s.Eigenvalues()[1], 0.0, kTol);
}

// ---- Deutsch algorithm ------------------------------------------------------
// q[0] = input qubit, q[1] = ancilla initialised to |−⟩ via X then H.
// Measure q[0]: 0 = constant function, 1 = balanced function.

TEST_F(SimulatorTest, DeutschConstantFunctionMeasuresZero) {
  // f(x) = 0 — no oracle gates.
  const auto sc = SimulationCircuit(
      MakeCircuit({{"q", 2}}, {{"c", 1}},
                  {
                      GateOp(registry_, "x", {{0, 1}}),  // ancilla → |1⟩
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "h", {{0, 1}}),
                      // oracle: nothing
                      GateOp(registry_, "h", {{0, 0}}),
                      MeasureOp({{0, 0}}, {{0, 0}}),
                  }));
  EXPECT_EQ(Simulator::RunFinal(sc).classical_bits[0], 0);
}

TEST_F(SimulatorTest, DeutschBalancedFunctionMeasuresOne) {
  // f(x) = x — oracle is CX(q[0], q[1]).
  const auto sc = SimulationCircuit(
      MakeCircuit({{"q", 2}}, {{"c", 1}},
                  {
                      GateOp(registry_, "x", {{0, 1}}),
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "h", {{0, 1}}),
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),  // oracle
                      GateOp(registry_, "h", {{0, 0}}),
                      MeasureOp({{0, 0}}, {{0, 0}}),
                  }));
  EXPECT_EQ(Simulator::RunFinal(sc).classical_bits[0], 1);
}

// ---- Bernstein-Vazirani (s = 0b101 = 5) -------------------------------------
// 3 input qubits + 1 ancilla. Oracle applies CX for each set bit in s.
// After the circuit, measuring the input qubits always yields s exactly.

TEST_F(SimulatorTest, BernsteinVaziraniRecoversBitstring) {
  // s = 0b101: bits 0 and 2 are set → CX(q[0], ancilla) and CX(q[2], ancilla)
  const auto sc = SimulationCircuit(
      MakeCircuit({{"q", 4}}, {{"c", 3}},
                  {
                      // Ancilla q[3] → |−⟩
                      GateOp(registry_, "x", {{0, 3}}),
                      GateOp(registry_, "h", {{0, 3}}),
                      // H on input qubits
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "h", {{0, 1}}),
                      GateOp(registry_, "h", {{0, 2}}),
                      // Oracle for s=101: CX on q[0] and q[2]
                      GateOp(registry_, "cx", {{0, 0}, {0, 3}}),
                      GateOp(registry_, "cx", {{0, 2}, {0, 3}}),
                      // H on input qubits
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "h", {{0, 1}}),
                      GateOp(registry_, "h", {{0, 2}}),
                      // Measure input qubits
                      MeasureOp({{0, 0}}, {{0, 0}}),
                      MeasureOp({{0, 1}}, {{0, 1}}),
                      MeasureOp({{0, 2}}, {{0, 2}}),
                  }));
  const auto s = Simulator::RunFinal(sc);
  EXPECT_EQ(s.classical_bits[0], 1);  // bit 0 of s=101
  EXPECT_EQ(s.classical_bits[1], 0);  // bit 1 of s=101
  EXPECT_EQ(s.classical_bits[2], 1);  // bit 2 of s=101
}

// ---- State representation ---------------------------------------------------

TEST_F(SimulatorTest, UnitaryCircuitStaysStateVector) {
  const auto sc = SimulationCircuit(
      MakeCircuit({{"q", 2}}, {},
                  {
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),
                  }));
  const auto s = Simulator::RunFinal(sc);
  EXPECT_TRUE(s.IsPure());
  EXPECT_EQ(s.state_vector.size(), 4U);
  EXPECT_TRUE(s.density_matrix.empty());
  EXPECT_TRUE(s.mixed_eigenvalues.empty());
}

TEST_F(SimulatorTest, ResetOfUnentangledQubitStaysPure) {
  // q[0] is in |-> and q[1] in |+>: a product state, so reset keeps it pure.
  const auto sc =
      SimulationCircuit(MakeCircuit({{"q", 2}}, {},
                                    {
                                        GateOp(registry_, "x", {{0, 0}}),
                                        GateOp(registry_, "h", {{0, 0}}),
                                        GateOp(registry_, "h", {{0, 1}}),
                                        ResetOp({{0, 0}}),
                                    }));
  const auto s = Simulator::RunFinal(sc);
  EXPECT_TRUE(s.IsPure());
  EXPECT_NEAR(s.Probability(0), 0.5, kTol);  // |00>
  EXPECT_NEAR(s.Probability(1), 0.0, kTol);
  EXPECT_NEAR(s.Probability(2), 0.5, kTol);  // |10>
  EXPECT_NEAR(s.Probability(3), 0.0, kTol);
}

TEST_F(SimulatorTest, ResetOfEntangledQubitGivesDensityMatrix) {
  const auto sc = SimulationCircuit(
      MakeCircuit({{"q", 2}}, {},
                  {
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),
                      ResetOp({{0, 0}}),
                  }));
  const auto s = Simulator::RunFinal(sc);
  EXPECT_FALSE(s.IsPure());
  ASSERT_EQ(s.density_matrix.size(), 16U);
  // rho = |0><0| on q[0], maximally mixed on q[1].
  EXPECT_NEAR(s.Probability(0), 0.5, kTol);
  EXPECT_NEAR(s.Probability(2), 0.5, kTol);
  EXPECT_NEAR(std::abs(s.DensityElement(0, 2)), 0.0, kTol);
  EXPECT_NEAR(s.Eigenvalues()[0], 0.5, kTol);
  EXPECT_NEAR(s.Eigenvalues()[1], 0.5, kTol);
}

TEST_F(SimulatorTest, GatesAndMeasurementOnMixedState) {
  // After the reset q[1] is maximally mixed; X on q[0], CX onto q[1] and a
  // measurement of q[1] must leave a definite basis state with q[0] = 1.
  const auto sc = SimulationCircuit(
      MakeCircuit({{"q", 2}}, {{"c", 1}},
                  {
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),
                      ResetOp({{0, 0}}),
                      GateOp(registry_, "x", {{0, 0}}),
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),
                      MeasureOp({{0, 1}}, {{0, 0}}),
                  }));
  const auto s = Simulator::RunFinal(sc);
  const std::size_t expected = 1U | (std::size_t{s.classical_bits[0]} << 1);
  EXPECT_NEAR(s.Probability(expected), 1.0, kTol);
  EXPECT_NEAR(s.Eigenvalues()[0], 1.0, kTol);
}

TEST_F(SimulatorTest, MeasurementWithoutTargetBitIsSafe) {
  const auto sc =
      SimulationCircuit(MakeCircuit({{"q", 1}}, {},
                                    {
                                        GateOp(registry_, "x", {{0, 0}}),
                                        MeasureOp({{0, 0}}, {}),
                                    }));
  const auto s = Simulator::RunFinal(sc);
  EXPECT_TRUE(s.classical_bits.empty());
  EXPECT_NEAR(s.Probability(1), 1.0, kTol);
}

// ---- Scaling and limits -----------------------------------------------------

TEST_F(SimulatorTest, WideGHZState) {
  constexpr std::uint8_t qubits = 18;
  std::vector<Operation> ops{GateOp(registry_, "h", {{0, 0}})};
  for (std::uint8_t q = 0; q + 1 < qubits; ++q) {
    ops.push_back(GateOp(registry_, "cx",
                         {{0, q}, {0, static_cast<std::uint8_t>(q + 1)}}));
  }
  const auto s = Simulator::RunFinal(
      SimulationCircuit(MakeCircuit({{"q", qubits}}, {}, std::move(ops))));
  const std::size_t dim = std::size_t{1} << qubits;
  ASSERT_EQ(s.Dim(), dim);
  EXPECT_NEAR(s.Probability(0), 0.5, kTol);
  EXPECT_NEAR(s.Probability(dim - 1), 0.5, kTol);
  EXPECT_NEAR(s.Probability(1), 0.0, kTol);
}

TEST_F(SimulatorTest, MixedStateTooLargeForMemoryThrows) {
  // A 20-qubit density matrix needs 16 TiB.
  const auto sc = SimulationCircuit(
      MakeCircuit({{"q", 20}}, {},
                  {
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),
                      ResetOp({{0, 0}}),
                  }));
  EXPECT_THROW(static_cast<void>(Simulator::RunFinal(sc)), std::runtime_error);
}

}  // namespace qde::simulator
