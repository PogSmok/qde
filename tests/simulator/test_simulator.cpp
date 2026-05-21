#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <complex>
#include <numeric>
#include <utility>

#include "qde/circuit.hpp"
#include "qde/gate_registry.hpp"
#include "qde/operation.hpp"
#include "qde/simulator/simulation_circuit.hpp"
#include "qde/simulator/simulator.hpp"

namespace qde {

namespace {
constexpr double kTol = 1e-9;

Circuit MakeCircuit(std::vector<QubitRegister> qregs,
                    std::vector<BitRegister> bregs,
                    std::vector<Operation> ops) {
  return {GateRegistry::WithBuiltins(), std::move(qregs), std::move(bregs),
          std::move(ops)};
}

Operation GateOp(const GateRegistry& reg, const std::string& name,
                 std::vector<QubitReference> qubits,
                 std::vector<double> params = {}) {
  return {OperationType::kGate,
          reg.Find(name),
          std::move(params),
          std::move(qubits),
          {}};
}

Operation MeasureOp(std::vector<QubitReference> qubits,
                    std::vector<BitReference> bits) {
  return {
      OperationType::kMeasure, nullptr, {}, std::move(qubits), std::move(bits)};
}

Operation ResetOp(std::vector<QubitReference> qubits) {
  return {OperationType::kReset, nullptr, {}, std::move(qubits), {}};
}
}  // namespace

class SimulatorTest : public ::testing::Test {
 protected:
  GateRegistry registry_ = GateRegistry::WithBuiltins();
  Simulator sim_;
};

// ---- Initial state
// -----------------------------------------------------------

TEST_F(SimulatorTest, InitialStateIsAllZeros) {
  auto states =
      qde::Simulator::Run(SimulationCircuit(MakeCircuit({{"q", 2}}, {}, {})));
  const auto& s = states[0];
  EXPECT_EQ(s.layer, 0U);
  EXPECT_NEAR(s.basis_probabilities[0], 1.0, kTol);
  for (std::size_t i = 1; i < 4; ++i) {
    EXPECT_NEAR(s.basis_probabilities[i], 0.0, kTol);
  }
}

TEST_F(SimulatorTest, InitialStateDensityMatrixHasSingleOne) {
  auto states =
      qde::Simulator::Run(SimulationCircuit(MakeCircuit({{"q", 1}}, {}, {})));
  const auto& rho = states[0].density_matrix;
  EXPECT_NEAR(rho[0].real(), 1.0, kTol);  // rho[0][0]
  EXPECT_NEAR(rho[1].real(), 0.0, kTol);  // rho[0][1]
  EXPECT_NEAR(rho[2].real(), 0.0, kTol);  // rho[1][0]
  EXPECT_NEAR(rho[3].real(), 0.0, kTol);  // rho[1][1]
}

// ---- Single qubit gates -----------------------------------------------------

TEST_F(SimulatorTest, HGateProducesPlusState) {
  auto op = GateOp(registry_, "h", {{0, 0}});
  auto sc = SimulationCircuit(MakeCircuit({{"q", 1}}, {}, {op}));
  auto states = qde::Simulator::Run(sc);
  const auto& s = states.back();

  // |+> = (|0>+|1>)/√2 → ρ = [[0.5, 0.5],[0.5, 0.5]]
  EXPECT_NEAR(s.basis_probabilities[0], 0.5, kTol);
  EXPECT_NEAR(s.basis_probabilities[1], 0.5, kTol);
  EXPECT_NEAR(s.density_matrix[0].real(), 0.5, kTol);  // rho[0][0]
  EXPECT_NEAR(s.density_matrix[1].real(), 0.5, kTol);  // rho[0][1]
  EXPECT_NEAR(s.density_matrix[2].real(), 0.5, kTol);  // rho[1][0]
  EXPECT_NEAR(s.density_matrix[3].real(), 0.5, kTol);  // rho[1][1]
}

TEST_F(SimulatorTest, HHIsIdentity) {
  auto sc = SimulationCircuit(MakeCircuit({{"q", 1}}, {},
                                          {
                                              GateOp(registry_, "h", {{0, 0}}),
                                              GateOp(registry_, "h", {{0, 0}}),
                                          }));
  const auto s = qde::Simulator::RunFinal(sc);
  EXPECT_NEAR(s.basis_probabilities[0], 1.0, kTol);
  EXPECT_NEAR(s.basis_probabilities[1], 0.0, kTol);
}

TEST_F(SimulatorTest, XGateFlipsQubit) {
  auto sc = SimulationCircuit(MakeCircuit({{"q", 1}}, {},
                                          {
                                              GateOp(registry_, "x", {{0, 0}}),
                                          }));
  const auto s = qde::Simulator::RunFinal(sc);
  EXPECT_NEAR(s.basis_probabilities[0], 0.0, kTol);
  EXPECT_NEAR(s.basis_probabilities[1], 1.0, kTol);
}

// ---- Two qubit gates --------------------------------------------------------

TEST_F(SimulatorTest, CXOnZeroStateNoChange) {
  // |00> with CX (control=q0, target=q1): control=0, no flip.
  auto sc = SimulationCircuit(
      MakeCircuit({{"q", 2}}, {},
                  {
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),
                  }));
  const auto s = qde::Simulator::RunFinal(sc);
  EXPECT_NEAR(s.basis_probabilities[0], 1.0, kTol);
}

