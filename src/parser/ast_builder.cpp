#include "qde/parser/internal/ast_builder.hpp"

// The program and its statements.
// Expressions are built in ast_builder_expressions.cpp.
// Literals and types in    ast_builder_literals.cpp.

#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <antlr4-runtime.h>
#include <qasm3Parser.h>

#include "qde/parser/internal/ast.hpp"
#include "qde/parser/internal/parse_tree_text.hpp"

namespace qde::parser::internal {

ast::SourceLocation AstBuilder::ReportUnsupported(
    antlr4::ParserRuleContext* ctx, const std::string& what) {
  // This is a programming error: the grammar has been extended but the
  // AstBuilder has not been updated to handle it.
  // Release builds report it and parse on to report the rest of the program.
  assert(false && "the AstBuilder has no case for this part of the grammar");
  if (ctx == nullptr) {
    // A rule that matched is missing a part: there is no position to report.
    const ast::SourceLocation start{1, 0};
    diagnostics_.Error(start, "internal error: missing " + what);
    return start;
  }
  diagnostics_.Error(Loc(ctx), "internal error: unsupported " + what + " '" +
                                   SourceText(ctx) + "'");
  return Loc(ctx);
}

namespace {

using ast::BinaryOp;

// `ctx` as it is written up to its body.
std::string Header(antlr4::ParserRuleContext* ctx,
                   antlr4::ParserRuleContext* body) {
  return SourceText(ctx, body->getStart()->getStartIndex());
}

BinaryOp CompoundOperator(std::string_view text) {
  // `text` is the operator without its trailing '='.
  if (text == "+") {
    return BinaryOp::kAdd;
  }
  if (text == "-") {
    return BinaryOp::kSubtract;
  }
  if (text == "*") {
    return BinaryOp::kMultiply;
  }
  if (text == "/") {
    return BinaryOp::kDivide;
  }
  if (text == "%") {
    return BinaryOp::kModulo;
  }
  if (text == "**") {
    return BinaryOp::kPower;
  }
  if (text == "&") {
    return BinaryOp::kBitAnd;
  }
  if (text == "|") {
    return BinaryOp::kBitOr;
  }
  if (text == "^") {
    return BinaryOp::kBitXor;
  }
  if (text == "<<") {
    return BinaryOp::kShiftLeft;
  }
  return BinaryOp::kShiftRight;
}

}  // namespace

// ---- program ---------------------------------------------------------------

std::shared_ptr<const ast::Program> AstBuilder::Build(
    qasm3Parser::ProgramContext* ctx) {
  auto program = std::make_shared<ast::Program>();
  if (auto* version = ctx->version()) {
    program->version = version->VersionSpecifier()->getText();
    program->version_loc = Loc(version);
  }
  for (auto* item : ctx->statementOrScope()) {
    program->statements.push_back(StatementOrScope(item));
  }
  return program;
}

// ---- statements ------------------------------------------------------------

ast::StmtPtr AstBuilder::StatementOrScope(
    qasm3Parser::StatementOrScopeContext* ctx) {
  if (auto* statement = ctx->statement()) {
    return Statement(statement);
  }
  auto block = Make<ast::BlockStmt>(Loc(ctx));
  block->body = Scope(ctx->scope());
  return block;
}

std::vector<ast::StmtPtr> AstBuilder::Body(
    qasm3Parser::StatementOrScopeContext* ctx) {
  if (auto* scope = ctx->scope()) {
    return Scope(scope);
  }

  return {Statement(ctx->statement())};  // Single statement without scope
}

std::vector<ast::StmtPtr> AstBuilder::Scope(qasm3Parser::ScopeContext* ctx) {
  std::vector<ast::StmtPtr> statements;
  for (auto* item : ctx->statementOrScope()) {
    statements.push_back(StatementOrScope(item));
  }
  return statements;
}

ast::StmtPtr AstBuilder::Statement(qasm3Parser::StatementContext* ctx) {
  if (auto* pragma = ctx->pragma()) {
    auto stmt = Make<ast::PragmaStmt>(Loc(ctx));
    stmt->text = pragma->RemainingLineContent()->getText();
    return stmt;
  }

  StmtNode stmt;
  if (auto* alias = ctx->aliasDeclarationStatement()) {
    stmt = Alias(alias);
  } else if (auto* assignment = ctx->assignmentStatement()) {
    stmt = Assignment(assignment);
  } else if (auto* barrier = ctx->barrierStatement()) {
    stmt = Barrier(barrier);
  } else if (auto* box = ctx->boxStatement()) {
    stmt = Box(box);
  } else if (ctx->breakStatement() != nullptr) {
    stmt = Make<ast::BreakStmt>(Loc(ctx));
  } else if (ctx->calStatement() != nullptr) {
    stmt = Make<ast::CalibrationStmt>(Loc(ctx));
  } else if (auto* grammar = ctx->calibrationGrammarStatement()) {
    auto calibration_grammar = Make<ast::CalibrationGrammarStmt>(Loc(ctx));
    calibration_grammar->name = Unquoted(grammar->StringLiteral()->getText());
    stmt = calibration_grammar;
  } else if (auto* classical = ctx->classicalDeclarationStatement()) {
    stmt = ClassicalDeclaration(classical);
  } else if (auto* constant = ctx->constDeclarationStatement()) {
    stmt = ConstDeclaration(constant);
  } else if (ctx->continueStatement() != nullptr) {
    stmt = Make<ast::ContinueStmt>(Loc(ctx));
  } else if (auto* def = ctx->defStatement()) {
    stmt = Subroutine(def);
  } else if (auto* defcal = ctx->defcalStatement()) {
    stmt = Defcal(defcal);
  } else if (auto* delay = ctx->delayStatement()) {
    stmt = Delay(delay);
  } else if (ctx->endStatement() != nullptr) {
    stmt = Make<ast::EndStmt>(Loc(ctx));
  } else if (auto* expression = ctx->expressionStatement()) {
    auto expression_stmt = Make<ast::ExpressionStmt>(Loc(ctx));
    expression_stmt->expression = Expression(expression->expression());
    stmt = expression_stmt;
  } else if (auto* external = ctx->externStatement()) {
    stmt = Extern(external);
  } else if (auto* loop = ctx->forStatement()) {
    stmt = For(loop);
  } else if (auto* call = ctx->gateCallStatement()) {
    stmt = GateCall(call);
  } else if (auto* gate = ctx->gateStatement()) {
    stmt = Gate(gate);
  } else if (auto* branch = ctx->ifStatement()) {
    stmt = If(branch);
  } else if (auto* include = ctx->includeStatement()) {
    auto include_stmt = Make<ast::IncludeStmt>(Loc(ctx));
    include_stmt->path = Unquoted(include->StringLiteral()->getText());
    stmt = include_stmt;
  } else if (auto* io = ctx->ioDeclarationStatement()) {
    stmt = IoDeclaration(io);
  } else if (auto* measure = ctx->measureArrowAssignmentStatement()) {
    stmt = MeasureArrow(measure);
  } else if (auto* nop = ctx->nopStatement()) {
    auto nop_stmt = Make<ast::NopStmt>(Loc(ctx));
    nop_stmt->qubits = GateOperands(nop->gateOperandList());
    stmt = nop_stmt;
  } else if (auto* old_style = ctx->oldStyleDeclarationStatement()) {
    stmt = OldStyleDeclaration(old_style);
  } else if (auto* quantum = ctx->quantumDeclarationStatement()) {
    stmt = QuantumDeclaration(quantum);
  } else if (auto* reset = ctx->resetStatement()) {
    auto reset_stmt = Make<ast::ResetStmt>(Loc(ctx));
    reset_stmt->qubit = GateOperand(reset->gateOperand());
    stmt = reset_stmt;
  } else if (auto* ret = ctx->returnStatement()) {
    stmt = Return(ret);
  } else if (auto* switch_stmt = ctx->switchStatement()) {
    stmt = Switch(switch_stmt);
  } else if (auto* while_stmt = ctx->whileStatement()) {
    stmt = While(while_stmt);
  } else {
    stmt = Make<ast::ErrorStmt>(ReportUnsupported(ctx, "statement"));
  }

  stmt->loc = Loc(ctx);
  if (stmt->source.empty()) {
    // The statement itself is the last child, after its annotations.
    if (auto* written =
            dynamic_cast<antlr4::ParserRuleContext*>(ctx->children.back())) {
      stmt->source = SourceText(written);
    }
  }
  for (auto* annotation : ctx->annotation()) {
    std::string text = annotation->AnnotationKeyword()->getText();
    if (auto* content = annotation->RemainingLineContent()) {
      text += " " + content->getText();
    }
    stmt->annotations.push_back(std::move(text));
  }
  return stmt;
}

AstBuilder::StmtNode AstBuilder::Alias(
    qasm3Parser::AliasDeclarationStatementContext* ctx) {
  auto stmt = Make<ast::AliasStmt>(Loc(ctx));
  stmt->name = ctx->Identifier()->getText();
  stmt->value = AliasExpression(ctx->aliasExpression());
  return stmt;
}

AstBuilder::StmtNode AstBuilder::Assignment(
    qasm3Parser::AssignmentStatementContext* ctx) {
  auto stmt = Make<ast::AssignmentStmt>(Loc(ctx));
  stmt->target = IndexedIdentifier(ctx->indexedIdentifier());
  if (ctx->CompoundAssignmentOperator() != nullptr) {
    std::string op = ctx->op->getText();
    op.pop_back();  // the trailing '='
    stmt->compound_op = CompoundOperator(op);
  }
  if (auto* expression = ctx->expression()) {
    stmt->value = Expression(expression);
  } else if (auto* measure = ctx->measureExpression()) {
    stmt->value = Measure(measure);
  } else {
    stmt->value = QuantumCall(ctx->quantumCallExpression());
  }
  return stmt;
}

AstBuilder::StmtNode AstBuilder::Barrier(
    qasm3Parser::BarrierStatementContext* ctx) {
  auto stmt = Make<ast::BarrierStmt>(Loc(ctx));
  stmt->qubits = GateOperands(ctx->gateOperandList());
  return stmt;
}

AstBuilder::StmtNode AstBuilder::Box(qasm3Parser::BoxStatementContext* ctx) {
  auto stmt = Make<ast::BoxStmt>(Loc(ctx));
  stmt->source = Header(ctx, ctx->scope());
  stmt->duration = Designator(ctx->designator());
  stmt->body = Scope(ctx->scope());
  return stmt;
}

AstBuilder::StmtNode AstBuilder::ClassicalDeclaration(
    qasm3Parser::ClassicalDeclarationStatementContext* ctx) {
  auto stmt = Make<ast::ClassicalDeclarationStmt>(Loc(ctx));
  stmt->name = ctx->Identifier()->getText();
  stmt->type = ctx->scalarType() != nullptr ? ScalarType(ctx->scalarType())
                                            : ArrayType(ctx->arrayType());
  if (auto* initializer = ctx->declarationExpression()) {
    stmt->initializer = DeclarationExpression(initializer);
  }
  return stmt;
}

AstBuilder::StmtNode AstBuilder::ConstDeclaration(
    qasm3Parser::ConstDeclarationStatementContext* ctx) {
  auto stmt = Make<ast::ConstDeclarationStmt>(Loc(ctx));
  stmt->name = ctx->Identifier()->getText();
  stmt->type = ScalarType(ctx->scalarType());
  stmt->initializer = DeclarationExpression(ctx->declarationExpression());
  return stmt;
}

AstBuilder::StmtNode AstBuilder::Subroutine(
    qasm3Parser::DefStatementContext* ctx) {
  auto stmt = Make<ast::SubroutineDefinitionStmt>(Loc(ctx));
  stmt->source = Header(ctx, ctx->scope());
  stmt->name = ctx->Identifier()->getText();
  if (auto* list = ctx->argumentDefinitionList()) {
    for (auto* argument : list->argumentDefinition()) {
      ast::Parameter parameter;
      parameter.loc = Loc(argument);
      parameter.name = argument->Identifier()->getText();
      if (auto* scalar = argument->scalarType()) {
        parameter.type = ScalarType(scalar);
      } else if (auto* qubit = argument->qubitType()) {
        parameter.type = QubitType(Loc(argument), qubit->designator());
      } else if (auto* reference = argument->arrayReferenceType()) {
        parameter.type = ArrayReferenceType(reference);
      } else if (argument->QREG() != nullptr) {
        parameter.type = QubitType(Loc(argument), argument->designator());
      } else {
        parameter.type = BitType(Loc(argument), argument->designator());
      }
      stmt->parameters.push_back(std::move(parameter));
    }
  }
  if (auto* signature = ctx->returnSignature()) {
    stmt->return_type = ScalarType(signature->scalarType());
  }
  stmt->body = Scope(ctx->scope());
  return stmt;
}

AstBuilder::StmtNode AstBuilder::Defcal(
    qasm3Parser::DefcalStatementContext* ctx) {
  auto stmt = Make<ast::CalibrationDefinitionStmt>(Loc(ctx));
  stmt->target = ctx->defcalTarget()->getText();
  if (auto* signature = ctx->returnSignature()) {
    stmt->return_type = ScalarType(signature->scalarType());
  }
  return stmt;
}

AstBuilder::StmtNode AstBuilder::Delay(
    qasm3Parser::DelayStatementContext* ctx) {
  auto stmt = Make<ast::DelayStmt>(Loc(ctx));
  stmt->duration = Designator(ctx->designator());
  stmt->qubits = GateOperands(ctx->gateOperandList());
  return stmt;
}

AstBuilder::StmtNode AstBuilder::Extern(
    qasm3Parser::ExternStatementContext* ctx) {
  auto stmt = Make<ast::ExternDeclarationStmt>(Loc(ctx));
  stmt->name = ctx->Identifier()->getText();
  if (auto* list = ctx->externArgumentList()) {
    for (auto* argument : list->externArgument()) {
      ast::Parameter parameter;
      parameter.loc = Loc(argument);
      if (auto* scalar = argument->scalarType()) {
        parameter.type = ScalarType(scalar);
      } else if (auto* reference = argument->arrayReferenceType()) {
        parameter.type = ArrayReferenceType(reference);
      } else {
        parameter.type = BitType(Loc(argument), argument->designator());
      }
      stmt->parameters.push_back(std::move(parameter));
    }
  }
  if (auto* signature = ctx->returnSignature()) {
    stmt->return_type = ScalarType(signature->scalarType());
  }
  return stmt;
}

AstBuilder::StmtNode AstBuilder::For(qasm3Parser::ForStatementContext* ctx) {
  auto stmt = Make<ast::ForStmt>(Loc(ctx));
  stmt->source = Header(ctx, ctx->body);
  stmt->name = ctx->Identifier()->getText();
  stmt->type = ScalarType(ctx->scalarType());
  if (auto* set = ctx->setExpression()) {
    stmt->sequence = Set(set);
  } else if (auto* range = ctx->rangeExpression()) {
    stmt->sequence = Range(range);
  } else {
    stmt->sequence = Expression(ctx->expression());
  }
  stmt->body = Body(ctx->body);
  return stmt;
}

AstBuilder::StmtNode AstBuilder::GateCall(
    qasm3Parser::GateCallStatementContext* ctx) {
  auto stmt = Make<ast::GateCallStmt>(Loc(ctx));
  stmt->is_gphase = ctx->GPHASE() != nullptr;
  stmt->name = stmt->is_gphase ? "gphase" : ctx->Identifier()->getText();
  for (auto* modifier_ctx : ctx->gateModifier()) {
    ast::GateModifier modifier;
    modifier.loc = Loc(modifier_ctx);
    if (modifier_ctx->INV() != nullptr) {
      modifier.kind = ast::GateModifier::Kind::kInv;
    } else if (modifier_ctx->POW() != nullptr) {
      modifier.kind = ast::GateModifier::Kind::kPow;
    } else if (modifier_ctx->CTRL() != nullptr) {
      modifier.kind = ast::GateModifier::Kind::kCtrl;
    } else {
      modifier.kind = ast::GateModifier::Kind::kNegCtrl;
    }
    if (auto* argument = modifier_ctx->expression()) {
      modifier.argument = Expression(argument);
    }
    stmt->modifiers.push_back(std::move(modifier));
  }
  stmt->arguments = Expressions(ctx->expressionList());
  stmt->duration = Designator(ctx->designator());
  stmt->qubits = GateOperands(ctx->gateOperandList());
  return stmt;
}

AstBuilder::StmtNode AstBuilder::Gate(qasm3Parser::GateStatementContext* ctx) {
  auto stmt = Make<ast::GateDefinitionStmt>(Loc(ctx));
  stmt->source = Header(ctx, ctx->scope());
  stmt->name = ctx->Identifier()->getText();
  if (ctx->params != nullptr) {
    for (auto* identifier : ctx->params->Identifier()) {
      stmt->parameter_names.push_back(identifier->getText());
    }
  }
  if (ctx->qubits != nullptr) {
    for (auto* identifier : ctx->qubits->Identifier()) {
      stmt->qubit_names.push_back(identifier->getText());
    }
  }
  stmt->body = Scope(ctx->scope());
  return stmt;
}

AstBuilder::StmtNode AstBuilder::If(qasm3Parser::IfStatementContext* ctx) {
  auto stmt = Make<ast::IfStmt>(Loc(ctx));
  stmt->source = Header(ctx, ctx->if_body);
  stmt->condition = Expression(ctx->expression());
  stmt->body = Body(ctx->if_body);
  if (ctx->else_body != nullptr) {
    stmt->has_else = true;
    stmt->else_body = Body(ctx->else_body);
  }
  return stmt;
}

AstBuilder::StmtNode AstBuilder::IoDeclaration(
    qasm3Parser::IoDeclarationStatementContext* ctx) {
  auto stmt = Make<ast::IoDeclarationStmt>(Loc(ctx));
  stmt->name = ctx->Identifier()->getText();
  stmt->is_input = ctx->INPUT() != nullptr;
  stmt->type = ctx->scalarType() != nullptr ? ScalarType(ctx->scalarType())
                                            : ArrayType(ctx->arrayType());
  return stmt;
}

// `measure q -> c;` assigns like `c = measure q;`.
// Without the arrow the measurement is evaluated for its effect alone.
AstBuilder::StmtNode AstBuilder::MeasureArrow(
    qasm3Parser::MeasureArrowAssignmentStatementContext* ctx) {
  ast::ExprPtr value = ctx->measureExpression() != nullptr
                           ? Measure(ctx->measureExpression())
                           : QuantumCall(ctx->quantumCallExpression());
  if (auto* target = ctx->indexedIdentifier()) {
    auto stmt = Make<ast::AssignmentStmt>(Loc(ctx));
    stmt->target = IndexedIdentifier(target);
    stmt->value = std::move(value);
    return stmt;
  }
  auto stmt = Make<ast::ExpressionStmt>(Loc(ctx));
  stmt->expression = std::move(value);
  return stmt;
}

AstBuilder::StmtNode AstBuilder::OldStyleDeclaration(
    qasm3Parser::OldStyleDeclarationStatementContext* ctx) {
  // `qreg q;` and `creg c;` declare registers of size one.
  ast::ExprPtr width = Designator(ctx->designator());
  if (width == nullptr) {
    auto one = Make<ast::IntegerLiteral>(Loc(ctx));
    one->value = 1;
    width = one;
  }
  if (ctx->QREG() != nullptr) {
    auto type = Make<ast::QubitTypeSpec>(Loc(ctx));
    type->width = std::move(width);
    auto stmt = Make<ast::QubitDeclarationStmt>(Loc(ctx));
    stmt->name = ctx->Identifier()->getText();
    stmt->type = std::move(type);
    return stmt;
  }
  auto type = Make<ast::ScalarTypeSpec>(Loc(ctx));
  type->scalar = ast::ScalarKind::kBit;
  type->width = std::move(width);
  auto stmt = Make<ast::ClassicalDeclarationStmt>(Loc(ctx));
  stmt->name = ctx->Identifier()->getText();
  stmt->type = std::move(type);
  return stmt;
}

AstBuilder::StmtNode AstBuilder::QuantumDeclaration(
    qasm3Parser::QuantumDeclarationStatementContext* ctx) {
  auto stmt = Make<ast::QubitDeclarationStmt>(Loc(ctx));
  stmt->name = ctx->Identifier()->getText();
  stmt->type = QubitType(Loc(ctx), ctx->qubitType()->designator());
  return stmt;
}

AstBuilder::StmtNode AstBuilder::Return(
    qasm3Parser::ReturnStatementContext* ctx) {
  auto stmt = Make<ast::ReturnStmt>(Loc(ctx));
  if (auto* expression = ctx->expression()) {
    stmt->value = Expression(expression);
  } else if (auto* measure = ctx->measureExpression()) {
    stmt->value = Measure(measure);
  } else if (auto* call = ctx->quantumCallExpression()) {
    stmt->value = QuantumCall(call);
  }
  return stmt;
}

AstBuilder::StmtNode AstBuilder::Switch(
    qasm3Parser::SwitchStatementContext* ctx) {
  auto stmt = Make<ast::SwitchStmt>(Loc(ctx));
  stmt->source =
      SourceText(ctx, ctx->RPAREN()->getSymbol()->getStopIndex() + 1);
  stmt->subject = Expression(ctx->expression());
  for (auto* item : ctx->switchCaseItem()) {
    ast::SwitchCase switch_case;
    switch_case.loc = Loc(item);
    switch_case.source = Header(item, item->scope());
    switch_case.values = Expressions(item->expressionList());
    switch_case.body = Scope(item->scope());
    stmt->cases.push_back(std::move(switch_case));
  }
  return stmt;
}

AstBuilder::StmtNode AstBuilder::While(
    qasm3Parser::WhileStatementContext* ctx) {
  auto stmt = Make<ast::WhileStmt>(Loc(ctx));
  stmt->source = Header(ctx, ctx->body);
  stmt->condition = Expression(ctx->expression());
  stmt->body = Body(ctx->body);
  return stmt;
}

}  // namespace qde::parser::internal
