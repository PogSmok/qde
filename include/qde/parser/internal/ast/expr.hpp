#ifndef PARSER_INTERNAL_AST_EXPR_HPP_
#define PARSER_INTERNAL_AST_EXPR_HPP_

#include <cstdint>
#include <string>
#include <vector>

#include "qde/parser/internal/ast/node.hpp"

// The expressions of the AST.

namespace qde::parser::internal::ast {

// ---- expressions -----------------------------------------------------------

enum class ExprKind : std::uint8_t {
  kIntegerLiteral,
  kFloatLiteral,
  kImaginaryLiteral,
  kBooleanLiteral,
  kBitstringLiteral,
  kTimingLiteral,
  kHardwareQubit,
  kIdentifier,
  kUnary,
  kBinary,
  kIndex,
  kCast,
  kCall,
  kDurationOf,
  kMeasure,
  kQuantumCall,
  kArrayLiteral,
  kRange,
  kSet,
  kConcatenation,
  kError,
};

enum class UnaryOp : std::uint8_t { kNegate, kBitNot, kLogicalNot };

enum class BinaryOp : std::uint8_t {
  kPower,
  kMultiply,
  kDivide,
  kModulo,
  kAdd,
  kSubtract,
  kShiftLeft,
  kShiftRight,
  kLess,
  kGreater,
  kLessEqual,
  kGreaterEqual,
  kEqual,
  kNotEqual,
  kBitAnd,
  kBitXor,
  kBitOr,
  kLogicalAnd,
  kLogicalOr,
};

enum class TimeUnit : std::uint8_t {
  kNanosecond,
  kMicrosecond,
  kMillisecond,
  kSecond,
  kDeviceCycle,  // dt
};

class Expr {
 public:
  virtual ~Expr() = default;

  [[nodiscard]] ExprKind Kind() const { return kind_; }

  SourceLocation loc;

 private:
  template <ExprKind KindValue>
  friend class ExprNode;

  explicit Expr(ExprKind kind) : kind_(kind) {}

  ExprKind kind_;
};

template <ExprKind KindValue>
class ExprNode : public Expr {
 public:
  static constexpr ExprKind kKind = KindValue;

 protected:
  ExprNode() : Expr(KindValue) {}
};

struct IntegerLiteral final : ExprNode<ExprKind::kIntegerLiteral> {
  std::uint64_t value{0};
};

struct FloatLiteral final : ExprNode<ExprKind::kFloatLiteral> {
  double value{0.0};
};

struct ImaginaryLiteral final : ExprNode<ExprKind::kImaginaryLiteral> {
  double value{0.0};  // the coefficient of `im`
};

struct BooleanLiteral final : ExprNode<ExprKind::kBooleanLiteral> {
  bool value{false};
};

struct BitstringLiteral final : ExprNode<ExprKind::kBitstringLiteral> {
  std::string digits;  // most significant first
};

struct TimingLiteral final : ExprNode<ExprKind::kTimingLiteral> {
  double value{0.0};
  TimeUnit unit{TimeUnit::kNanosecond};
};

struct HardwareQubit final : ExprNode<ExprKind::kHardwareQubit> {
  std::uint64_t number{0};
};

struct Identifier final : ExprNode<ExprKind::kIdentifier> {
  std::string name;
};

struct UnaryExpr final : ExprNode<ExprKind::kUnary> {
  UnaryOp op{UnaryOp::kNegate};
  ExprPtr operand;
};

struct BinaryExpr final : ExprNode<ExprKind::kBinary> {
  BinaryOp op{BinaryOp::kAdd};
  ExprPtr lhs;
  ExprPtr rhs;
};

struct IndexExpr final : ExprNode<ExprKind::kIndex> {
  ExprPtr base;
  // The items of one pair of brackets: expressions, RangeExpr or SetExpr.
  std::vector<ExprPtr> items;
};

struct CastExpr final : ExprNode<ExprKind::kCast> {
  TypeSpecPtr type;
  ExprPtr operand;
};

struct CallExpr final : ExprNode<ExprKind::kCall> {
  std::string callee;
  std::vector<ExprPtr> arguments;
  std::string source;  // the call as written, on one line
};

struct DurationOfExpr final : ExprNode<ExprKind::kDurationOf> {
  StmtPtr scope;  // a BlockStmt, the measured scope
};

struct MeasureExpr final : ExprNode<ExprKind::kMeasure> {
  ExprPtr qubit;
};

struct QuantumCallExpr final : ExprNode<ExprKind::kQuantumCall> {
  std::string callee;
  std::vector<ExprPtr> arguments;  // the classical arguments
  std::vector<ExprPtr> qubits;
};

struct ArrayLiteral final : ExprNode<ExprKind::kArrayLiteral> {
  std::vector<ExprPtr> elements;  // expressions or nested ArrayLiterals
};

struct RangeExpr final : ExprNode<ExprKind::kRange> {
  // Each null if absent.
  ExprPtr start;
  ExprPtr step;
  ExprPtr stop;
};

struct SetExpr final : ExprNode<ExprKind::kSet> {
  std::vector<ExprPtr> elements;
};

struct ConcatenationExpr final : ExprNode<ExprKind::kConcatenation> {
  std::vector<ExprPtr> parts;
};

// Stands in for an expression the builder could not convert, which it has
// reported. A tree with errors is not checked or run, so nothing evaluates
// this node: it only lets the builder go on and report the rest.
struct ErrorExpr final : ExprNode<ExprKind::kError> {};

}  // namespace qde::parser::internal::ast

#endif  // PARSER_INTERNAL_AST_EXPR_HPP_
