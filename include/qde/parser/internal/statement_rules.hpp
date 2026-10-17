#ifndef PARSER_INTERNAL_STATEMENT_RULES_HPP_
#define PARSER_INTERNAL_STATEMENT_RULES_HPP_

#include <string_view>

#include "qde/parser/internal/ast.hpp"

namespace qde::parser::internal {

// The statement as it is named in an error message, such as "a gate call".
[[nodiscard]] std::string_view Describe(ast::StmtKind kind);

// Statements that declare something for the whole program.
[[nodiscard]] bool IsGlobalOnly(ast::StmtKind kind);

// A gate is a unitary: its body can call gates, arrange the calls with
// classical control flow and constants, and add what leaves the state alone
// (barrier, delay, nop, pragma). No measurements, resets or variables.
[[nodiscard]] bool IsAllowedInGate(ast::StmtKind kind);

}  // namespace qde::parser::internal

#endif  // PARSER_INTERNAL_STATEMENT_RULES_HPP_