TEST_F(SimulatorTest, BellStateCreation) {
  // H on q0, then CX(q0, q1) → (|00>+|11>)/√2
  // q0 = qubit 0 = bit 0 of index → |11> = index 3
  auto sc = SimulationCircuit(
      MakeCircuit({{"q", 2}}, {},
                  {
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),
                  }));
  const auto s = qde::Simulator::RunFinal(sc);
  EXPECT_NEAR(s.basis_probabilities[0], 0.5, kTol);
  EXPECT_NEAR(s.basis_probabilities[1], 0.0, kTol);
  EXPECT_NEAR(s.basis_probabilities[2], 0.0, kTol);
  EXPECT_NEAR(s.basis_probabilities[3], 0.5, kTol);
}

// ---- Eigenvalues / purity ---------------------------------------------------

TEST_F(SimulatorTest, PureStateHasUnitEigenvalue) {
  // Any pure state has exactly one eigenvalue = 1, rest = 0.
  auto sc = SimulationCircuit(MakeCircuit({{"q", 1}}, {},
                                          {
                                              GateOp(registry_, "h", {{0, 0}}),
                                          }));
  const auto s = qde::Simulator::RunFinal(sc);
  ASSERT_EQ(s.eigenvalues.size(), 2U);
  EXPECT_NEAR(s.eigenvalues[0], 1.0, kTol);
  EXPECT_NEAR(s.eigenvalues[1], 0.0, kTol);
}

TEST_F(SimulatorTest, EigenvaluesSumToOne) {
  auto sc = SimulationCircuit(
      MakeCircuit({{"q", 2}}, {},
                  {
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),
                  }));
  const auto s = qde::Simulator::RunFinal(sc);
  const double sum =
      std::accumulate(s.eigenvalues.begin(), s.eigenvalues.end(), 0.0);
  EXPECT_NEAR(sum, 1.0, kTol);
}

TEST_F(SimulatorTest, PurityOfPureStateIsOne) {
  auto sc = SimulationCircuit(
      MakeCircuit({{"q", 2}}, {},
                  {
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),
                  }));
  const auto s = qde::Simulator::RunFinal(sc);
  double purity = 0.0;
  for (double e : s.eigenvalues) {
    purity += e * e;
  }
  EXPECT_NEAR(purity, 1.0, kTol);
}

// ---- Measurement ------------------------------------------------------------

