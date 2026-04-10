#include <gtest/gtest.h>

#include "qde/parser.hpp"

TEST(Parser, ValidProgramIsOk) {
  qde::Parser parser;
  auto result = parser.parse("OPENQASM 3.0;\nqubit q;\nh q;\n");
  EXPECT_TRUE(result.isOk());
  EXPECT_TRUE(result.errors().empty());
}

TEST(Parser, InvalidTokenFails) {
  qde::Parser parser;
  auto result = parser.parse("OPENQASM 3.0;\n???\n");
  EXPECT_FALSE(result.isOk());
  EXPECT_FALSE(result.errors().empty());
}

TEST(Parser, ErrorReportsCorrectLocation) {
  qde::Parser parser;
  auto result = parser.parse("OPENQASM 3.0;\nqubit q;\n???\n");
  ASSERT_FALSE(result.errors().empty());
  EXPECT_EQ(result.errors().front().line, 3u);
  EXPECT_EQ(result.errors().front().column, 0u);
}

// Empty source file is valid according to grammar
// An empty circuit is expected
TEST(Parser, EmptyInputIsOk) {
  qde::Parser parser;
  auto result = parser.parse("");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, MultipleErrorsAreDetected) {
  qde::Parser parser;
  auto result = parser.parse("OPENQASM 3.0;\n???\n!!!\n");
  EXPECT_GT(result.errors().size(), 1u);
}
