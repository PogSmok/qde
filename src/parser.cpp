#include <string>

#include <antlr4-runtime.h>

#include "qde/parser.hpp"
#include "qde/circuit.hpp"
#include "qde/parse_result.hpp"
#include "qde/syntax_error_listener.hpp"
#include "qasm3Lexer.h"
#include "qasm3Parser.h"

namespace qde {

ParseResult Parser::parse(const std::string& source) const {
  SyntaxErrorListener listener;

  antlr4::ANTLRInputStream input(source);
  qasm3Lexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  qasm3Parser parser(&tokens);

  lexer.removeErrorListeners();
  lexer.addErrorListener(&listener);
  parser.removeErrorListeners();
  parser.addErrorListener(&listener);

  parser.program();

  if (listener.hasErrors()) return ParseResult::fail(listener.errors());

  return ParseResult::ok(Circuit{});
}

}  // namespace qde