TEST_F(SimulatorTest, MeasureDefiniteZeroStateGivesZero) {
  // Qubit in |0>, measure → classical bit must be 0.
  auto sc = SimulationCircuit(MakeCircuit({{"q", 1}}, {{"c", 1}},
                                          {
                                              MeasureOp({{0, 0}}, {{0, 0}}),
                                          }));
  const auto s = qde::Simulator::RunFinal(sc);
  EXPECT_EQ(s.classical_bits[0], 0);
  EXPECT_NEAR(s.basis_probabilities[0], 1.0, kTol);
}

TEST_F(SimulatorTest, MeasureDefiniteOneStateGivesOne) {
  // X gate puts qubit in |1>, measure → classical bit must be 1.
  auto sc = SimulationCircuit(MakeCircuit({{"q", 1}}, {{"c", 1}},
                                          {
                                              GateOp(registry_, "x", {{0, 0}}),
                                              MeasureOp({{0, 0}}, {{0, 0}}),
                                          }));
  const auto s = qde::Simulator::RunFinal(sc);
  EXPECT_EQ(s.classical_bits[0], 1);
  EXPECT_NEAR(s.basis_probabilities[1], 1.0, kTol);
}

TEST_F(SimulatorTest, MeasurementCollapsesState) {
  // After measuring a superposition, state is collapsed (purity = 1).
  auto sc = SimulationCircuit(MakeCircuit({{"q", 1}}, {{"c", 1}},
                                          {
                                              GateOp(registry_, "h", {{0, 0}}),
                                              MeasureOp({{0, 0}}, {{0, 0}}),
                                          }));
  const auto s = qde::Simulator::RunFinal(sc);
  // Outcome is 0 or 1 — either way the state must be a definite basis state.
  const double p0 = s.basis_probabilities[0];
  const double p1 = s.basis_probabilities[1];
  EXPECT_TRUE(p0 > 1.0 - kTol || p1 > 1.0 - kTol);
  EXPECT_NEAR(p0 + p1, 1.0, kTol);
}

// ---- Reset ------------------------------------------------------------------

TEST_F(SimulatorTest, ResetFromOneReturnsToZero) {
  auto sc = SimulationCircuit(MakeCircuit({{"q", 1}}, {},
                                          {
                                              GateOp(registry_, "x", {{0, 0}}),
                                              ResetOp({{0, 0}}),
                                          }));
  const auto s = qde::Simulator::RunFinal(sc);
  EXPECT_NEAR(s.basis_probabilities[0], 1.0, kTol);
  EXPECT_NEAR(s.basis_probabilities[1], 0.0, kTol);
}

TEST_F(SimulatorTest, ResetFromSuperpositionReturnsToZero) {
  auto sc = SimulationCircuit(MakeCircuit({{"q", 1}}, {},
                                          {
                                              GateOp(registry_, "h", {{0, 0}}),
                                              ResetOp({{0, 0}}),
                                          }));
  const auto s = qde::Simulator::RunFinal(sc);
  EXPECT_NEAR(s.basis_probabilities[0], 1.0, kTol);
  EXPECT_NEAR(s.basis_probabilities[1], 0.0, kTol);
}

// ---- Layer count / API ------------------------------------------------------

TEST_F(SimulatorTest, RunReturnsOneStatePerLayerPlusInitial) {
  auto sc = SimulationCircuit(MakeCircuit({{"q", 1}}, {},
                                          {
                                              GateOp(registry_, "h", {{0, 0}}),
                                              GateOp(registry_, "x", {{0, 0}}),
                                          }));
  auto states = qde::Simulator::Run(sc);
  // Two gates → two layers + one initial state.
  EXPECT_EQ(states.size(), sc.Layers().size() + 1);
}

