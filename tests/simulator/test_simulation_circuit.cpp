#include <gtest/gtest.h>

#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "qde/circuit.hpp"
#include "qde/gate_registry.hpp"
#include "qde/operation.hpp"
#include "qde/simulator/simulation_circuit.hpp"

#include "circuit_builders.hpp"

namespace qde::simulator {

namespace {

using test_util::If;
using test_util::MakeCircuit;
using test_util::MeasureOp;

}  // namespace

class SimulationCircuitTest : public ::testing::Test {
 protected:
  // Build a gate operation using a named gate from the registry.
  [[nodiscard]] Operation GateOp(const std::string& name,
                                 std::vector<QubitReference> qubits,
                                 std::vector<double> params = {}) const {
    return test_util::GateOp(registry_, name, std::move(qubits),
                             std::move(params));
  }

 private:
  GateRegistry registry_{GateRegistry::WithBuiltins()};
};

// --- Qubit and bit counts ----------------------------------------------------

TEST_F(SimulationCircuitTest, QubitCountMatchesRegisters) {
  const SimulationCircuit sc(MakeCircuit({{"q", 3}}, {}, {}));
  EXPECT_EQ(sc.QubitCount(), 3U);
}

TEST_F(SimulationCircuitTest, QubitCountAcrossMultipleRegisters) {
  const SimulationCircuit sc(MakeCircuit({{"a", 2}, {"b", 3}}, {}, {}));
  EXPECT_EQ(sc.QubitCount(), 5U);
}

TEST_F(SimulationCircuitTest, BitCountMatchesClassicalRegisters) {
  const SimulationCircuit sc(MakeCircuit({{"q", 2}}, {{"c", 4}}, {}));
  EXPECT_EQ(sc.BitCount(), 4U);
}

TEST_F(SimulationCircuitTest, BitCountAcrossMultipleRegisters) {
  const SimulationCircuit sc(MakeCircuit({{"q", 2}}, {{"a", 2}, {"b", 3}}, {}));
  EXPECT_EQ(sc.BitCount(), 5U);
}

TEST_F(SimulationCircuitTest, BitCountZeroWithNoClassicalRegisters) {
  const SimulationCircuit sc(MakeCircuit({{"q", 1}}, {}, {}));
  EXPECT_EQ(sc.BitCount(), 0U);
}

// --- Layer structure ---------------------------------------------------------

TEST_F(SimulationCircuitTest, EmptyCircuitHasOneEmptyLayer) {
  const SimulationCircuit sc(MakeCircuit({{"q", 2}}, {}, {}));
  ASSERT_EQ(sc.Layers().size(), 1U);
  EXPECT_TRUE(sc.Layers()[0].empty());
}

TEST_F(SimulationCircuitTest, SingleGateScheduledInLayerZero) {
  const auto op = GateOp("h", {{0, 0}});
  const SimulationCircuit sc(MakeCircuit({{"q", 1}}, {}, {op}));
  ASSERT_GE(sc.Layers().size(), 1U);
  ASSERT_EQ(sc.Layers()[0].size(), 1U);
  EXPECT_EQ(sc.Layers()[0][0].qubits[0], 0U);
}

TEST_F(SimulationCircuitTest, IndependentGatesScheduledInSameLayer) {
  const SimulationCircuit sc(MakeCircuit({{"q", 2}}, {},
                                         {
                                             GateOp("h", {{0, 0}}),
                                             GateOp("h", {{0, 1}}),
                                         }));
  ASSERT_GE(sc.Layers().size(), 1U);
  EXPECT_EQ(sc.Layers()[0].size(), 2U);
}

TEST_F(SimulationCircuitTest, DependentGatesScheduledInDifferentLayers) {
  const SimulationCircuit sc(MakeCircuit({{"q", 1}}, {},
                                         {
                                             GateOp("h", {{0, 0}}),
                                             GateOp("h", {{0, 0}}),
                                         }));
  ASSERT_GE(sc.Layers().size(), 2U);
  EXPECT_EQ(sc.Layers()[0].size(), 1U);
  EXPECT_EQ(sc.Layers()[1].size(), 1U);
}

TEST_F(SimulationCircuitTest, TwoQubitGateDependencyRespected) {
  const SimulationCircuit sc(MakeCircuit({{"q", 2}}, {},
                                         {
                                             GateOp("cx", {{0, 0}, {0, 1}}),
                                             GateOp("h", {{0, 0}}),
                                         }));
  ASSERT_GE(sc.Layers().size(), 2U);
  EXPECT_EQ(sc.Layers()[0].size(), 1U);
  EXPECT_EQ(sc.Layers()[1].size(), 1U);
}

// --- Compiled operation fields -----------------------------------------------

TEST_F(SimulationCircuitTest, CompiledOperationHasCorrectType) {
  const auto op = GateOp("h", {{0, 0}});
  const SimulationCircuit sc(MakeCircuit({{"q", 1}}, {}, {op}));
  EXPECT_EQ(sc.Layers()[0][0].type, OperationType::kGate);
}

TEST_F(SimulationCircuitTest, CompiledOperationHasCorrectGate) {
  const auto op = GateOp("h", {{0, 0}});
  const SimulationCircuit sc(MakeCircuit({{"q", 1}}, {}, {op}));
  ASSERT_NE(sc.Layers()[0][0].gate, nullptr);
  EXPECT_EQ(sc.Layers()[0][0].gate->Name(), "h");
}

TEST_F(SimulationCircuitTest, CompiledOperationQubitsAreResolvedToFlatIndex) {
  // b[0] is the 3rd qubit overall (after a[0] and a[1]).
  const auto op = GateOp("h", {{1, 0}});  // reg=1 (b), qubit=0
  const SimulationCircuit sc(MakeCircuit({{"a", 2}, {"b", 1}}, {}, {op}));
  ASSERT_EQ(sc.Layers()[0].size(), 1U);
  EXPECT_EQ(sc.Layers()[0][0].qubits[0], 2U);
}

TEST_F(SimulationCircuitTest, MeasureBitTargetResolvedCorrectly) {
  // measure q[0] -> c[0], where c is the second classical register
  const Operation msr = MeasureOp({{0, 0}}, {{1, 0}});
  const SimulationCircuit sc(
      MakeCircuit({{"q", 1}}, {{"a", 2}, {"c", 1}}, {msr}));
  ASSERT_EQ(sc.Layers()[0].size(), 1U);
  EXPECT_EQ(sc.Layers()[0][0].measure_target[0], 2U);
}

// --- Limits and validation ---------------------------------------------------

TEST_F(SimulationCircuitTest, AcceptsMaxQubits) {
  const SimulationCircuit sc(MakeCircuit({{"q", kMaxQubits}}, {}, {}));
  EXPECT_EQ(sc.QubitCount(), kMaxQubits);
}

TEST_F(SimulationCircuitTest, RejectsMoreThanMaxQubits) {
  EXPECT_THROW(SimulationCircuit(MakeCircuit({{"q", kMaxQubits + 1}}, {}, {})),
               std::invalid_argument);
  EXPECT_THROW(
      SimulationCircuit(MakeCircuit({{"a", kMaxQubits}, {"b", 1}}, {}, {})),
      std::invalid_argument);
}

TEST_F(SimulationCircuitTest, RejectsQubitCountThatWouldOverflow) {
  constexpr std::size_t huge = std::numeric_limits<std::size_t>::max();
  EXPECT_THROW(SimulationCircuit(MakeCircuit({{"a", huge}, {"b", 2}}, {}, {})),
               std::invalid_argument);
}

TEST_F(SimulationCircuitTest, RejectsQubitOutsideRegister) {
  EXPECT_THROW(
      SimulationCircuit(MakeCircuit({{"q", 2}}, {}, {GateOp("h", {{0, 2}})})),
      std::invalid_argument);
  EXPECT_THROW(
      SimulationCircuit(MakeCircuit({{"q", 2}}, {}, {GateOp("h", {{1, 0}})})),
      std::invalid_argument);
}

TEST_F(SimulationCircuitTest, RejectsBitOutsideRegister) {
  const Operation msr = MeasureOp({{0, 0}}, {{0, 1}});
  EXPECT_THROW(SimulationCircuit(MakeCircuit({{"q", 1}}, {{"c", 1}}, {msr})),
               std::invalid_argument);
}

TEST_F(SimulationCircuitTest, RejectsGateOnRepeatedQubit) {
  EXPECT_THROW(SimulationCircuit(MakeCircuit({{"q", 2}}, {},
                                             {GateOp("cx", {{0, 0}, {0, 0}})})),
               std::invalid_argument);
}

TEST_F(SimulationCircuitTest, RejectsGateWithWrongQubitCount) {
  EXPECT_THROW(
      SimulationCircuit(MakeCircuit({{"q", 2}}, {}, {GateOp("cx", {{0, 0}})})),
      std::invalid_argument);
}

TEST_F(SimulationCircuitTest, RejectsGateWithoutDefinition) {
  const Operation op{OperationType::kGate, nullptr, {}, {{0, 0}}, {},
                     std::nullopt};
  EXPECT_THROW(SimulationCircuit(MakeCircuit({{"q", 1}}, {}, {op})),
               std::invalid_argument);
}

TEST_F(SimulationCircuitTest, RejectsMeasurementWithPartialTargets) {
  const Operation msr = MeasureOp({{0, 0}, {0, 1}}, {{0, 0}});
  EXPECT_THROW(SimulationCircuit(MakeCircuit({{"q", 2}}, {{"c", 2}}, {msr})),
               std::invalid_argument);
}

TEST_F(SimulationCircuitTest, AcceptsMeasurementWithoutTargets) {
  const Operation msr = MeasureOp({{0, 0}, {0, 1}}, {});
  const SimulationCircuit sc(MakeCircuit({{"q", 2}}, {}, {msr}));
  EXPECT_TRUE(sc.Layers()[0][0].measure_target.empty());
}

TEST_F(SimulationCircuitTest, RejectsMoreThanMaxBits) {
  EXPECT_NO_THROW(SimulationCircuit(MakeCircuit({}, {{"c", kMaxBits}}, {})));
  EXPECT_THROW(SimulationCircuit(MakeCircuit({}, {{"c", kMaxBits + 1}}, {})),
               std::invalid_argument);
  constexpr std::size_t huge = std::numeric_limits<std::size_t>::max();
  EXPECT_THROW(SimulationCircuit(MakeCircuit({}, {{"a", huge}, {"b", 2}}, {})),
               std::invalid_argument);
}

// --- Classical dependencies --------------------------------------------------

TEST_F(SimulationCircuitTest, ConditionWaitsForMeasurementOfItsBit) {
  // q[1] is idle, but the conditional gate on it reads c[0] and must not be
  // scheduled before the measurement that writes it.
  const SimulationCircuit sc(
      MakeCircuit({{"q", 2}}, {{"c", 1}},
                  {
                      GateOp("h", {{0, 0}}),
                      MeasureOp({{0, 0}}, {{0, 0}}),
                      If({0, 0}, 1, GateOp("x", {{0, 1}})),
                  }));
  ASSERT_EQ(sc.Layers().size(), 3U);
  EXPECT_EQ(sc.Layers()[1][0].type, OperationType::kMeasure);
  ASSERT_EQ(sc.Layers()[2].size(), 1U);
  EXPECT_TRUE(sc.Layers()[2][0].condition.has_value());
}

TEST_F(SimulationCircuitTest, MeasurementWaitsForEarlierReadOfItsBit) {
  // The second measurement overwrites c[0]; it acts on an idle qubit but must
  // stay behind the conditional gate that reads the first outcome.
  const SimulationCircuit sc(
      MakeCircuit({{"q", 3}}, {{"c", 1}},
                  {
                      MeasureOp({{0, 0}}, {{0, 0}}),
                      If({0, 0}, 1, GateOp("x", {{0, 1}})),
                      MeasureOp({{0, 2}}, {{0, 0}}),
                  }));
  ASSERT_EQ(sc.Layers().size(), 3U);
  ASSERT_EQ(sc.Layers()[2].size(), 1U);
  EXPECT_EQ(sc.Layers()[2][0].type, OperationType::kMeasure);
  EXPECT_EQ(sc.Layers()[2][0].qubits[0], 2U);
}

TEST_F(SimulationCircuitTest, IndependentConditionsShareALayer) {
  // Two gates reading the same bit do not depend on each other.
  const SimulationCircuit sc(
      MakeCircuit({{"q", 3}}, {{"c", 1}},
                  {
                      MeasureOp({{0, 0}}, {{0, 0}}),
                      If({0, 0}, 1, GateOp("x", {{0, 1}})),
                      If({0, 0}, 1, GateOp("x", {{0, 2}})),
                  }));
  ASSERT_EQ(sc.Layers().size(), 2U);
  EXPECT_EQ(sc.Layers()[1].size(), 2U);
}

}  // namespace qde::simulator
