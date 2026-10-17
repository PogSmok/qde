#ifndef PARSER_INTERNAL_AST_TRAVERSAL_HPP_
#define PARSER_INTERNAL_AST_TRAVERSAL_HPP_

#include <vector>

#include "qde/parser/internal/ast/expr.hpp"
#include "qde/parser/internal/ast/node.hpp"
#include "qde/parser/internal/ast/stmt.hpp"

namespace qde::parser::internal::ast {

// Calls visit(child) for every expression child of `expr`, in source order,
// but not for the expressions inside the types and scopes of `expr` itself.
template <typename Visit>
void ForEachChild(const Expr& expr, const Visit& visit) {
  const auto visit_one = [&visit](const ExprPtr& child) {
    if (child != nullptr) {
      visit(*child);
    }
  };
  const auto visit_all = [&visit_one](const std::vector<ExprPtr>& children) {
    for (const ExprPtr& child : children) {
      visit_one(child);
    }
  };
  switch (expr.Kind()) {
    case ExprKind::kIntegerLiteral:
    case ExprKind::kFloatLiteral:
    case ExprKind::kImaginaryLiteral:
    case ExprKind::kBooleanLiteral:
    case ExprKind::kBitstringLiteral:
    case ExprKind::kTimingLiteral:
    case ExprKind::kHardwareQubit:
    case ExprKind::kIdentifier:
    case ExprKind::kDurationOf:
    case ExprKind::kError:
      break;
    case ExprKind::kUnary:
      visit_one(Cast<UnaryExpr>(expr).operand);
      break;
    case ExprKind::kBinary:
      visit_one(Cast<BinaryExpr>(expr).lhs);
      visit_one(Cast<BinaryExpr>(expr).rhs);
      break;
    case ExprKind::kIndex:
      visit_one(Cast<IndexExpr>(expr).base);
      visit_all(Cast<IndexExpr>(expr).items);
      break;
    case ExprKind::kCast:
      visit_one(Cast<CastExpr>(expr).operand);
      break;
    case ExprKind::kCall:
      visit_all(Cast<CallExpr>(expr).arguments);
      break;
    case ExprKind::kMeasure:
      visit_one(Cast<MeasureExpr>(expr).qubit);
      break;
    case ExprKind::kQuantumCall:
      visit_all(Cast<QuantumCallExpr>(expr).arguments);
      visit_all(Cast<QuantumCallExpr>(expr).qubits);
      break;
    case ExprKind::kArrayLiteral:
      visit_all(Cast<ArrayLiteral>(expr).elements);
      break;
    case ExprKind::kRange:
      visit_one(Cast<RangeExpr>(expr).start);
      visit_one(Cast<RangeExpr>(expr).step);
      visit_one(Cast<RangeExpr>(expr).stop);
      break;
    case ExprKind::kSet:
      visit_all(Cast<SetExpr>(expr).elements);
      break;
    case ExprKind::kConcatenation:
      visit_all(Cast<ConcatenationExpr>(expr).parts);
      break;
  }
}

// Whether `predicate` holds for any child of `expr`, as
// ForEachChild visits them.
template <typename Predicate>
[[nodiscard]] bool AnyChild(const Expr& expr, const Predicate& predicate) {
  bool found = false;
  ForEachChild(expr,
               [&](const Expr& child) { found = found || predicate(child); });
  return found;
}

// Calls visit(expression) for every expression child of `stmt`, in source
// order, but not for the expressions inside the types and scopes of `stmt`
// itself.
template <typename Visit>
void ForEachExpression(const Stmt& stmt, const Visit& visit) {
  const auto visit_one = [&visit](const ExprPtr& expr) {
    if (expr != nullptr) {
      visit(*expr);
    }
  };
  const auto visit_all = [&visit_one](const std::vector<ExprPtr>& exprs) {
    for (const ExprPtr& expr : exprs) {
      visit_one(expr);
    }
  };
  switch (stmt.Kind()) {
    case StmtKind::kPragma:
    case StmtKind::kInclude:
    case StmtKind::kCalibrationGrammar:
    case StmtKind::kCalibration:
    case StmtKind::kCalibrationDefinition:
    case StmtKind::kQubitDeclaration:
    case StmtKind::kIoDeclaration:
    case StmtKind::kBlock:
    case StmtKind::kBreak:
    case StmtKind::kContinue:
    case StmtKind::kEnd:
    case StmtKind::kGateDefinition:
    case StmtKind::kSubroutineDefinition:
    case StmtKind::kExternDeclaration:
    case StmtKind::kError:
      break;
    case StmtKind::kClassicalDeclaration:
      visit_one(Cast<ClassicalDeclarationStmt>(stmt).initializer);
      break;
    case StmtKind::kConstDeclaration:
      visit_one(Cast<ConstDeclarationStmt>(stmt).initializer);
      break;
    case StmtKind::kAlias:
      visit_one(Cast<AliasStmt>(stmt).value);
      break;
    case StmtKind::kAssignment:
      visit_one(Cast<AssignmentStmt>(stmt).target);
      visit_one(Cast<AssignmentStmt>(stmt).value);
      break;
    case StmtKind::kExpression:
      visit_one(Cast<ExpressionStmt>(stmt).expression);
      break;
    case StmtKind::kGateCall: {
      const auto& call = Cast<GateCallStmt>(stmt);
      for (const GateModifier& modifier : call.modifiers) {
        visit_one(modifier.argument);
      }
      visit_all(call.arguments);
      visit_one(call.duration);
      visit_all(call.qubits);
      break;
    }
    case StmtKind::kReset:
      visit_one(Cast<ResetStmt>(stmt).qubit);
      break;
    case StmtKind::kBarrier:
      visit_all(Cast<BarrierStmt>(stmt).qubits);
      break;
    case StmtKind::kDelay:
      visit_one(Cast<DelayStmt>(stmt).duration);
      visit_all(Cast<DelayStmt>(stmt).qubits);
      break;
    case StmtKind::kNop:
      visit_all(Cast<NopStmt>(stmt).qubits);
      break;
    case StmtKind::kBox:
      visit_one(Cast<BoxStmt>(stmt).duration);
      break;
    case StmtKind::kIf:
      visit_one(Cast<IfStmt>(stmt).condition);
      break;
    case StmtKind::kFor:
      visit_one(Cast<ForStmt>(stmt).sequence);
      break;
    case StmtKind::kWhile:
      visit_one(Cast<WhileStmt>(stmt).condition);
      break;
    case StmtKind::kSwitch: {
      const auto& switch_stmt = Cast<SwitchStmt>(stmt);
      visit_one(switch_stmt.subject);
      for (const SwitchCase& switch_case : switch_stmt.cases) {
        visit_all(switch_case.values);
      }
      break;
    }
    case StmtKind::kReturn:
      visit_one(Cast<ReturnStmt>(stmt).value);
      break;
  }
}

// Calls visit(statements) for every body of `stmt`, in source order.
// An if without an else still has an empty else body
template <typename Visit>
void ForEachBody(const Stmt& stmt, const Visit& visit) {
  switch (stmt.Kind()) {
    case StmtKind::kPragma:
    case StmtKind::kInclude:
    case StmtKind::kCalibrationGrammar:
    case StmtKind::kCalibration:
    case StmtKind::kCalibrationDefinition:
    case StmtKind::kQubitDeclaration:
    case StmtKind::kClassicalDeclaration:
    case StmtKind::kConstDeclaration:
    case StmtKind::kIoDeclaration:
    case StmtKind::kAlias:
    case StmtKind::kAssignment:
    case StmtKind::kExpression:
    case StmtKind::kGateCall:
    case StmtKind::kReset:
    case StmtKind::kBarrier:
    case StmtKind::kDelay:
    case StmtKind::kNop:
    case StmtKind::kBreak:
    case StmtKind::kContinue:
    case StmtKind::kEnd:
    case StmtKind::kReturn:
    case StmtKind::kExternDeclaration:
    case StmtKind::kError:
      break;
    case StmtKind::kBox:
      visit(Cast<BoxStmt>(stmt).body);
      break;
    case StmtKind::kBlock:
      visit(Cast<BlockStmt>(stmt).body);
      break;
    case StmtKind::kIf:
      visit(Cast<IfStmt>(stmt).body);
      visit(Cast<IfStmt>(stmt).else_body);
      break;
    case StmtKind::kFor:
      visit(Cast<ForStmt>(stmt).body);
      break;
    case StmtKind::kWhile:
      visit(Cast<WhileStmt>(stmt).body);
      break;
    case StmtKind::kSwitch:
      for (const SwitchCase& switch_case : Cast<SwitchStmt>(stmt).cases) {
        visit(switch_case.body);
      }
      break;
    case StmtKind::kGateDefinition:
      visit(Cast<GateDefinitionStmt>(stmt).body);
      break;
    case StmtKind::kSubroutineDefinition:
      visit(Cast<SubroutineDefinitionStmt>(stmt).body);
      break;
  }
}

}  // namespace qde::parser::internal::ast

#endif  // PARSER_INTERNAL_AST_TRAVERSAL_HPP_
