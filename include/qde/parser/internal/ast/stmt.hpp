#ifndef PARSER_INTERNAL_AST_STMT_HPP_
#define PARSER_INTERNAL_AST_STMT_HPP_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "qde/parser/internal/ast/node.hpp"
#include "qde/parser/internal/ast/type_spec.hpp"

// The statements of the AST, and the program they make up.

namespace qde::parser::internal::ast {

// ---- statements ------------------------------------------------------------

enum class StmtKind : std::uint8_t {
  kPragma,
  kInclude,
  kCalibrationGrammar,
  kCalibration,
  kCalibrationDefinition,
  kQubitDeclaration,
  kClassicalDeclaration,
  kConstDeclaration,
  kIoDeclaration,
  kAlias,
  kAssignment,
  kExpression,
  kGateCall,
  kReset,
  kBarrier,
  kDelay,
  kNop,
  kBox,
  kBlock,
  kIf,
  kFor,
  kWhile,
  kSwitch,
  kBreak,
  kContinue,
  kEnd,
  kReturn,
  kGateDefinition,
  kSubroutineDefinition,
  kExternDeclaration,
  kError,
};

struct GateModifier {
  enum class Kind : std::uint8_t { kInv, kPow, kCtrl, kNegCtrl };

  Kind kind{Kind::kInv};
  SourceLocation loc;
  ExprPtr argument;  // kPow: the exponent; controls: the count, null if absent
};

struct Parameter {
  TypeSpecPtr type;
  std::string name;  // empty for the arguments of an extern
  SourceLocation loc;
};

struct SwitchCase {
  std::vector<ExprPtr> values;  // empty for `default`
  std::vector<StmtPtr> body;
  SourceLocation loc;
  std::string source;  // as written, without the body: "case 1, 2"
};

class Stmt {
 public:
  virtual ~Stmt() = default;

  [[nodiscard]] StmtKind Kind() const { return kind_; }

  SourceLocation loc;
  std::vector<std::string> annotations;

  // The statement as written, on one line. A statement with a body has its
  // header only: "while (!c)", "for int i in [0:n]".
  std::string source;

 private:
  // The only class that sets the tag, so that it always names the class of
  // the node.
  template <StmtKind KindValue>
  friend class StmtNode;

  explicit Stmt(StmtKind kind) : kind_(kind) {}

  StmtKind kind_;
};

template <StmtKind KindValue>
class StmtNode : public Stmt {
 public:
  static constexpr StmtKind kKind = KindValue;