TEST_F(SimulatorTest, RunFinalMatchesLastStateOfRun) {
  auto sc = SimulationCircuit(MakeCircuit({{"q", 1}}, {},
                                          {
                                              GateOp(registry_, "h", {{0, 0}}),
                                          }));
  auto all = qde::Simulator::Run(sc);
  auto final = qde::Simulator::RunFinal(sc);
  ASSERT_EQ(all.back().basis_probabilities.size(),
            final.basis_probabilities.size());
  for (std::size_t i = 0; i < final.basis_probabilities.size(); ++i) {
    EXPECT_NEAR(all.back().basis_probabilities[i], final.basis_probabilities[i],
                kTol);
  }
}

TEST_F(SimulatorTest, LayerIndicesAreSequential) {
  auto sc = SimulationCircuit(MakeCircuit({{"q", 1}}, {},
                                          {
                                              GateOp(registry_, "h", {{0, 0}}),
                                              GateOp(registry_, "h", {{0, 0}}),
                                          }));
  auto states = qde::Simulator::Run(sc);
  for (std::size_t i = 0; i < states.size(); ++i) {
    EXPECT_EQ(states[i].layer, i);
  }
}

// ---- Conditional gates ------------------------------------------------------

TEST_F(SimulatorTest, ConditionalGateFiresWhenConditionMet) {
  // X puts qubit in |1>, measure -> c[0]=1, conditional X flips back to |0>.
  auto sc = SimulationCircuit(
      MakeCircuit({{"q", 1}}, {{"c", 1}},
                  {
                      GateOp(registry_, "x", {{0, 0}}),
                      MeasureOp({{0, 0}}, {{0, 0}}),
                      // if (c[0]==1) x q[0]
                      {OperationType::kGate,
                       registry_.Find("x"),
                       {},
                       {{0, 0}},
                       {},
                       std::make_optional(
                           std::pair<BitReference, std::uint8_t>{{0, 0}, 1})},
                  }));
  const auto s = qde::Simulator::RunFinal(sc);
  EXPECT_NEAR(s.basis_probabilities[0], 1.0, kTol);
  EXPECT_EQ(s.classical_bits[0], 1);
}

TEST_F(SimulatorTest, ConditionalGateSkippedWhenConditionNotMet) {
  // Qubit stays in |0>, measure -> c[0]=0, conditional X (if c==1) is skipped.
  auto sc = SimulationCircuit(
      MakeCircuit({{"q", 1}}, {{"c", 1}},
                  {
                      MeasureOp({{0, 0}}, {{0, 0}}),
                      // if (c[0]==1) x q[0]  — condition not met, must stay |0>
                      {OperationType::kGate,
                       registry_.Find("x"),
                       {},
                       {{0, 0}},
                       {},
                       std::make_optional(
                           std::pair<BitReference, std::uint8_t>{{0, 0}, 1})},
                  }));
  const auto s = qde::Simulator::RunFinal(sc);
  EXPECT_NEAR(s.basis_probabilities[0], 1.0, kTol);
  EXPECT_EQ(s.classical_bits[0], 0);
}

// ---- Teleportation ----------------------------------------------------------

