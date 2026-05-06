#include <gtest/gtest.h>

#include "qde/gate_definition.hpp"

using namespace qde;
using C = std::complex<double>;

// ---- fixed-matrix constructor ---------------------------------------------

TEST(GateDefinition, FixedGateStoresNameAndQubits) {
  GateDefinition g("x", 1, {0, 1, 1, 0});
  EXPECT_EQ(g.name(), "x");
  EXPECT_EQ(g.numQubits(), 1U);
  EXPECT_EQ(g.numParams(), 0U);
}

TEST(GateDefinition, FixedGateMatrixReturnsCorrectValue) {
  std::vector<C> mat = {0, 1, 1, 0};
  GateDefinition g("x", 1, mat);
  EXPECT_EQ(g.matrix(), mat);
}

TEST(GateDefinition, FixedGateAcceptsEmptyParams) {
  GateDefinition g("x", 1, {0, 1, 1, 0});
  EXPECT_EQ(g.matrix({}), g.matrix());
}

TEST(GateDefinition, FixedGateZeroQubitsThrows) {
  EXPECT_THROW(GateDefinition("bad", 0, {1}), std::invalid_argument);
}

TEST(GateDefinition, FixedGateWrongMatrixSizeThrows) {
  // 1-qubit gate needs 4 elements, not 2
  EXPECT_THROW(GateDefinition("bad", 1, {1, 0}), std::invalid_argument);
}

TEST(GateDefinition, FixedGateWrongMatrixSizeMessageContainsSizes) {
  try {
    GateDefinition give_me_a_name("bad", 1, {1, 0});
    FAIL();
  } catch (const std::invalid_argument& e) {
    EXPECT_NE(std::string(e.what()).find('4'), std::string::npos);
    EXPECT_NE(std::string(e.what()).find('2'), std::string::npos);
  }
}

TEST(GateDefinition, FixedGateAcceptsCorrectMatrix) {
  std::vector<C> mat(16, C{0, 0});
  mat[0] = mat[5] = mat[10] = mat[15] = C{1, 0};  // identity
  EXPECT_NO_THROW(GateDefinition("id2", 2, mat));
}

// ---- parametric constructor -----------------------------------------------

TEST(GateDefinition, ParametricGateStoresMetadata) {
  GateDefinition g("rx", 1, 1, [](const std::vector<double>& p) {
    const double kC = std::cos(p[0] / 2.0);
    const double kS = std::sin(p[0] / 2.0);
    return std::vector<C>{{kC, 0}, {0, -kS}, {0, -kS}, {kC, 0}};
  });
  EXPECT_EQ(g.name(), "rx");
  EXPECT_EQ(g.numQubits(), 1U);
  EXPECT_EQ(g.numParams(), 1U);
}

TEST(GateDefinition, ParametricGateZeroQubitsThrows) {
  EXPECT_THROW(GateDefinition("bad", 0, 1,
                              [](const std::vector<double>&) {
                                return std::vector<C>(4);
                              }),
               std::invalid_argument);
}

TEST(GateDefinition, ParametricGateNullFnThrows) {
  EXPECT_THROW(GateDefinition("bad", 1, 1, nullptr), std::invalid_argument);
}

TEST(GateDefinition, ParametricGateNoParamsThrows) {
  EXPECT_THROW(GateDefinition("bad", 1, 0,
                              [](const std::vector<double>&) {
                                return std::vector<C>(4);
                              }),
               std::invalid_argument);
}

TEST(GateDefinition, ParametricGateEvaluatesCorrectly) {
  GateDefinition g("phase", 1, 1, [](const std::vector<double>& p) {
    return std::vector<C>{1, 0, 0, std::exp(C{0, p[0]})};
  });
  auto mat = g.matrix({0.0});
  EXPECT_NEAR(mat[0].real(), 1.0, 1e-10);
  EXPECT_NEAR(mat[3].real(), 1.0, 1e-10);
  EXPECT_NEAR(mat[3].imag(), 0.0, 1e-10);
}

// ---- matrix() param validation --------------------------------------------

TEST(GateDefinition, MatrixWrongParamCountThrows) {
  GateDefinition g("rx", 1, 1, [](const std::vector<double>& p) {
    return std::vector<C>(4);
  });
  EXPECT_THROW(g.matrix({}), std::invalid_argument);
  EXPECT_THROW(g.matrix({1.0, 2.0}), std::invalid_argument);
}

TEST(GateDefinition, MatrixWrongParamCountMessageContainsCounts) {
  GateDefinition g(
      "rx", 1, 1, [](const std::vector<double>&) { return std::vector<C>(4); });
  try {
    g.matrix({});
    FAIL();
  } catch (const std::invalid_argument& e) {
    std::string msg = e.what();
    EXPECT_NE(msg.find("expected 1"), std::string::npos);
    EXPECT_NE(msg.find("got 0"), std::string::npos);
  }
}

TEST(GateDefinition, FixedGatePassingParamsThrows) {
  GateDefinition g("x", 1, {0, 1, 1, 0});
  EXPECT_THROW(g.matrix({1.0}), std::invalid_argument);
}