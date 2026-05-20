#include <gtest/gtest.h>

#include <utility>

#include "qde/circuit.hpp"
#include "qde/gate_registry.hpp"
#include "qde/operation.hpp"
#include "qde/simulator/simulation_circuit.hpp"

namespace qde {

class SimulationCircuitTest : public ::testing::Test {
 protected:
  GateRegistry registry_ = GateRegistry::WithBuiltins();

  // Build a gate operation using a named gate from the registry.
  Operation GateOp(const std::string& name, std::vector<QubitReference> qubits,
                   std::vector<double> params = {}) {
    return {OperationType::kGate,
            registry_.Find(name),
            std::move(params),
            std::move(qubits),
            {}};
  }

  static Circuit MakeCircuit(std::vector<QubitRegister> qregs,
                             std::vector<BitRegister> bregs,
                             std::vector<Operation> ops) {
    return {GateRegistry::WithBuiltins(), std::move(qregs), std::move(bregs),
            std::move(ops)};
  }
};

// --- Qubit and bit counts ----------------------------------------------------

TEST_F(SimulationCircuitTest, QubitCountMatchesRegisters) {
  SimulationCircuit sc(MakeCircuit({{"q", 3}}, {}, {}));
  EXPECT_EQ(sc.QubitCount(), 3U);
}

TEST_F(SimulationCircuitTest, QubitCountAcrossMultipleRegisters) {
  SimulationCircuit sc(MakeCircuit({{"a", 2}, {"b", 3}}, {}, {}));
  EXPECT_EQ(sc.QubitCount(), 5U);
}

TEST_F(SimulationCircuitTest, BitCountMatchesClassicalRegisters) {
  SimulationCircuit sc(MakeCircuit({{"q", 2}}, {{"c", 4}}, {}));
  EXPECT_EQ(sc.BitCount(), 4U);
}

TEST_F(SimulationCircuitTest, BitCountAcrossMultipleRegisters) {
  SimulationCircuit sc(MakeCircuit({{"q", 2}}, {{"a", 2}, {"b", 3}}, {}));
  EXPECT_EQ(sc.BitCount(), 5U);
}

TEST_F(SimulationCircuitTest, BitCountZeroWithNoClassicalRegisters) {
  SimulationCircuit sc(MakeCircuit({{"q", 1}}, {}, {}));
  EXPECT_EQ(sc.BitCount(), 0U);
}

// --- Layer structure ---------------------------------------------------------

TEST_F(SimulationCircuitTest, EmptyCircuitHasOneEmptyLayer) {
  SimulationCircuit sc(MakeCircuit({{"q", 2}}, {}, {}));
  ASSERT_EQ(sc.Layers().size(), 1U);
  EXPECT_TRUE(sc.Layers()[0].empty());
}

TEST_F(SimulationCircuitTest, SingleGateScheduledInLayerZero) {
  auto op = GateOp("h", {{0, 0}});
  SimulationCircuit sc(MakeCircuit({{"q", 1}}, {}, {op}));
  ASSERT_GE(sc.Layers().size(), 1U);
  ASSERT_EQ(sc.Layers()[0].size(), 1U);
  EXPECT_EQ(sc.Layers()[0][0].qubits[0], 0U);
}

TEST_F(SimulationCircuitTest, IndependentGatesScheduledInSameLayer) {
  SimulationCircuit sc(MakeCircuit({{"q", 2}}, {},
                                   {
                                       GateOp("h", {{0, 0}}),
                                       GateOp("h", {{0, 1}}),
                                   }));
  ASSERT_GE(sc.Layers().size(), 1U);
  EXPECT_EQ(sc.Layers()[0].size(), 2U);
}

TEST_F(SimulationCircuitTest, DependentGatesScheduledInDifferentLayers) {
  SimulationCircuit sc(MakeCircuit({{"q", 1}}, {},
                                   {
                                       GateOp("h", {{0, 0}}),
                                       GateOp("h", {{0, 0}}),
                                   }));
  ASSERT_GE(sc.Layers().size(), 2U);
  EXPECT_EQ(sc.Layers()[0].size(), 1U);
  EXPECT_EQ(sc.Layers()[1].size(), 1U);
}

TEST_F(SimulationCircuitTest, TwoQubitGateDependencyRespected) {
  SimulationCircuit sc(MakeCircuit({{"q", 2}}, {},
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
  auto op = GateOp("h", {{0, 0}});
  SimulationCircuit sc(MakeCircuit({{"q", 1}}, {}, {op}));
  EXPECT_EQ(sc.Layers()[0][0].type, OperationType::kGate);
}

TEST_F(SimulationCircuitTest, CompiledOperationHasCorrectGate) {
  auto op = GateOp("h", {{0, 0}});
  SimulationCircuit sc(MakeCircuit({{"q", 1}}, {}, {op}));
  ASSERT_NE(sc.Layers()[0][0].gate, nullptr);
  EXPECT_EQ(sc.Layers()[0][0].gate->Name(), "h");
}

TEST_F(SimulationCircuitTest, CompiledOperationQubitsAreResolvedToFlatIndex) {
  // b[0] is the 3rd qubit overall (after a[0] and a[1]).
  auto op = GateOp("h", {{1, 0}});  // reg=1 (b), qubit=0
  SimulationCircuit sc(MakeCircuit({{"a", 2}, {"b", 1}}, {}, {op}));
  ASSERT_EQ(sc.Layers()[0].size(), 1U);
  EXPECT_EQ(sc.Layers()[0][0].qubits[0], 2U);
}

TEST_F(SimulationCircuitTest, MeasureBitTargetResolvedCorrectly) {
  // measure q[0] -> c[0], where c is the second classical register
  Operation msr{OperationType::kMeasure, nullptr, {}, {{0, 0}}, {{1, 0}}};
  SimulationCircuit sc(MakeCircuit({{"q", 1}}, {{"a", 2}, {"c", 1}}, {msr}));
  ASSERT_EQ(sc.Layers()[0].size(), 1U);
  EXPECT_EQ(sc.Layers()[0][0].measure_target[0], 2U);
}

}  // namespace qde
