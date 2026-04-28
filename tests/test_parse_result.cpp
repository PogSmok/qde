#include <gtest/gtest.h>

#include "qde/circuit.hpp"
#include "qde/gate_registry.hpp"
#include "qde/parse_result.hpp"
#include "qde/syntax_error.hpp"

static qde::Circuit makeEmptyCircuit() {
  return {qde::GateRegistry{}, {}, {}, {}};
}

TEST(ParseResult, OkIsOk) {
  auto result = qde::ParseResult::ok(makeEmptyCircuit());
  EXPECT_TRUE(result.isOk());
}

TEST(ParseResult, FailIsNotOk) {
  auto result = qde::ParseResult::fail({{1, 0, "error"}});
  EXPECT_FALSE(result.isOk());
}

TEST(ParseResult, OkHasNoErrors) {
  auto result = qde::ParseResult::ok(makeEmptyCircuit());
  EXPECT_TRUE(result.errors().empty());
}

TEST(ParseResult, FailStoresErrors) {
  std::vector<qde::SyntaxError> errors = {{1, 0, "unexpected token"}};
  auto result = qde::ParseResult::fail(errors);
  ASSERT_EQ(result.errors().size(), 1U);
  EXPECT_EQ(result.errors().front().message, "unexpected token");
  EXPECT_EQ(result.errors().front().line, 1U);
  EXPECT_EQ(result.errors().front().column, 0U);
}

TEST(ParseResult, OkCircuitIsAccessible) {
  auto result = qde::ParseResult::ok(makeEmptyCircuit());
  EXPECT_NO_FATAL_FAILURE(result.circuit());
}

TEST(ParseResult, FailCircuitAssertsInDebug) {
  auto result = qde::ParseResult::fail({{1, 0, "error"}});
  EXPECT_DEBUG_DEATH(result.circuit(),
                     "ParseResult::circuit\\(\\) called on a failed result");
}

TEST(ParseResult, FailWithNoErrorsAssertsInDebug) {
  EXPECT_DEBUG_DEATH(qde::ParseResult::fail({}),
                     "ParseResult::fail\\(\\) called with no errors");
}
