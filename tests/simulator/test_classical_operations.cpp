// Conditions on several classical bits and classical assignments.

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
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

using test_util::AssignOp;
using test_util::GateOp;
using test_util::If;
using test_util::MakeCircuit;
using test_util::MeasureOp;

constexpr double kTol = 1e-9;

using Bits = std::vector<std::uint8_t>;

// True while both bits of register 0 are set.
Condition BothBits() { return {{{0, 0}, {0, 1}}, {0, 0, 0, 1}}; }

}  // namespace

class ClassicalOperationsTest : public ::testing::Test {
 protected:
  [[nodiscard]] Operation Gate(const char* name,
                               std::vector<QubitReference> qubits) const {
    return GateOp(registry_, name, std::move(qubits));
  }

  GateRegistry registry_{GateRegistry::WithBuiltins()};
};

// ---- conditions -------------------------------------------------------------

TEST_F(ClassicalOperationsTest, ConditionOnTwoBitsHolds) {
  const SimulationCircuit sc(
      MakeCircuit({{"q", 3}}, {{"c", 2}},
                  {
                      Gate("x", {{0, 0}}),
                      Gate("x", {{0, 1}}),
                      MeasureOp({{0, 0}}, {{0, 0}}),
                      MeasureOp({{0, 1}}, {{0, 1}}),
                      If(BothBits(), Gate("x", {{0, 2}})),
                  }));
  EXPECT_NEAR(Simulator::RunFinal(sc).Probability(7), 1.0, kTol);
}

TEST_F(ClassicalOperationsTest, ConditionOnTwoBitsFails) {
  const SimulationCircuit sc(
      MakeCircuit({{"q", 3}}, {{"c", 2}},
                  {
                      Gate("x", {{0, 0}}),
                      MeasureOp({{0, 0}}, {{0, 0}}),
                      MeasureOp({{0, 1}}, {{0, 1}}),
                      If(BothBits(), Gate("x", {{0, 2}})),
                  }));
  EXPECT_NEAR(Simulator::RunFinal(sc).Probability(1), 1.0, kTol);
}

TEST_F(ClassicalOperationsTest, ConstantConditions) {
  const SimulationCircuit sc(
      MakeCircuit({{"q", 2}}, {},
                  {
                      If(Condition::Constant(true), Gate("x", {{0, 0}})),
                      If(Condition::Constant(false), Gate("x", {{0, 1}})),
                  }));
  EXPECT_NEAR(Simulator::RunFinal(sc).Probability(1), 1.0, kTol);
}

TEST_F(ClassicalOperationsTest, ConditionWaitsForEveryBitItReads) {
  // The gate reads both bits and must come after both measurements, even
  // though its qubit is idle from the start.
  const SimulationCircuit sc(
      MakeCircuit({{"q", 3}}, {{"c", 2}},
                  {
                      MeasureOp({{0, 0}}, {{0, 0}}),
                      Gate("h", {{0, 1}}),
                      MeasureOp({{0, 1}}, {{0, 1}}),
                      If(BothBits(), Gate("x", {{0, 2}})),
                  }));
  ASSERT_EQ(sc.Layers().size(), 3U);
  ASSERT_EQ(sc.Layers()[2].size(), 1U);
  EXPECT_TRUE(sc.Layers()[2][0].condition.has_value());
}

TEST_F(ClassicalOperationsTest, TruthTableOfTheWrongSizeIsRejected) {
  Condition condition;
  condition.bits = {{0, 0}};
  condition.truth = {1};
  EXPECT_THROW(
      SimulationCircuit(MakeCircuit({{"q", 1}}, {{"c", 1}},
                                    {If(condition, Gate("x", {{0, 0}}))})),
      std::invalid_argument);
}

TEST_F(ClassicalOperationsTest, ConditionOnAMissingBitIsRejected) {
  EXPECT_THROW(
      SimulationCircuit(MakeCircuit({{"q", 1}}, {{"c", 1}},
                                    {If({0, 5}, 1, Gate("x", {{0, 0}}))})),
      std::invalid_argument);
}

// ---- assignments ------------------------------------------------------------

TEST_F(ClassicalOperationsTest, AssignmentWritesConstants) {
  const SimulationCircuit sc(MakeCircuit(
      {{"q", 1}}, {{"c", 2}},
      {AssignOp({{0, 0}, {0, 1}},
                {Condition::Constant(true), Condition::Constant(false)})}));
  EXPECT_EQ(Simulator::RunFinal(sc).classical_bits, (Bits{1, 0}));
}

TEST_F(ClassicalOperationsTest, AssignmentCopiesABit) {
  const SimulationCircuit sc(
      MakeCircuit({{"q", 1}}, {{"c", 2}},
                  {
                      Gate("x", {{0, 0}}),
                      MeasureOp({{0, 0}}, {{0, 0}}),
                      AssignOp({{0, 1}}, {Condition({0, 0}, 1)}),
                  }));
  EXPECT_EQ(Simulator::RunFinal(sc).classical_bits, (Bits{1, 1}));
}

TEST_F(ClassicalOperationsTest, AssignmentReadsItsTargetsBeforeWritingThem) {
  // The two bits are exchanged.
  const SimulationCircuit sc(
      MakeCircuit({{"q", 1}}, {{"c", 2}},
                  {
                      AssignOp({{0, 0}}, {Condition::Constant(true)}),
                      AssignOp({{0, 0}, {0, 1}},
                               {Condition({0, 1}, 1), Condition({0, 0}, 1)}),
                  }));
  EXPECT_EQ(Simulator::RunFinal(sc).classical_bits, (Bits{0, 1}));
}

TEST_F(ClassicalOperationsTest, GuardedAssignment) {
  const SimulationCircuit sc(MakeCircuit(
      {{"q", 1}}, {{"c", 3}},
      {
          AssignOp({{0, 0}}, {Condition::Constant(true)}),
          If({0, 0}, 1, AssignOp({{0, 1}}, {Condition::Constant(true)})),
          If({0, 0}, 0, AssignOp({{0, 2}}, {Condition::Constant(true)})),
      }));
  EXPECT_EQ(Simulator::RunFinal(sc).classical_bits, (Bits{1, 1, 0}));
}

TEST_F(ClassicalOperationsTest, AssignmentGuardsALaterGate) {
  // The gate is idle until then, but must not run before the assignment.
  const SimulationCircuit sc(
      MakeCircuit({{"q", 1}}, {{"c", 1}},
                  {
                      AssignOp({{0, 0}}, {Condition::Constant(true)}),
                      If({0, 0}, 1, Gate("x", {{0, 0}})),
                  }));
  ASSERT_EQ(sc.Layers().size(), 2U);
  EXPECT_NEAR(Simulator::RunFinal(sc).Probability(1), 1.0, kTol);
}

TEST_F(ClassicalOperationsTest, AssignmentWithoutAValueIsRejected) {
  EXPECT_THROW(SimulationCircuit(MakeCircuit({{"q", 1}}, {{"c", 1}},
                                             {AssignOp({{0, 0}}, {})})),
               std::invalid_argument);
}

}  // namespace qde::simulator
