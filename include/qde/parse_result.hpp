#ifndef PARSE_RESULT_HPP_
#define PARSE_RESULT_HPP_

#include <cassert>
#include <optional>
#include <utility>
#include <vector>

#include "qde/circuit.hpp"
#include "qde/syntax_error.hpp"

namespace qde {

class ParseResult {
 public:
  [[nodiscard]] static ParseResult ok(Circuit circuit) {
    return {std::move(circuit), {}};
  }

  [[nodiscard]] static ParseResult fail(std::vector<SyntaxError> errors) {
    assert(!errors.empty() &&
           "ParseResult::fail() called with no errors, "
           "a failed result must explain why it failed");
    return {std::nullopt, std::move(errors)};
  }

  [[nodiscard]] const Circuit& circuit() const {
    assert(isOk() &&
           "ParseResult::circuit() called on a failed result, "
           "check isOk() before accessing the circuit");
    return *circuit_;
  }

  [[nodiscard]] const std::vector<SyntaxError>& errors() const {
    return errors_;
  }
  [[nodiscard]] bool isOk() const { return circuit_.has_value(); }

 private:
  ParseResult(std::optional<Circuit> circuit, std::vector<SyntaxError> errors)
      : circuit_(std::move(circuit)), errors_(std::move(errors)) {}

  std::optional<Circuit> circuit_;
  std::vector<SyntaxError> errors_;
};

}  // namespace qde

#endif  // PARSE_RESULT_HPP_