#ifndef PARSER_INTERNAL_DIAGNOSTICS_HPP_
#define PARSER_INTERNAL_DIAGNOSTICS_HPP_

#include <algorithm>
#include <cstddef>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "qde/parser/internal/ast.hpp"
#include "qde/parser/syntax_error.hpp"

namespace qde::parser::internal {

// Collects the errors of one parse.
//
// A loop body is interpreted once per iteration, so the same error can be
// reported many times: duplicates are dropped, and the total is capped.
class Diagnostics {
 public:
  static constexpr std::size_t kMaxErrors = 200;

  void Error(ast::SourceLocation loc, std::string message) {
    if (muted_ != 0) {
      return;
    }
    ++reported_;
    if (!includes_.empty()) {
      // `loc` is a position in the included file. The editor shows the main
      // file only, so the error goes to the include statement there.
      message = "in included file '" + includes_.back().second + "', line " +
                std::to_string(loc.line) + ": " + message;
      loc = includes_.front().first;
    }
    if (errors_.size() >= kMaxErrors) {
      return;
    }
    if (!seen_.emplace(loc.line, loc.column, message).second) {
      return;
    }
    errors_.push_back({loc.line, loc.column, std::move(message)});
  }

  void EnterInclude(ast::SourceLocation loc, std::string path) {
    includes_.emplace_back(loc, std::move(path));
  }
  void LeaveInclude() { includes_.pop_back(); }

  using IncludeSite = std::pair<ast::SourceLocation, std::string>;
  [[nodiscard]] std::optional<IncludeSite> CurrentInclude() const {
    if (includes_.empty()) {
      return std::nullopt;
    }
    return IncludeSite{includes_.front().first, includes_.back().second};
  }

  void Mute() { ++muted_; }
  void Unmute() { --muted_; }

  [[nodiscard]] bool HasErrors() const { return reported_ != 0; }

  // Number of errors reported so far, duplicates included.
  // Comparing two readings tells whether the code in between failed.
  [[nodiscard]] std::size_t Count() const { return reported_; }

  // The collected errors in source order.
  [[nodiscard]] std::vector<SyntaxError> Take() {
    std::stable_sort(errors_.begin(), errors_.end(),
                     [](const SyntaxError& a, const SyntaxError& b) {
                       return std::tie(a.line, a.column) <
                              std::tie(b.line, b.column);
                     });
    seen_.clear();
    return std::move(errors_);
  }

 private:
  std::vector<SyntaxError> errors_;
  std::set<std::tuple<std::size_t, std::size_t, std::string>> seen_;
  std::vector<IncludeSite> includes_;
  std::size_t reported_{0};
  std::size_t muted_{0};
};

}  // namespace qde::parser::internal

#endif  // PARSER_INTERNAL_DIAGNOSTICS_HPP_
