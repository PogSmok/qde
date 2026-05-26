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
  [[nodiscard]] static ParseResult Ok(Circuit circuit) {
    return {std::move(circuit), {}};
  }

  [[nodiscard]] static ParseResult Fail(std::vector<SyntaxError> errors) {
    assert(!errors.empty() &&
           "ParseResult::Fail() called with no errors, "
           "a failed result must explain why it failed");
    return {std::nullopt, std::move(errors)};
  }

  [[nodiscard]] const Circuit& GetCircuit() const {
    assert(IsOk() &&
           "ParseResult::GetCircuit() called on a failed result, "
           "check IsOk() before accessing the circuit");
    return *circuit_;  // NOLINT(bugprone-unchecked-optional-access)
  }

  [[nodiscard]] const std::vector<SyntaxError>& Errors() const {
    return errors_;
  }
  [[nodiscard]] bool IsOk() const { return circuit_.has_value(); }

 private:
  ParseResult(std::optional<qde::Circuit> circuit,
              std::vector<SyntaxError> errors)
      : circuit_(std::move(circuit)), errors_(std::move(errors)) {}

  std::optional<qde::Circuit> circuit_;
  std::vector<SyntaxError> errors_;
};

}  // namespace qde

#endif  // PARSE_RESULT_HPP_