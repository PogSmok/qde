#include <cmath>
#include <complex>
#include <cstddef>

#include <gtest/gtest.h>

#include "qde/gate_registry.hpp"

using namespace qde;

class GateRegistryTest : public ::testing::Test {
 protected:
  GateRegistryTest() : reg_(GateRegistry::WithBuiltins()) {}
  GateRegistry reg_;
};

// ---- Add() ----------------------------------------------------------------

TEST_F(GateRegistryTest, FindGate) {
  auto gate = reg_.Find("x");
  ASSERT_NE(gate, nullptr);
  EXPECT_EQ(gate->Name(), "x");
}

TEST_F(GateRegistryTest, FindUnknownGateReturnsNull) {
  EXPECT_EQ(reg_.Find("nonexistent"), nullptr);
}

TEST_F(GateRegistryTest, ContainsKnownGate) { EXPECT_TRUE(reg_.Contains("h")); }

TEST_F(GateRegistryTest, ContainsUnknownGateReturnsFalse) {
  EXPECT_FALSE(reg_.Contains("nonexistent"));
}

TEST_F(GateRegistryTest, AddDuplicateThrows) {
  EXPECT_THROW(reg_.Add(GateDefinition("x", 1, {0, 1, 1, 0})),
               std::invalid_argument);
}

TEST_F(GateRegistryTest, AddDuplicateMessageContainsName) {
  try {
    reg_.Add(GateDefinition("x", 1, {0, 1, 1, 0}));
    FAIL();
  } catch (const std::invalid_argument& e) {
    EXPECT_NE(std::string(e.what()).find("'x'"), std::string::npos);
  }
}

TEST_F(GateRegistryTest, AddUserDefinedGate) {
  reg_.Add(GateDefinition("my_gate", 1, {1, 0, 0, 1}));
  EXPECT_TRUE(reg_.Contains("my_gate"));
}

// ---- WithBuiltins() -------------------------------------------------------

TEST_F(GateRegistryTest, ContainsSingleQubitGates) {
  for (const auto& name :
       {"id", "h", "x", "y", "z", "s", "sdg", "t", "tdg", "sx", "sxdg"}) {
    EXPECT_TRUE(reg_.Contains(name)) << "missing gate: " << name;
  }
}

TEST_F(GateRegistryTest, ContainsParametricSingleQubitGates) {
  for (const auto& name : {"p", "u1", "u2", "u3", "rx", "ry", "rz"}) {
    EXPECT_TRUE(reg_.Contains(name)) << "missing gate: " << name;
  }
}

TEST_F(GateRegistryTest, ContainsTwoQubitGates) {
  for (const auto& name :
       {"cx", "cz", "cy", "swap", "iswap", "cp", "crx", "cry", "crz"}) {
    EXPECT_TRUE(reg_.Contains(name)) << "missing gate: " << name;
  }
}

TEST_F(GateRegistryTest, ContainsRemainingStdgates) {
  for (const auto& name : {"ch", "cu", "CX", "phase", "cphase"}) {
    EXPECT_TRUE(reg_.Contains(name)) << "missing gate: " << name;
  }
}

TEST_F(GateRegistryTest, ContainsThreeQubitGates) {
  for (const auto& name : {"ccx", "ccz", "cswap"}) {
    EXPECT_TRUE(reg_.Contains(name)) << "missing gate: " << name;
  }
}

// ---- built-in gate correctness --------------------------------------------

TEST_F(GateRegistryTest, XGateMatrix) {
  auto mat = reg_.Find("x")->Matrix();
  EXPECT_NEAR(mat[0].real(), 0.0, 1e-10);
  EXPECT_NEAR(mat[1].real(), 1.0, 1e-10);
  EXPECT_NEAR(mat[2].real(), 1.0, 1e-10);
  EXPECT_NEAR(mat[3].real(), 0.0, 1e-10);
}

TEST_F(GateRegistryTest, HGateMatrix) {
  auto mat = reg_.Find("h")->Matrix();
  const double i_s2 = 0.7071067811865475;
  EXPECT_NEAR(mat[0].real(), i_s2, 1e-10);
  EXPECT_NEAR(mat[1].real(), i_s2, 1e-10);
  EXPECT_NEAR(mat[2].real(), i_s2, 1e-10);
  EXPECT_NEAR(mat[3].real(), -i_s2, 1e-10);
}

TEST_F(GateRegistryTest, RxGateAtZeroIsIdentity) {
  auto mat = reg_.Find("rx")->Matrix({0.0});
  EXPECT_NEAR(mat[0].real(), 1.0, 1e-10);
  EXPECT_NEAR(mat[1].real(), 0.0, 1e-10);
  EXPECT_NEAR(mat[2].real(), 0.0, 1e-10);
  EXPECT_NEAR(mat[3].real(), 1.0, 1e-10);
}

TEST_F(GateRegistryTest, CuWithZeroPhaseIsControlledU) {
  // cu(theta, phi, lambda, 0) leaves |0x> alone and applies U on |1x>.
  const auto cu = reg_.Find("cu")->Matrix({0.3, 0.5, 0.7, 0.0});
  const auto u = reg_.Find("U")->Matrix({0.3, 0.5, 0.7});
  for (std::size_t row = 0; row < 2; ++row) {
    for (std::size_t col = 0; col < 2; ++col) {
      EXPECT_NEAR(std::abs(cu[((2 + row) * 4) + 2 + col] - u[(row * 2) + col]),
                  0.0, 1e-12);
      EXPECT_NEAR(std::abs(cu[(row * 4) + col] - (row == col ? 1.0 : 0.0)), 0.0,
                  1e-12);
    }
  }
}

TEST_F(GateRegistryTest, CuPhaseMultipliesControlledBlock) {
  const double gamma = 0.9;
  const auto with_phase = reg_.Find("cu")->Matrix({0.3, 0.5, 0.7, gamma});
  const auto without = reg_.Find("cu")->Matrix({0.3, 0.5, 0.7, 0.0});
  const std::complex<double> phase = std::polar(1.0, gamma);
  EXPECT_NEAR(std::abs(with_phase[10] - (phase * without[10])), 0.0, 1e-12);
  EXPECT_NEAR(std::abs(with_phase[15] - (phase * without[15])), 0.0, 1e-12);
  EXPECT_NEAR(std::abs(with_phase[0] - 1.0), 0.0, 1e-12);
}

TEST_F(GateRegistryTest, ChGateIsControlledHadamard) {
  const auto mat = reg_.Find("ch")->Matrix();
  const double i_s2 = 0.7071067811865475;
  EXPECT_NEAR(mat[0].real(), 1.0, 1e-10);
  EXPECT_NEAR(mat[5].real(), 1.0, 1e-10);
  EXPECT_NEAR(mat[10].real(), i_s2, 1e-10);
  EXPECT_NEAR(mat[11].real(), i_s2, 1e-10);
  EXPECT_NEAR(mat[14].real(), i_s2, 1e-10);
  EXPECT_NEAR(mat[15].real(), -i_s2, 1e-10);
}

TEST_F(GateRegistryTest, CXGateMetadata) {
  auto gate = reg_.Find("cx");
  ASSERT_NE(gate, nullptr);
  EXPECT_EQ(gate->NumQubits(), 2U);
  EXPECT_EQ(gate->Matrix().size(), 16U);
}

TEST_F(GateRegistryTest, ParametricGateWrongParamCountThrows) {
  EXPECT_THROW(reg_.Find("rx")->Matrix({}), std::invalid_argument);
  EXPECT_THROW(reg_.Find("rx")->Matrix({1.0, 2.0}), std::invalid_argument);
}