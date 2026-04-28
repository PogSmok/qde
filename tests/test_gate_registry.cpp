#include <gtest/gtest.h>

#include "qde/gate_registry.hpp"

using namespace qde;

class GateRegistryTest : public ::testing::Test {
 protected:
  GateRegistryTest() : reg_(GateRegistry::withBuiltins()) {}
  GateRegistry reg_;
};

// ---- add() ----------------------------------------------------------------

TEST_F(GateRegistryTest, FindGate) {
  auto gate = reg_.find("x");
  ASSERT_NE(gate, nullptr);
  EXPECT_EQ(gate->name(), "x");
}

TEST_F(GateRegistryTest, FindUnknownGateReturnsNull) {
  EXPECT_EQ(reg_.find("nonexistent"), nullptr);
}

TEST_F(GateRegistryTest, ContainsKnownGate) { EXPECT_TRUE(reg_.contains("h")); }

TEST_F(GateRegistryTest, ContainsUnknownGateReturnsFalse) {
  EXPECT_FALSE(reg_.contains("nonexistent"));
}

TEST_F(GateRegistryTest, AddDuplicateThrows) {
  EXPECT_THROW(reg_.add(GateDefinition("x", 1, {0, 1, 1, 0})),
               std::invalid_argument);
}

TEST_F(GateRegistryTest, AddDuplicateMessageContainsName) {
  try {
    reg_.add(GateDefinition("x", 1, {0, 1, 1, 0}));
    FAIL();
  } catch (const std::invalid_argument& e) {
    EXPECT_NE(std::string(e.what()).find("'x'"), std::string::npos);
  }
}

TEST_F(GateRegistryTest, AddUserDefinedGate) {
  reg_.add(GateDefinition("my_gate", 1, {1, 0, 0, 1}));
  EXPECT_TRUE(reg_.contains("my_gate"));
}

// ---- withBuiltins() -------------------------------------------------------

TEST_F(GateRegistryTest, ContainsSingleQubitGates) {
  for (const auto& name :
       {"id", "h", "x", "y", "z", "s", "sdg", "t", "tdg", "sx", "sxdg"}) {
    EXPECT_TRUE(reg_.contains(name)) << "missing gate: " << name;
  }
}

TEST_F(GateRegistryTest, ContainsParametricSingleQubitGates) {
  for (const auto& name : {"p", "u1", "u2", "u3", "rx", "ry", "rz"}) {
    EXPECT_TRUE(reg_.contains(name)) << "missing gate: " << name;
  }
}

TEST_F(GateRegistryTest, ContainsTwoQubitGates) {
  for (const auto& name :
       {"cx", "cz", "cy", "swap", "iswap", "cp", "crx", "cry", "crz"}) {
    EXPECT_TRUE(reg_.contains(name)) << "missing gate: " << name;
  }
}

TEST_F(GateRegistryTest, ContainsThreeQubitGates) {
  for (const auto& name : {"ccx", "ccz", "cswap"}) {
    EXPECT_TRUE(reg_.contains(name)) << "missing gate: " << name;
  }
}

// ---- built-in gate correctness --------------------------------------------

TEST_F(GateRegistryTest, XGateMatrix) {
  auto mat = reg_.find("x")->matrix();
  EXPECT_NEAR(mat[0].real(), 0.0, 1e-10);
  EXPECT_NEAR(mat[1].real(), 1.0, 1e-10);
  EXPECT_NEAR(mat[2].real(), 1.0, 1e-10);
  EXPECT_NEAR(mat[3].real(), 0.0, 1e-10);
}

TEST_F(GateRegistryTest, HGateMatrix) {
  auto mat = reg_.find("h")->matrix();
  const double kIS2 = 0.7071067811865475;
  EXPECT_NEAR(mat[0].real(), kIS2, 1e-10);
  EXPECT_NEAR(mat[1].real(), kIS2, 1e-10);
  EXPECT_NEAR(mat[2].real(), kIS2, 1e-10);
  EXPECT_NEAR(mat[3].real(), -kIS2, 1e-10);
}

TEST_F(GateRegistryTest, RxGateAtZeroIsIdentity) {
  auto mat = reg_.find("rx")->matrix({0.0});
  EXPECT_NEAR(mat[0].real(), 1.0, 1e-10);
  EXPECT_NEAR(mat[1].real(), 0.0, 1e-10);
  EXPECT_NEAR(mat[2].real(), 0.0, 1e-10);
  EXPECT_NEAR(mat[3].real(), 1.0, 1e-10);
}

TEST_F(GateRegistryTest, CXGateMetadata) {
  auto gate = reg_.find("cx");
  ASSERT_NE(gate, nullptr);
  EXPECT_EQ(gate->numQubits(), 2U);
  EXPECT_EQ(gate->matrix().size(), 16U);
}

TEST_F(GateRegistryTest, ParametricGateWrongParamCountThrows) {
  EXPECT_THROW(reg_.find("rx")->matrix({}), std::invalid_argument);
  EXPECT_THROW(reg_.find("rx")->matrix({1.0, 2.0}), std::invalid_argument);
}