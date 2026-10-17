#ifndef PARSER_INTERNAL_AST_NODE_HPP_
#define PARSER_INTERNAL_AST_NODE_HPP_

#include <cassert>
#include <cstddef>
#include <memory>

// What every node of the AST shares: where it is in the source, the
// pointers that hold it, and the casts that check its kind tag.

namespace qde::parser::internal::ast {

struct SourceLocation {
  std::size_t line{0};    // 1-indexed
  std::size_t column{0};  // 0-indexed
};

class Expr;
class Stmt;
class TypeSpec;

using ExprPtr = std::shared_ptr<const Expr>;
using StmtPtr = std::shared_ptr<const Stmt>;
using TypeSpecPtr = std::shared_ptr<const TypeSpec>;

// ---- kind tags -------------------------------------------------------------

// True iff `node` is a T.
template <typename T, typename Node>
[[nodiscard]] bool Isa(const Node& node) {
  return node.Kind() == T::kKind;
}

// `node` as a T, which it must be.
template <typename T, typename Node>
[[nodiscard]] const T& Cast(const Node& node) {
  assert(Isa<T>(node));
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  return static_cast<const T&>(node);
}

// `node` as a T, or null if it is something else.
template <typename T, typename Node>
[[nodiscard]] const T* DynCast(const Node& node) {
  return Isa<T>(node) ? &Cast<T>(node) : nullptr;
}

}  // namespace qde::parser::internal::ast

#endif  // PARSER_INTERNAL_AST_NODE_HPP_
