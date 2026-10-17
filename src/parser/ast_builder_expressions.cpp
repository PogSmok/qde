// Expressions: operators, calls, indices and the operands of gates.

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <antlr4-runtime.h>
#include <qasm3Parser.h>

#include "qde/parser/internal/ast.hpp"
#include "qde/parser/internal/ast_builder.hpp"
#include "qde/parser/internal/parse_tree_text.hpp"

namespace qde::parser::internal {

namespace {

using ast::BinaryOp;

BinaryOp ComparisonOperator(std::string_view text) {
  if (text == "<") {
    return BinaryOp::kLess;
  }
  if (text == ">") {
    return BinaryOp::kGreater;
  }
  if (text == "<=") {
    return BinaryOp::kLessEqual;
  }
  return BinaryOp::kGreaterEqual;
}

}  // namespace

// ---- expressions -----------------------------------------------------------

ast::ExprPtr AstBuilder::Binary(antlr4::ParserRuleContext* ctx, BinaryOp op,
                                qasm3Parser::ExpressionContext* lhs,
                                qasm3Parser::ExpressionContext* rhs) {
  auto expr = Make<ast::BinaryExpr>(Loc(ctx));
  expr->op = op;
  expr->lhs = Expression(lhs);
  expr->rhs = Expression(rhs);
  return expr;
}

ast::ExprPtr AstBuilder::Expression(qasm3Parser::ExpressionContext* ctx) {
  using P = qasm3Parser;

  if (auto* e = dynamic_cast<P::ParenthesisExpressionContext*>(ctx)) {
    return Expression(e->expression());
  }
  if (auto* e = dynamic_cast<P::IndexExpressionContext*>(ctx)) {
    return Index(Expression(e->expression()), e->indexOperator());
  }
  if (auto* e = dynamic_cast<P::PowerExpressionContext*>(ctx)) {
    return Binary(e, BinaryOp::kPower, e->expression(0), e->expression(1));
  }
  if (auto* e = dynamic_cast<P::UnaryExpressionContext*>(ctx)) {
    auto expr = Make<ast::UnaryExpr>(Loc(ctx));
    if (e->TILDE() != nullptr) {
      expr->op = ast::UnaryOp::kBitNot;
    } else if (e->EXCLAMATION_POINT() != nullptr) {
      expr->op = ast::UnaryOp::kLogicalNot;
    } else {
      expr->op = ast::UnaryOp::kNegate;
    }
    expr->operand = Expression(e->expression());
    return expr;
  }
  if (auto* e = dynamic_cast<P::MultiplicativeExpressionContext*>(ctx)) {
    BinaryOp op = BinaryOp::kModulo;
    if (e->ASTERISK() != nullptr) {
      op = BinaryOp::kMultiply;
    } else if (e->SLASH() != nullptr) {
      op = BinaryOp::kDivide;
    }
    return Binary(e, op, e->expression(0), e->expression(1));
  }
  if (auto* e = dynamic_cast<P::AdditiveExpressionContext*>(ctx)) {
    return Binary(e,
                  e->PLUS() != nullptr ? BinaryOp::kAdd : BinaryOp::kSubtract,
                  e->expression(0), e->expression(1));
  }
  if (auto* e = dynamic_cast<P::BitshiftExpressionContext*>(ctx)) {
    return Binary(
        e,
        e->op->getText() == "<<" ? BinaryOp::kShiftLeft : BinaryOp::kShiftRight,
        e->expression(0), e->expression(1));
  }
  if (auto* e = dynamic_cast<P::ComparisonExpressionContext*>(ctx)) {
    return Binary(e, ComparisonOperator(e->op->getText()), e->expression(0),
                  e->expression(1));
  }
  if (auto* e = dynamic_cast<P::EqualityExpressionContext*>(ctx)) {
    return Binary(
        e, e->op->getText() == "==" ? BinaryOp::kEqual : BinaryOp::kNotEqual,
        e->expression(0), e->expression(1));
  }
  if (auto* e = dynamic_cast<P::BitwiseAndExpressionContext*>(ctx)) {
    return Binary(e, BinaryOp::kBitAnd, e->expression(0), e->expression(1));
  }
  if (auto* e = dynamic_cast<P::BitwiseXorExpressionContext*>(ctx)) {
    return Binary(e, BinaryOp::kBitXor, e->expression(0), e->expression(1));
  }
  if (auto* e = dynamic_cast<P::BitwiseOrExpressionContext*>(ctx)) {
    return Binary(e, BinaryOp::kBitOr, e->expression(0), e->expression(1));
  }
  if (auto* e = dynamic_cast<P::LogicalAndExpressionContext*>(ctx)) {
    return Binary(e, BinaryOp::kLogicalAnd, e->expression(0), e->expression(1));
  }
  if (auto* e = dynamic_cast<P::LogicalOrExpressionContext*>(ctx)) {
    return Binary(e, BinaryOp::kLogicalOr, e->expression(0), e->expression(1));
  }
  if (auto* e = dynamic_cast<P::CastExpressionContext*>(ctx)) {
    auto expr = Make<ast::CastExpr>(Loc(ctx));
    expr->type = e->scalarType() != nullptr ? ScalarType(e->scalarType())
                                            : ArrayType(e->arrayType());
    expr->operand = Expression(e->expression());
    return expr;
  }
  if (auto* e = dynamic_cast<P::DurationofExpressionContext*>(ctx)) {
    auto expr = Make<ast::DurationOfExpr>(Loc(ctx));
    auto block = Make<ast::BlockStmt>(Loc(e->scope()));
    block->body = Scope(e->scope());
    expr->scope = std::move(block);
    return expr;
  }
  if (auto* e = dynamic_cast<P::CallExpressionContext*>(ctx)) {
    auto expr = Make<ast::CallExpr>(Loc(ctx));
    expr->callee = e->Identifier()->getText();
    expr->source = SourceText(e);
    expr->arguments = Expressions(e->expressionList());
    return expr;
  }
  if (auto* e = dynamic_cast<P::LiteralExpressionContext*>(ctx)) {
    return Literal(e);
  }
  return Make<ast::ErrorExpr>(ReportUnsupported(ctx, "expression"));
}

ast::ExprPtr AstBuilder::DeclarationExpression(
    qasm3Parser::DeclarationExpressionContext* ctx) {
  if (auto* literal = ctx->arrayLiteral()) {
    return ArrayLiteral(literal);
  }
  if (auto* expression = ctx->expression()) {
    return Expression(expression);
  }
  if (auto* measure = ctx->measureExpression()) {
    return Measure(measure);
  }
  return QuantumCall(ctx->quantumCallExpression());
}

ast::ExprPtr AstBuilder::Measure(qasm3Parser::MeasureExpressionContext* ctx) {
  auto expr = Make<ast::MeasureExpr>(Loc(ctx));
  expr->qubit = GateOperand(ctx->gateOperand());
  return expr;
}

ast::ExprPtr AstBuilder::QuantumCall(
    qasm3Parser::QuantumCallExpressionContext* ctx) {
  auto expr = Make<ast::QuantumCallExpr>(Loc(ctx));
  expr->callee = ctx->Identifier()->getText();
  expr->arguments = Expressions(ctx->expressionList());
  expr->qubits = GateOperands(ctx->gateOperandList());
  return expr;
}

// `a:b` has a start and a stop; `a:s:b` also a step.
ast::ExprPtr AstBuilder::Range(qasm3Parser::RangeExpressionContext* ctx) {
  std::vector<ast::ExprPtr> parts(1);
  for (auto* child : ctx->children) {
    if (auto* expression =
            dynamic_cast<qasm3Parser::ExpressionContext*>(child)) {
      parts.back() = Expression(expression);
    } else {
      parts.emplace_back();  // a colon
    }
  }
  auto expr = Make<ast::RangeExpr>(Loc(ctx));
  expr->start = parts.front();
  expr->stop = parts.back();
  if (parts.size() == 3) {
    expr->step = parts[1];
  }
  return expr;
}

ast::ExprPtr AstBuilder::Set(qasm3Parser::SetExpressionContext* ctx) {
  auto expr = Make<ast::SetExpr>(Loc(ctx));
  for (auto* element : ctx->expression()) {
    expr->elements.push_back(Expression(element));
  }
  return expr;
}

ast::ExprPtr AstBuilder::ArrayLiteral(qasm3Parser::ArrayLiteralContext* ctx) {
  auto expr = Make<ast::ArrayLiteral>(Loc(ctx));
  for (auto* child : ctx->children) {
    if (auto* expression =
            dynamic_cast<qasm3Parser::ExpressionContext*>(child)) {
      expr->elements.push_back(Expression(expression));
    } else if (auto* nested =
                   dynamic_cast<qasm3Parser::ArrayLiteralContext*>(child)) {
      expr->elements.push_back(ArrayLiteral(nested));
    }
  }
  return expr;
}

ast::ExprPtr AstBuilder::AliasExpression(
    qasm3Parser::AliasExpressionContext* ctx) {
  auto parts = ctx->expression();
  if (parts.size() == 1) {
    return Expression(parts[0]);
  }
  auto expr = Make<ast::ConcatenationExpr>(Loc(ctx));
  for (auto* part : parts) {
    expr->parts.push_back(Expression(part));
  }
  return expr;
}

ast::ExprPtr AstBuilder::IndexedIdentifier(
    qasm3Parser::IndexedIdentifierContext* ctx) {
  auto identifier = Make<ast::Identifier>(Loc(ctx));
  identifier->name = ctx->Identifier()->getText();
  ast::ExprPtr expr = identifier;
  for (auto* index : ctx->indexOperator()) {
    expr = Index(std::move(expr), index);
  }
  return expr;
}

ast::ExprPtr AstBuilder::GateOperand(qasm3Parser::GateOperandContext* ctx) {
  if (auto* indexed = ctx->indexedIdentifier()) {
    return IndexedIdentifier(indexed);
  }
  return HardwareQubit(ctx->HardwareQubit());
}

ast::ExprPtr AstBuilder::Index(ast::ExprPtr base,
                               qasm3Parser::IndexOperatorContext* ctx) {
  auto expr = Make<ast::IndexExpr>(base->loc);
  expr->base = std::move(base);
  for (auto* child : ctx->children) {
    if (auto* set = dynamic_cast<qasm3Parser::SetExpressionContext*>(child)) {
      expr->items.push_back(Set(set));
    } else if (auto* range =
                   dynamic_cast<qasm3Parser::RangeExpressionContext*>(child)) {
      expr->items.push_back(Range(range));
    } else if (auto* expression =
                   dynamic_cast<qasm3Parser::ExpressionContext*>(child)) {
      expr->items.push_back(Expression(expression));
    }
  }
  return expr;
}

ast::ExprPtr AstBuilder::Designator(qasm3Parser::DesignatorContext* ctx) {
  return ctx != nullptr ? Expression(ctx->expression()) : nullptr;
}

std::vector<ast::ExprPtr> AstBuilder::Expressions(
    qasm3Parser::ExpressionListContext* ctx) {
  std::vector<ast::ExprPtr> expressions;
  if (ctx != nullptr) {
    for (auto* expression : ctx->expression()) {
      expressions.push_back(Expression(expression));
    }
  }
  return expressions;
}

std::vector<ast::ExprPtr> AstBuilder::GateOperands(
    qasm3Parser::GateOperandListContext* ctx) {
  std::vector<ast::ExprPtr> operands;
  if (ctx != nullptr) {
    for (auto* operand : ctx->gateOperand()) {
      operands.push_back(GateOperand(operand));
    }
  }
  return operands;
}

}  // namespace qde::parser::internal
