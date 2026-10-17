#ifndef PARSER_INTERNAL_AST_TYPE_SPEC_HPP_
#define PARSER_INTERNAL_AST_TYPE_SPEC_HPP_

#include <cstdint>
#include <memory>
#include <vector>

#include "qde/parser/internal/ast/node.hpp"

namespace qde::parser::internal::ast {

// ---- types -----------------------------------------------------------------

enum class TypeSpecKind : std::uint8_t {
  kScalar,          // bit[n], int[n], float[n], complex[float[n]], bool, ...
  kArray,           // array[element, d0, d1, ...]
  kArrayReference,  // readonly/mutable array[element, d0, ...| #dim = n]
  kQubit,           // qubit, qubit[n]
};

enum class ScalarKind : std::uint8_t {
  kBit,
  kInt,
  kUint,
  kFloat,
  kAngle,
  kBool,
  kDuration,
  kStretch,
  kComplex,
};

// A type as written in the source: widths and dimensions are not evaluated.
class TypeSpec {
 public:
  virtual ~TypeSpec() = default;

  [[nodiscard]] TypeSpecKind Kind() const { return kind_; }

  SourceLocation loc;

 private:
  template <TypeSpecKind KindValue>
  friend class TypeSpecNode;

  explicit TypeSpec(TypeSpecKind kind) : kind_(kind) {}

  TypeSpecKind kind_;
};

template <TypeSpecKind KindValue>
class TypeSpecNode : public TypeSpec {
 public:
  static constexpr TypeSpecKind kKind = KindValue;

 protected:
  TypeSpecNode() : TypeSpec(KindValue) {}
};

struct ScalarTypeSpec final : TypeSpecNode<TypeSpecKind::kScalar> {
  ScalarKind scalar{ScalarKind::kBit};
  ExprPtr width;  // the designator, null if absent
  // kComplex: the type of its components, null if absent.
  std::shared_ptr<const ScalarTypeSpec> component;
};

struct ArrayTypeSpec final : TypeSpecNode<TypeSpecKind::kArray> {
  std::shared_ptr<const ScalarTypeSpec> element;
  std::vector<ExprPtr> dimensions;
};

struct ArrayReferenceTypeSpec final
    : TypeSpecNode<TypeSpecKind::kArrayReference> {
  std::shared_ptr<const ScalarTypeSpec> element;
  std::vector<ExprPtr> dimensions;  // empty if given by dimension_count
  ExprPtr dimension_count;          // `#dim = n`, null if absent
  bool is_mutable{false};
};

struct QubitTypeSpec final : TypeSpecNode<TypeSpecKind::kQubit> {
  ExprPtr width;  // the designator, null for a single qubit
};

}  // namespace qde::parser::internal::ast

#endif  // PARSER_INTERNAL_AST_TYPE_SPEC_HPP_
