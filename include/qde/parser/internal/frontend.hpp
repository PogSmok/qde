#ifndef PARSER_INTERNAL_FRONTEND_HPP_
#define PARSER_INTERNAL_FRONTEND_HPP_

#include <memory>
#include <string>

#include "qde/parser/internal/ast.hpp"
#include "qde/parser/internal/diagnostics.hpp"

namespace qde::parser::internal {

// Lexes and parses `source` and returns its syntax tree.
// Returns null and reports to `diagnostics`
// if the source has a syntax error.
[[nodiscard]] std::shared_ptr<const ast::Program> ParseToAst(
    const std::string& source, Diagnostics& diagnostics);

}  // namespace qde::parser::internal

#endif  // PARSER_INTERNAL_FRONTEND_HPP_
