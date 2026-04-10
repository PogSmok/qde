#include <string>

#include <gtest/gtest.h>
#include <antlr4-runtime.h>

#include "qasm3Lexer.h"
#include "qasm3Parser.h"
#include "qde/syntax_error_listener.hpp"

namespace {

void parse(const std::string& source, qde::SyntaxErrorListener& listener) {
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
  parse("OPENQASM 3.0;\nqubit q;\nh q;\n", listener);
  EXPECT_FALSE(listener.hasErrors());
  EXPECT_TRUE(listener.errors().empty());
}

TEST(SyntaxErrorListener, DetectsErrorOnInvalidToken) {
  qde::SyntaxErrorListener listener;
  parse("OPENQASM 3.0;\n???\n", listener);
  EXPECT_TRUE(listener.hasErrors());
}

TEST(SyntaxErrorListener, ErrorContainsLineNumber) {
  qde::SyntaxErrorListener listener;
  parse("OPENQASM 3.0;\nqubit q;\n???\n", listener);
  ASSERT_FALSE(listener.errors().empty());
  EXPECT_EQ(listener.errors().front().line, 3u);
}

TEST(SyntaxErrorListener, AccumulatesMultipleErrors) {
  qde::SyntaxErrorListener listener;
  parse("OPENQASM 3.0;\n???\n!!!\n", listener);
  EXPECT_GT(listener.errors().size(), 1u);
}
