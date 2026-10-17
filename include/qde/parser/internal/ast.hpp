#ifndef PARSER_INTERNAL_AST_HPP_
#define PARSER_INTERNAL_AST_HPP_

#include "qde/parser/internal/ast/expr.hpp"
#include "qde/parser/internal/ast/node.hpp"
#include "qde/parser/internal/ast/stmt.hpp"
#include "qde/parser/internal/ast/traversal.hpp"
#include "qde/parser/internal/ast/type_spec.hpp"

// ---------------------------------------------------------------------------
// Abstract syntax tree of an OpenQASM 3.0 program.
//
// Independent of ANTLR, so it outlives the parse
// Every node is immutable once built.
//
// The nodes are defined in ast/:
//   node.hpp       what every node shares, and the casts
//   type_spec.hpp  types
//   expr.hpp       expressions
//   stmt.hpp       statements and the program
//   traversal.hpp  visiting the parts of a node of any kind
// ---------------------------------------------------------------------------

#endif  // PARSER_INTERNAL_AST_HPP_
