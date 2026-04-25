#include "qde/parser.hpp"

#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>

#include <antlr4-runtime.h>
#include <qasm3Lexer.h>
#include <qasm3Parser.h>
#include <qasm3ParserBaseVisitor.h>

#include "qde/backend_config.hpp"
#include "qde/circuit.hpp"
#include "qde/parse_result.hpp"
#include "qde/syntax_error_listener.hpp"

namespace qde {

namespace {

using C   = std::complex<double>;
using Mat = std::vector<C>;

constexpr double kPi    = 3.141592653589793238462643383;
constexpr double kTau   = 6.283185307179586476925286766;
constexpr double kEuler = 2.718281828459045235360287471;

// ---------------------------------------------------------------------------
// CircuitBuilder
//
// ANTLR visitor that walks a parsed OpenQASM 3.0 syntax tree and builds a
// Circuit instance from it.
//
// Usage:
//   1. Construct
//   2. Call CircuitBuilder::visit(tree)
//   3. Call CircuitBuilder::semanticErrors() and CircuitBuilder::buildCircuit()
// ---------------------------------------------------------------------------

class CircuitBuilder : public qasm3ParserBaseVisitor {
 public:
  // ---- public interface ---------------------------------------------------
  explicit CircuitBuilder(const BackendConfig& config)
      : config_(config)
  {}

  // May only be called once. std::move transfers ownership out of the object.
  Circuit buildCircuit() {
    return Circuit(std::move(registry_), std::move(qubit_registers_),
                   std::move(bit_registers_), std::move(operations_));
  }

  const std::vector<SyntaxError>& semanticErrors() const {
    return semantic_errors_;
  }

  // ---- statement visitors (alphabetical, matching qasm3Parser.g4) ---------

  std::any visitAliasDeclarationStatement(
      qasm3Parser::AliasDeclarationStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitAssignmentStatement(
      qasm3Parser::AssignmentStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitBarrierStatement(
      qasm3Parser::BarrierStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitBoxStatement(
      qasm3Parser::BoxStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitBreakStatement(
      qasm3Parser::BreakStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitCalibrationGrammarStatement(
      qasm3Parser::CalibrationGrammarStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitCalStatement(
      qasm3Parser::CalStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitClassicalDeclarationStatement(
      qasm3Parser::ClassicalDeclarationStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitConstDeclarationStatement(
      qasm3Parser::ConstDeclarationStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitContinueStatement(
      qasm3Parser::ContinueStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitDefcalStatement(
      qasm3Parser::DefcalStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitDefStatement(
      qasm3Parser::DefStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitDelayStatement(
      qasm3Parser::DelayStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitEndStatement(
      qasm3Parser::EndStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitExpressionStatement(
      qasm3Parser::ExpressionStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitExternStatement(
      qasm3Parser::ExternStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitForStatement(
      qasm3Parser::ForStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitGateCallStatement(
      qasm3Parser::GateCallStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitGateStatement(
      qasm3Parser::GateStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitIfStatement(
      qasm3Parser::IfStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitIncludeStatement(
      qasm3Parser::IncludeStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitIoDeclarationStatement(
      qasm3Parser::IoDeclarationStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitMeasureArrowAssignmentStatement(
      qasm3Parser::MeasureArrowAssignmentStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitNopStatement(
      qasm3Parser::NopStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitOldStyleDeclarationStatement(
      qasm3Parser::OldStyleDeclarationStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitQuantumDeclarationStatement(
      qasm3Parser::QuantumDeclarationStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitResetStatement(
      qasm3Parser::ResetStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitReturnStatement(
      qasm3Parser::ReturnStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitSwitchStatement(
      qasm3Parser::SwitchStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  std::any visitWhileStatement(
      qasm3Parser::WhileStatementContext* ctx) override {
    return visitChildren(ctx);
  }

 private:
  // ---- types --------------------------------------------------------------

  struct RegisterInfo {
    std::uint8_t index;
    std::uint8_t size;
  };

  // ---- circuit state ------------------------------------------------------
  
  BackendConfig config_;
  GateRegistry registry_ = GateRegistry::withBuiltins();
  std::vector<QubitRegister> qubit_registers_;
  std::vector<BitRegister>   bit_registers_;
  std::vector<Operation>     operations_;

  // ---- symbol tables ------------------------------------------------------

  std::unordered_map<std::string, RegisterInfo> qubit_map_;
  std::unordered_map<std::string, RegisterInfo> bit_map_;
  std::unordered_map<std::string, double>       scalar_vals_;
  std::unordered_map<std::string, C>            complex_vals_;

  // ---- errors -------------------------------------------------------------

  std::vector<SyntaxError> semantic_errors_;
};

}  // namespace

// ---------------------------------------------------------------------------
// Parser::parse
//
// Builds a Circuit from OpenQASM 3.0 source using the given backend config.
// Returns a failed ParseResult on syntax or semantic errors.
// ---------------------------------------------------------------------------

ParseResult Parser::parse(const std::string& source,
                          const BackendConfig& config) const {
  SyntaxErrorListener listener;

  antlr4::ANTLRInputStream input(source);
  qasm3Lexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  qasm3Parser parser(&tokens);

  lexer.removeErrorListeners();
  lexer.addErrorListener(&listener);
  parser.removeErrorListeners();
  parser.addErrorListener(&listener);

  auto* tree = parser.program();

  if (listener.hasErrors()) {
    return ParseResult::fail(std::move(listener.errors()));
  }
  
  CircuitBuilder builder(config);
  builder.visit(tree);

  if (!builder.semanticErrors().empty()) {
    auto errors = builder.semanticErrors();
    return ParseResult::fail(std::move(errors));
  }

  return ParseResult::ok(builder.buildCircuit());
}

}  // namespace qde