namespace {

// Compute the 2x2 reduced density matrix of a single qubit by tracing out
// all other qubits.  Returns {rho[0][0], rho[0][1], rho[1][0], rho[1][1]}.
std::array<std::complex<double>, 4> PartialTrace(const DensityMatrix& rho,
                                                 std::size_t n,
                                                 std::size_t qubit) {
  const std::size_t d = std::size_t{1} << n;
  const std::size_t bit = std::size_t{1} << qubit;
  std::array<std::complex<double>, 4> out{};
  for (std::size_t bg = 0; bg < d; ++bg) {
    if ((bg & bit) != 0U) {
      continue;
    }
    for (std::size_t a = 0; a < 2; ++a) {
      for (std::size_t b = 0; b < 2; ++b) {
        out[(a * 2) + b] +=
            rho[((bg | (a << qubit)) * d) + (bg | (b << qubit))];
      }
    }
  }
  return out;
}

Circuit BuildTeleportationCircuit(const GateRegistry& reg) {
  constexpr double pi = 3.14159265358979323846;
  return Circuit(
      GateRegistry::WithBuiltins(), {{"q", 3}}, {{"c1", 1}, {"c2", 1}},
      {
          // Prepare state to teleport on q[0]: u3(pi/4, pi/2, pi/4)
          {OperationType::kGate,
           reg.Find("u3"),
           {pi / 4, pi / 2, pi / 4},
           {{0, 0}},
           {}},
          // Bell pair on q[1]/q[2]
          {OperationType::kGate, reg.Find("h"), {}, {{0, 1}}, {}},
          {OperationType::kGate, reg.Find("cx"), {}, {{0, 1}, {0, 2}}, {}},
          {OperationType::kBarrier, nullptr, {}, {{0, 0}, {0, 1}, {0, 2}}, {}},
          // Bell measurement
          {OperationType::kGate, reg.Find("cx"), {}, {{0, 0}, {0, 1}}, {}},
          {OperationType::kGate, reg.Find("h"), {}, {{0, 0}}, {}},
          {OperationType::kBarrier, nullptr, {}, {{0, 0}, {0, 1}, {0, 2}}, {}},
          // measure q[1]->c1, q[0]->c2
          {OperationType::kMeasure, nullptr, {}, {{0, 1}}, {{0, 0}}},
          {OperationType::kMeasure, nullptr, {}, {{0, 0}}, {{1, 0}}},
          {OperationType::kBarrier, nullptr, {}, {{0, 2}}, {}},
          // if (c1==1) x q[2]
          {OperationType::kGate,
           reg.Find("x"),
           {},
           {{0, 2}},
           {},
           std::make_optional(
               std::pair<BitReference, std::uint8_t>{{0, 0}, 1})},
          // if (c2==1) z q[2]
          {OperationType::kGate,
           reg.Find("z"),
           {},
           {{0, 2}},
           {},
           std::make_optional(
               std::pair<BitReference, std::uint8_t>{{1, 0}, 1})},
      });
}

}  // namespace

TEST_F(SimulatorTest, TeleportationPreservesState) {
  // After teleportation q[2] must hold u3(pi/4, pi/2, pi/4)|0>.
  // Reduced density matrix of that state:
  //   rho[0][0] = cos²(pi/8),  rho[1][1] = sin²(pi/8)
  //   rho[0][1] = -i * sin(pi/8)*cos(pi/8)
  constexpr double pi = 3.14159265358979323846;
  const double c2 = std::cos(pi / 8) * std::cos(pi / 8);
  const double s2 = std::sin(pi / 8) * std::sin(pi / 8);
  const double sc = std::sin(pi / 8) * std::cos(pi / 8);

  const auto s = qde::Simulator::RunFinal(
      SimulationCircuit(BuildTeleportationCircuit(registry_)));

  const auto rho_q2 = PartialTrace(s.density_matrix, 3, 2);

  EXPECT_NEAR(rho_q2[0].real(), c2, kTol);   // rho[0][0]
  EXPECT_NEAR(rho_q2[3].real(), s2, kTol);   // rho[1][1]
  EXPECT_NEAR(rho_q2[1].real(), 0.0, kTol);  // rho[0][1] real
  EXPECT_NEAR(rho_q2[1].imag(), -sc, kTol);  // rho[0][1] imag
  EXPECT_NEAR(rho_q2[2].imag(), +sc, kTol);  // rho[1][0] imag
}

TEST_F(SimulatorTest, TeleportationYieldsPureState) {
  const auto s = qde::Simulator::RunFinal(
      SimulationCircuit(BuildTeleportationCircuit(registry_)));
  // Teleportation is lossless — global state must remain pure.
  EXPECT_NEAR(s.eigenvalues[0], 1.0, kTol);
  EXPECT_NEAR(s.eigenvalues[1], 0.0, kTol);
}

// ---- GHZ state --------------------------------------------------------------

