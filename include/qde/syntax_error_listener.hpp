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
  void syntaxError(antlr4::Recognizer*, antlr4::Token*, std::size_t line,
                    std::size_t column, const std::string& msg,
                    std::exception_ptr) override {
    errors_.push_back({line, column, msg});
  }

  const std::vector<SyntaxError>& errors() const { return errors_; }
  bool hasErrors() const { return !errors_.empty(); }

private:
  std::vector<SyntaxError> errors_;
};

} // namespace qde

#endif // SYNTAX_ERROR_LISTENER_HPP_