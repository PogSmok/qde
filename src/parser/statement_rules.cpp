#include "qde/parser/internal/statement_rules.hpp"

#include "qde/parser/internal/ast.hpp"

namespace qde::parser::internal {

using ast::StmtKind;

std::string_view Describe(ast::StmtKind kind) {
  switch (kind) {
    case StmtKind::kPragma:
      return "a pragma";
    case StmtKind::kInclude:
      return "an include";
    case StmtKind::kCalibrationGrammar:
      return "a defcalgrammar statement";
    case StmtKind::kCalibration:
      return "a cal block";
    case StmtKind::kCalibrationDefinition:
      return "a defcal";
    case StmtKind::kQubitDeclaration:
      return "a qubit declaration";
    case StmtKind::kClassicalDeclaration:
      return "a variable declaration";
    case StmtKind::kConstDeclaration:
      return "a constant declaration";
    case StmtKind::kIoDeclaration:
      return "an input or output declaration";
    case StmtKind::kAlias:
      return "an alias";
    case StmtKind::kAssignment:
      return "an assignment";
    case StmtKind::kExpression:
      return "an expression statement";
    case StmtKind::kGateCall:
      return "a gate call";
    case StmtKind::kReset:
      return "a reset";
    case StmtKind::kBarrier:
      return "a barrier";
    case StmtKind::kDelay:
      return "a delay";
    case StmtKind::kNop:
      return "a nop";
    case StmtKind::kBox:
      return "a box";
    case StmtKind::kBlock:
      return "a block";
    case StmtKind::kIf:
      return "an if statement";
    case StmtKind::kFor:
      return "a for loop";
    case StmtKind::kWhile:
      return "a while loop";
    case StmtKind::kSwitch:
      return "a switch statement";
    case StmtKind::kBreak:
      return "a break";
    case StmtKind::kContinue:
      return "a continue";
    case StmtKind::kEnd:
      return "an end statement";
    case StmtKind::kReturn:
      return "a return";
    case StmtKind::kGateDefinition:
      return "a gate definition";
    case StmtKind::kSubroutineDefinition:
      return "a subroutine definition";
    case StmtKind::kExternDeclaration:
      return "an extern declaration";
    case StmtKind::kError:
      return "an unsupported statement";
  }
  return "this statement";
}

bool IsGlobalOnly(ast::StmtKind kind) {
  switch (kind) {
    case StmtKind::kInclude:
    case StmtKind::kCalibrationGrammar:
    case StmtKind::kCalibration:
    case StmtKind::kCalibrationDefinition:
    case StmtKind::kQubitDeclaration:
    case StmtKind::kIoDeclaration:
    case StmtKind::kGateDefinition:
    case StmtKind::kSubroutineDefinition:
    case StmtKind::kExternDeclaration:
      return true;
    default:
      return false;
  }
}

bool IsAllowedInGate(ast::StmtKind kind) {
  switch (kind) {
    case StmtKind::kPragma:
    case StmtKind::kConstDeclaration:
    case StmtKind::kGateCall:
    case StmtKind::kBarrier:
    case StmtKind::kDelay:
    case StmtKind::kNop:
    case StmtKind::kBox:
    case StmtKind::kBlock:
    case StmtKind::kIf:
    case StmtKind::kFor:
    case StmtKind::kWhile:
    case StmtKind::kSwitch:
    case StmtKind::kBreak:
    case StmtKind::kContinue:
      return true;
    default:
      return false;
  }
}

}  // namespace qde::parser::internal
