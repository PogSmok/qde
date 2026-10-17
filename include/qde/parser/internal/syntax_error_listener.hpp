#ifndef PARSER_INTERNAL_SYNTAX_ERROR_LISTENER_HPP_
#define PARSER_INTERNAL_SYNTAX_ERROR_LISTENER_HPP_

#include <cstddef>
#include <string>
#include <vector>

#include <antlr4-runtime.h>

#include "qde/parser/syntax_error.hpp"

namespace qde::parser::internal {

// Collects the errors reported by the ANTLR lexer and parser.
class SyntaxErrorListener : public antlr4::BaseErrorListener {
 public:
  [[nodiscard]] const std::vector<SyntaxError>& Errors() const {
    return errors_;
  }
  [[nodiscard]] bool HasErrors() const { return !errors_.empty(); }

 private:
  void syntaxError(antlr4::Recognizer*, antlr4::Token* /*offendingSymbol*/,
                   std::size_t line, std::size_t column, const std::string& msg,
                   std::exception_ptr /*e*/) override {
    errors_.push_back({line, column, msg});
  }

  std::vector<SyntaxError> errors_;
};

}  // namespace qde::parser::internal

#endif  // PARSER_INTERNAL_SYNTAX_ERROR_LISTENER_HPP_