TEST_F(SimulatorTest, GHZStateHasCorrectProbabilities) {
  // H q[0]; CX(q[0],q[1]); CX(q[1],q[2]) → (|000⟩+|111⟩)/√2
  // In our encoding: qubit k = bit k → |111⟩ = index 7.
  auto sc = SimulationCircuit(
      MakeCircuit({{"q", 3}}, {},
                  {
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),
                      GateOp(registry_, "cx", {{0, 1}, {0, 2}}),
                  }));
  const auto s = qde::Simulator::RunFinal(sc);

  EXPECT_NEAR(s.basis_probabilities[0], 0.5, kTol);
  EXPECT_NEAR(s.basis_probabilities[7], 0.5, kTol);
  for (std::size_t i = 1; i < 7; ++i) {
    EXPECT_NEAR(s.basis_probabilities[i], 0.0, kTol);
  }
}

TEST_F(SimulatorTest, GHZStateIsPure) {
  auto sc = SimulationCircuit(
      MakeCircuit({{"q", 3}}, {},
                  {
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),
                      GateOp(registry_, "cx", {{0, 1}, {0, 2}}),
                  }));
  const auto s = qde::Simulator::RunFinal(sc);
  EXPECT_NEAR(s.eigenvalues[0], 1.0, kTol);
  EXPECT_NEAR(s.eigenvalues[1], 0.0, kTol);
}

// ---- Deutsch algorithm ------------------------------------------------------
// q[0] = input qubit, q[1] = ancilla initialised to |−⟩ via X then H.
// Measure q[0]: 0 = constant function, 1 = balanced function.

TEST_F(SimulatorTest, DeutschConstantFunctionMeasuresZero) {
  // f(x) = 0 — no oracle gates.
  auto sc = SimulationCircuit(
      MakeCircuit({{"q", 2}}, {{"c", 1}},
                  {
                      GateOp(registry_, "x", {{0, 1}}),  // ancilla → |1⟩
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "h", {{0, 1}}),
                      // oracle: nothing
                      GateOp(registry_, "h", {{0, 0}}),
                      MeasureOp({{0, 0}}, {{0, 0}}),
                  }));
  EXPECT_EQ(sim_.RunFinal(sc).classical_bits[0], 0);
}

TEST_F(SimulatorTest, DeutschBalancedFunctionMeasuresOne) {
  // f(x) = x — oracle is CX(q[0], q[1]).
  auto sc = SimulationCircuit(
      MakeCircuit({{"q", 2}}, {{"c", 1}},
                  {
                      GateOp(registry_, "x", {{0, 1}}),
                      GateOp(registry_, "h", {{0, 0}}),
                      GateOp(registry_, "h", {{0, 1}}),
                      GateOp(registry_, "cx", {{0, 0}, {0, 1}}),  // oracle
                      GateOp(registry_, "h", {{0, 0}}),
                      MeasureOp({{0, 0}}, {{0, 0}}),
                  }));
  EXPECT_EQ(sim_.RunFinal(sc).classical_bits[0], 1);
}

// ---- Bernstein-Vazirani (s = 0b101 = 5) -------------------------------------
// 3 input qubits + 1 ancilla. Oracle applies CX for each set bit in s.
// After the circuit, measuring the input qubits always yields s exactly.

TEST_F(SimulatorTest, BernsteinVaziraniRecoversBitstring) {
  // s = 0b101: bits 0 and 2 are set → CX(q[0], ancilla) and CX(q[2], ancilla)
  auto sc = SimulationCircuit(
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
  const auto s = qde::Simulator::RunFinal(sc);
  EXPECT_EQ(s.classical_bits[0], 1);  // bit 0 of s=101
  EXPECT_EQ(s.classical_bits[1], 0);  // bit 1 of s=101
  EXPECT_EQ(s.classical_bits[2], 1);  // bit 2 of s=101
}

}  // namespace qde
