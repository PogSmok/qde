#include <string>

#include <antlr4-runtime.h>
#include <gtest/gtest.h>

#include "qasm3Lexer.h"
#include "qasm3Parser.h"
#include "qde/syntax_error_listener.hpp"

namespace {

void Parse(const std::string& source, qde::SyntaxErrorListener& listener) {
  antlr4::ANTLRInputStream input(source);
  qasm3Lexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  qasm3Parser parser(&tokens);

  lexer.removeErrorListeners();
  lexer.addErrorListener(&listener);
  parser.removeErrorListeners();
  parser.addErrorListener(&listener);

  parser.program();
}

}  // namespace

TEST(SyntaxErrorListener, NoErrorsOnValidProgram) {
  qde::SyntaxErrorListener listener;
  Parse("OPENQASM 3.0;\nqubit q;\nh q;\n", listener);
  EXPECT_FALSE(listener.HasErrors());
  EXPECT_TRUE(listener.Errors().empty());
}

TEST(SyntaxErrorListener, DetectsErrorOnInvalidToken) {
  qde::SyntaxErrorListener listener;
  Parse("OPENQASM 3.0;\n???\n", listener);
  EXPECT_TRUE(listener.HasErrors());
}

TEST(SyntaxErrorListener, ErrorContainsLineNumber) {
  qde::SyntaxErrorListener listener;
  Parse("OPENQASM 3.0;\nqubit q;\n???\n", listener);
  ASSERT_FALSE(listener.Errors().empty());
  EXPECT_EQ(listener.Errors().front().line, 3U);
}

TEST(SyntaxErrorListener, AccumulatesMultipleErrors) {
  qde::SyntaxErrorListener listener;
  Parse("OPENQASM 3.0;\n???\n!!!\n", listener);
  EXPECT_GT(listener.Errors().size(), 1U);
}