 protected:
  StmtNode() : Stmt(KindValue) {}
};

struct PragmaStmt final : StmtNode<StmtKind::kPragma> {
  std::string text;
};

struct IncludeStmt final : StmtNode<StmtKind::kInclude> {
  std::string path;
};

struct CalibrationGrammarStmt final : StmtNode<StmtKind::kCalibrationGrammar> {
  std::string name;
};

struct CalibrationStmt final : StmtNode<StmtKind::kCalibration> {};

struct CalibrationDefinitionStmt final
    : StmtNode<StmtKind::kCalibrationDefinition> {
  std::string target;
  TypeSpecPtr return_type;  // null if absent
};

struct QubitDeclarationStmt final : StmtNode<StmtKind::kQubitDeclaration> {
  std::string name;
  std::shared_ptr<const QubitTypeSpec> type;
};

struct ClassicalDeclarationStmt final
    : StmtNode<StmtKind::kClassicalDeclaration> {
  std::string name;
  TypeSpecPtr type;
  ExprPtr initializer;  // null if absent
};

struct ConstDeclarationStmt final : StmtNode<StmtKind::kConstDeclaration> {
  std::string name;
  TypeSpecPtr type;
  ExprPtr initializer;
};

struct IoDeclarationStmt final : StmtNode<StmtKind::kIoDeclaration> {
  std::string name;
  TypeSpecPtr type;
  bool is_input{false};  // input, not output
};

struct AliasStmt final : StmtNode<StmtKind::kAlias> {
  std::string name;
  ExprPtr value;  // the aliased expression
};

struct AssignmentStmt final : StmtNode<StmtKind::kAssignment> {
  ExprPtr target;  // the assigned l-value
  ExprPtr value;
  std::optional<BinaryOp> compound_op;  // set for the `op=` form
};

struct ExpressionStmt final : StmtNode<StmtKind::kExpression> {
  ExprPtr expression;
};

struct GateCallStmt final : StmtNode<StmtKind::kGateCall> {
  std::string name;
  bool is_gphase{false};
  std::vector<GateModifier> modifiers;
  std::vector<ExprPtr> arguments;  // the classical arguments
  ExprPtr duration;                // null if absent
  std::vector<ExprPtr> qubits;
};

struct ResetStmt final : StmtNode<StmtKind::kReset> {
  ExprPtr qubit;
};

struct BarrierStmt final : StmtNode<StmtKind::kBarrier> {
  std::vector<ExprPtr> qubits;
};

struct DelayStmt final : StmtNode<StmtKind::kDelay> {
  ExprPtr duration;
  std::vector<ExprPtr> qubits;
};

struct NopStmt final : StmtNode<StmtKind::kNop> {
  std::vector<ExprPtr> qubits;
};

struct BoxStmt final : StmtNode<StmtKind::kBox> {
  ExprPtr duration;  // null if absent
  std::vector<StmtPtr> body;
};

struct BlockStmt final : StmtNode<StmtKind::kBlock> {
  std::vector<StmtPtr> body;
};

struct IfStmt final : StmtNode<StmtKind::kIf> {
  ExprPtr condition;
  std::vector<StmtPtr> body;
  bool has_else{false};
  std::vector<StmtPtr> else_body;
};

struct ForStmt final : StmtNode<StmtKind::kFor> {
  TypeSpecPtr type;  // type of the loop variable
  std::string name;  // name of the loop variable
  ExprPtr sequence;  // a RangeExpr, a SetExpr or an expression
  std::vector<StmtPtr> body;
};

struct WhileStmt final : StmtNode<StmtKind::kWhile> {
  ExprPtr condition;
  std::vector<StmtPtr> body;
};

struct SwitchStmt final : StmtNode<StmtKind::kSwitch> {
  ExprPtr subject;
  std::vector<SwitchCase> cases;
};

struct BreakStmt final : StmtNode<StmtKind::kBreak> {};

struct ContinueStmt final : StmtNode<StmtKind::kContinue> {};

struct EndStmt final : StmtNode<StmtKind::kEnd> {};

struct ReturnStmt final : StmtNode<StmtKind::kReturn> {
  ExprPtr value;  // null if absent
};

struct GateDefinitionStmt final : StmtNode<StmtKind::kGateDefinition> {
  std::string name;
  std::vector<std::string> parameter_names;
  std::vector<std::string> qubit_names;
  std::vector<StmtPtr> body;
};

struct SubroutineDefinitionStmt final
    : StmtNode<StmtKind::kSubroutineDefinition> {
  std::string name;
  std::vector<Parameter> parameters;
  TypeSpecPtr return_type;  // null if absent
  std::vector<StmtPtr> body;
};

struct ExternDeclarationStmt final : StmtNode<StmtKind::kExternDeclaration> {
  std::string name;
  std::vector<Parameter> parameters;
  TypeSpecPtr return_type;  // null if absent
};

// Stands in for a statement the builder could not convert
struct ErrorStmt final : StmtNode<StmtKind::kError> {};

struct Program {
  std::string version;  // empty if the program has no version statement
  SourceLocation version_loc;
  std::vector<StmtPtr> statements;
};

}  // namespace qde::parser::internal::ast

#endif  // PARSER_INTERNAL_AST_STMT_HPP_
