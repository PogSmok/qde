#include <gtest/gtest.h>

#include "qde/circuit.hpp"
#include "qde/gate_registry.hpp"
#include "qde/operation.hpp"
#include "qde/simulator/simulation_circuit.hpp"

namespace qde {

class SimulationCircuitTest : public ::testing::Test {
 protected:
  GateRegistry registry_ = GateRegistry::withBuiltins();

  // Build a gate operation using a named gate from the registry.
  Operation gateOp(const std::string& name, std::vector<QubitReference> qubits,
                   std::vector<double> params = {}) {
    return {OperationType::kGate, registry_.find(name), params, qubits, {}};
  }

  Circuit makeCircuit(std::vector<QubitRegister> qregs,
                      std::vector<BitRegister> bregs,
                      std::vector<Operation> ops) {
    return Circuit(GateRegistry::withBuiltins(), std::move(qregs),
                   std::move(bregs), std::move(ops));
  }
};

// --- Qubit and bit counts ----------------------------------------------------

TEST_F(SimulationCircuitTest, QubitCountMatchesRegisters) {
  SimulationCircuit sc(makeCircuit({{"q", 3}}, {}, {}));
  EXPECT_EQ(sc.qubitCount(), 3U);
}

TEST_F(SimulationCircuitTest, QubitCountAcrossMultipleRegisters) {
  SimulationCircuit sc(makeCircuit({{"a", 2}, {"b", 3}}, {}, {}));
  EXPECT_EQ(sc.qubitCount(), 5U);
}

TEST_F(SimulationCircuitTest, BitCountMatchesClassicalRegisters) {
  SimulationCircuit sc(makeCircuit({{"q", 2}}, {{"c", 4}}, {}));
  EXPECT_EQ(sc.bitCount(), 4U);
}

TEST_F(SimulationCircuitTest, BitCountAcrossMultipleRegisters) {
  SimulationCircuit sc(makeCircuit({{"q", 2}}, {{"a", 2}, {"b", 3}}, {}));
  EXPECT_EQ(sc.bitCount(), 5U);
}

TEST_F(SimulationCircuitTest, BitCountZeroWithNoClassicalRegisters) {
  SimulationCircuit sc(makeCircuit({{"q", 1}}, {}, {}));
  EXPECT_EQ(sc.bitCount(), 0U);
}

// --- Layer structure ---------------------------------------------------------

TEST_F(SimulationCircuitTest, EmptyCircuitHasOneEmptyLayer) {
  SimulationCircuit sc(makeCircuit({{"q", 2}}, {}, {}));
  ASSERT_EQ(sc.layers().size(), 1U);
  EXPECT_TRUE(sc.layers()[0].empty());
}

TEST_F(SimulationCircuitTest, SingleGateScheduledInLayerZero) {
  auto op = gateOp("h", {{0, 0}});
  SimulationCircuit sc(makeCircuit({{"q", 1}}, {}, {op}));
  ASSERT_GE(sc.layers().size(), 1U);
  ASSERT_EQ(sc.layers()[0].size(), 1U);
  EXPECT_EQ(sc.layers()[0][0].qubits[0], 0U);
}

TEST_F(SimulationCircuitTest, IndependentGatesScheduledInSameLayer) {
  SimulationCircuit sc(makeCircuit({{"q", 2}}, {}, {
      gateOp("h", {{0, 0}}),
      gateOp("h", {{0, 1}}),
  }));
  ASSERT_GE(sc.layers().size(), 1U);
  EXPECT_EQ(sc.layers()[0].size(), 2U);
}

TEST_F(SimulationCircuitTest, DependentGatesScheduledInDifferentLayers) {
  SimulationCircuit sc(makeCircuit({{"q", 1}}, {}, {
      gateOp("h", {{0, 0}}),
      gateOp("h", {{0, 0}}),
  }));
  ASSERT_GE(sc.layers().size(), 2U);
  EXPECT_EQ(sc.layers()[0].size(), 1U);
  EXPECT_EQ(sc.layers()[1].size(), 1U);
}

TEST_F(SimulationCircuitTest, TwoQubitGateDependencyRespected) {
  SimulationCircuit sc(makeCircuit({{"q", 2}}, {}, {
      gateOp("cx", {{0, 0}, {0, 1}}),
      gateOp("h",  {{0, 0}}),
  }));
  ASSERT_GE(sc.layers().size(), 2U);
  EXPECT_EQ(sc.layers()[0].size(), 1U);
  EXPECT_EQ(sc.layers()[1].size(), 1U);
}

// --- Compiled operation fields -----------------------------------------------

TEST_F(SimulationCircuitTest, CompiledOperationHasCorrectType) {
  auto op = gateOp("h", {{0, 0}});
  SimulationCircuit sc(makeCircuit({{"q", 1}}, {}, {op}));
  EXPECT_EQ(sc.layers()[0][0].type, OperationType::kGate);
}

TEST_F(SimulationCircuitTest, CompiledOperationHasCorrectGate) {
  auto op = gateOp("h", {{0, 0}});
  SimulationCircuit sc(makeCircuit({{"q", 1}}, {}, {op}));
  ASSERT_NE(sc.layers()[0][0].gate, nullptr);
  EXPECT_EQ(sc.layers()[0][0].gate->name(), "h");
}

TEST_F(SimulationCircuitTest, CompiledOperationQubitsAreResolvedToFlatIndex) {
  // b[0] is the 3rd qubit overall (after a[0] and a[1]).
  auto op = gateOp("h", {{1, 0}});  // reg=1 (b), qubit=0
  SimulationCircuit sc(makeCircuit({{"a", 2}, {"b", 1}}, {}, {op}));
  ASSERT_EQ(sc.layers()[0].size(), 1U);
  EXPECT_EQ(sc.layers()[0][0].qubits[0], 2U);
}

TEST_F(SimulationCircuitTest, MeasureBitTargetResolvedCorrectly) {
  // measure q[0] -> c[0], where c is the second classical register
  Operation msr{OperationType::kMeasure, nullptr, {}, {{0, 0}}, {{1, 0}}};
  SimulationCircuit sc(makeCircuit({{"q", 1}}, {{"a", 2}, {"c", 1}}, {msr}));
  ASSERT_EQ(sc.layers()[0].size(), 1U);
  EXPECT_EQ(sc.layers()[0][0].measure_target[0], 2U);
}

}  // namespace qde
