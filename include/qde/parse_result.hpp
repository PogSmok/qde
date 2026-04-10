#ifndef PARSE_RESULT_HPP_
#define PARSE_RESULT_HPP_

#include <vector>
#include <utility>
#include <optional>
#include <cassert>

#include "qde/circuit.hpp"
#include "qde/syntax_error.hpp"

namespace qde {

class ParseResult {
public:
  static ParseResult ok(Circuit circuit) {
    return ParseResult(std::move(circuit), {});
  }

  static ParseResult fail(std::vector<SyntaxError> errors) {
    return ParseResult(std::nullopt, std::move(errors));
  }

  const Circuit& circuit() const { 
    assert(isOk() && "circuit() called on failed ParseResult");
    return *circuit_; 
  }

  const std::vector<SyntaxError>& errors() const { return errors_; } 
  bool isOk() const { return circuit_.has_value(); }

private:
  ParseResult(std::optional<Circuit> circuit, std::vector<SyntaxError> errors)
      : circuit_(std::move(circuit)), errors_(std::move(errors)) {}

  std::optional<Circuit> circuit_;
  std::vector<SyntaxError> errors_;
};

} // namespace qde

#endif // PARSE_RESULT_HPP_