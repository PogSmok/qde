#ifndef SYNTAX_ERROR_LISTENER_HPP_
#define SYNTAX_ERROR_LISTENER_HPP_

#include <cstddef>
#include <string>
#include <vector>

#include <antlr4-runtime.h>

#include "qde/syntax_error.hpp"

namespace qde {

class SyntaxErrorListener : public antlr4::BaseErrorListener {
 public:
  void syntaxError(antlr4::Recognizer* /*recognizer*/,
                   antlr4::Token* /*offendingSymbol*/, std::size_t line,
                   std::size_t column, const std::string& msg,
                   std::exception_ptr /*e*/) override {
    errors_.push_back({line, column, msg});
  }

  [[nodiscard]] const std::vector<SyntaxError>& errors() const {
    return errors_;
  }
  [[nodiscard]] bool hasErrors() const { return !errors_.empty(); }

 private:
  std::vector<SyntaxError> errors_;
};

}  // namespace qde

#endif  // SYNTAX_ERROR_LISTENER_HPP_