#ifndef PARSER_INTERNAL_AST_BUILDER_HPP_
#define PARSER_INTERNAL_AST_BUILDER_HPP_

#include <memory>
#include <string>
#include <vector>

#include <antlr4-runtime.h>
#include <qasm3Parser.h>

#include "qde/parser/internal/ast.hpp"
#include "qde/parser/internal/diagnostics.hpp"

namespace qde::parser::internal {

// ---------------------------------------------------------------------------
// AstBuilder
//
// Converts the ANTLR parse tree of a program into the AST.
// The tree must be free of syntax errors.
// ---------------------------------------------------------------------------

class AstBuilder {
 public:
  explicit AstBuilder(Diagnostics& diagnostics) : diagnostics_(diagnostics) {}

  [[nodiscard]] std::shared_ptr<const ast::Program> Build(
      qasm3Parser::ProgramContext* ctx);

 private:
  // A statement whose common fields Statement() still fills in.
  using StmtNode = std::shared_ptr<ast::Stmt>;

  // A new node of type T at `loc`.
  template <typename T>
  static std::shared_ptr<T> Make(ast::SourceLocation loc) {
    auto node = std::make_shared<T>();
    node->loc = loc;
    return node;
  }

  // ---- statements ---------------------------------------------------------

  ast::StmtPtr StatementOrScope(qasm3Parser::StatementOrScopeContext* ctx);
  std::vector<ast::StmtPtr> Body(qasm3Parser::StatementOrScopeContext* ctx);
  std::vector<ast::StmtPtr> Scope(qasm3Parser::ScopeContext* ctx);
  ast::StmtPtr Statement(qasm3Parser::StatementContext* ctx);

  StmtNode Alias(qasm3Parser::AliasDeclarationStatementContext* ctx);
  StmtNode Assignment(qasm3Parser::AssignmentStatementContext* ctx);
  StmtNode Barrier(qasm3Parser::BarrierStatementContext* ctx);
  StmtNode Box(qasm3Parser::BoxStatementContext* ctx);
  StmtNode ClassicalDeclaration(
      qasm3Parser::ClassicalDeclarationStatementContext* ctx);
  StmtNode ConstDeclaration(qasm3Parser::ConstDeclarationStatementContext* ctx);
  StmtNode Subroutine(qasm3Parser::DefStatementContext* ctx);
  StmtNode Defcal(qasm3Parser::DefcalStatementContext* ctx);
  StmtNode Delay(qasm3Parser::DelayStatementContext* ctx);
  StmtNode Extern(qasm3Parser::ExternStatementContext* ctx);
  StmtNode For(qasm3Parser::ForStatementContext* ctx);
  StmtNode GateCall(qasm3Parser::GateCallStatementContext* ctx);
  StmtNode Gate(qasm3Parser::GateStatementContext* ctx);
  StmtNode If(qasm3Parser::IfStatementContext* ctx);
  StmtNode IoDeclaration(qasm3Parser::IoDeclarationStatementContext* ctx);
  StmtNode MeasureArrow(
      qasm3Parser::MeasureArrowAssignmentStatementContext* ctx);
  StmtNode OldStyleDeclaration(
      qasm3Parser::OldStyleDeclarationStatementContext* ctx);
  StmtNode QuantumDeclaration(
      qasm3Parser::QuantumDeclarationStatementContext* ctx);
  StmtNode Return(qasm3Parser::ReturnStatementContext* ctx);
  StmtNode Switch(qasm3Parser::SwitchStatementContext* ctx);
  StmtNode While(qasm3Parser::WhileStatementContext* ctx);

  // ---- expressions --------------------------------------------------------

  ast::ExprPtr Expression(qasm3Parser::ExpressionContext* ctx);
  ast::ExprPtr Literal(qasm3Parser::LiteralExpressionContext* ctx);
  ast::ExprPtr Binary(antlr4::ParserRuleContext* ctx, ast::BinaryOp op,
                      qasm3Parser::ExpressionContext* lhs,
                      qasm3Parser::ExpressionContext* rhs);
  ast::ExprPtr DeclarationExpression(
      qasm3Parser::DeclarationExpressionContext* ctx);
  ast::ExprPtr Measure(qasm3Parser::MeasureExpressionContext* ctx);
  ast::ExprPtr QuantumCall(qasm3Parser::QuantumCallExpressionContext* ctx);
  ast::ExprPtr Range(qasm3Parser::RangeExpressionContext* ctx);
  ast::ExprPtr Set(qasm3Parser::SetExpressionContext* ctx);
  ast::ExprPtr ArrayLiteral(qasm3Parser::ArrayLiteralContext* ctx);
  ast::ExprPtr AliasExpression(qasm3Parser::AliasExpressionContext* ctx);
  ast::ExprPtr IndexedIdentifier(qasm3Parser::IndexedIdentifierContext* ctx);
  ast::ExprPtr GateOperand(qasm3Parser::GateOperandContext* ctx);
  ast::ExprPtr HardwareQubit(antlr4::tree::TerminalNode* node);
  ast::ExprPtr Index(ast::ExprPtr base, qasm3Parser::IndexOperatorContext* ctx);
  ast::ExprPtr Designator(qasm3Parser::DesignatorContext* ctx);
  std::vector<ast::ExprPtr> Expressions(
      qasm3Parser::ExpressionListContext* ctx);
  std::vector<ast::ExprPtr> GateOperands(
      qasm3Parser::GateOperandListContext* ctx);

  // ---- types --------------------------------------------------------------

  std::shared_ptr<const ast::ScalarTypeSpec> ScalarType(
      qasm3Parser::ScalarTypeContext* ctx);
  ast::TypeSpecPtr ArrayType(qasm3Parser::ArrayTypeContext* ctx);
  ast::TypeSpecPtr ArrayReferenceType(
      qasm3Parser::ArrayReferenceTypeContext* ctx);
  std::shared_ptr<const ast::QubitTypeSpec> QubitType(
      ast::SourceLocation loc, qasm3Parser::DesignatorContext* designator);
  ast::TypeSpecPtr BitType(ast::SourceLocation loc,
                           qasm3Parser::DesignatorContext* designator);

  // ---- literals -----------------------------------------------------------

  std::uint64_t Integer(antlr4::tree::TerminalNode* node, std::size_t prefix,
                        unsigned base);
  double Real(antlr4::tree::TerminalNode* node, const std::string& text);

  // Reports `ctx`, a `what` the builder cannot convert into an AST node.
  // Returns where the ErrorExpr or ErrorStmt standing in for the unsupported
  // construct goes.
  [[nodiscard]] ast::SourceLocation ReportUnsupported(
      antlr4::ParserRuleContext* ctx, const std::string& what);

  // Outlives the AstBuilder, which only exists for one parse.
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
  Diagnostics& diagnostics_;
};

}  // namespace qde::parser::internal

#endif  // PARSER_INTERNAL_AST_BUILDER_HPP_
