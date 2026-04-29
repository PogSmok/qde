// ---------------------------------------------------------------------------
// Coverage gaps — not tested here
//
// Stub statements (visitChildren only; parse without error):
//   aliasDeclarationStatement  (let alias = expr)
//   breakStatement             (break)
//   calStatement               (cal { })
//   calibrationGrammarStatement(defcalgrammar "openpulse")
//   continueStatement          (continue)
//   defcalStatement            (defcal pulse-level calibration)
//   expressionStatement        (standalone expression e.g. 1 + 2;)
//   externStatement            (extern fn)
//   includeStatement           (include "stdgates.inc")
//   ioDeclarationStatement     (input/output port)
//   nopStatement               (nop)
//   returnStatement            (return)
//   switchStatement            (switch/case)
//
// Expression forms:
//   TimingLiteral as a gate parameter — only covered via duration decl
//   durationof({ }) — evaluates to 0.0 (TODO in buildExprTreeImpl)
//   User-defined function calls (non-builtin CallExpression) — emits a
//     parse-time error; the error path is not asserted
//   BitstringLiteral as a classical initializer
//   Gate-parameter reference as an array index (a[theta]) — evaluates
//     to 0.0 at parse time; unsupported by design
//
// Known limitations (see parser.cpp for rationale):
//   Complex compound assignment (z += 1.0im) clears the imaginary part;
//     compound operators are real-only by design
//   quantumCallExpression in assignmentStatement (c = gate q) and in
//     returnStatement — requires a working def/extern stub to trigger
//   subroutine_map_ is never populated because defStatement is a stub
// ---------------------------------------------------------------------------

#include <gtest/gtest.h>

#include "qde/backend_config.hpp"
#include "qde/circuit.hpp"
#include "qde/parser.hpp"

// ---- helpers ----------------------------------------------------------------

static qde::ParseResult parse(const std::string& source) {
  return qde::Parser::parse("OPENQASM 3.0;\n" + source, qde::BackendConfig{});
}

// ---- syntax -----------------------------------------------------------------

TEST(Parser, ValidProgramIsOk) {
  auto result = parse("qubit q;\nh q;\n");
  EXPECT_TRUE(result.isOk());
  EXPECT_TRUE(result.errors().empty());
}

TEST(Parser, InvalidTokenFails) {
  auto result = parse("???\n");
  EXPECT_FALSE(result.isOk());
  EXPECT_FALSE(result.errors().empty());
}

TEST(Parser, ErrorReportsCorrectLocation) {
  qde::Parser parser;
  auto result = qde::Parser::parse("OPENQASM 3.0;\nqubit q;\n???\n",
                                   qde::BackendConfig{});
  ASSERT_FALSE(result.errors().empty());
  EXPECT_EQ(result.errors().front().line, 3U);
  EXPECT_EQ(result.errors().front().column, 0U);
}

TEST(Parser, EmptyInputIsOk) {
  auto result = parse("");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, MultipleErrorsAreDetected) {
  auto result = parse("???\n!!!\n");
  EXPECT_GT(result.errors().size(), 1U);
}

// ---- qubit / bit register declarations --------------------------------------

TEST(Parser, QubitRegisterDeclared) {
  auto result = parse("qubit[3] q;");
  ASSERT_TRUE(result.isOk());
  ASSERT_EQ(result.circuit().qubitRegisters().size(), 1U);
  EXPECT_EQ(result.circuit().qubitRegisters()[0].name, "q");
  EXPECT_EQ(result.circuit().qubitRegisters()[0].size, 3U);
}

TEST(Parser, QubitNoDesignatorIsSize1) {
  auto result = parse("qubit q;");
  ASSERT_TRUE(result.isOk());
  EXPECT_EQ(result.circuit().qubitRegisters()[0].size, 1U);
}

TEST(Parser, BitRegisterDeclared) {
  auto result = parse("bit[2] c;");
  ASSERT_TRUE(result.isOk());
  ASSERT_EQ(result.circuit().bitRegisters().size(), 1U);
  EXPECT_EQ(result.circuit().bitRegisters()[0].name, "c");
  EXPECT_EQ(result.circuit().bitRegisters()[0].size, 2U);
}

TEST(Parser, BitNoDesignatorIsSize1) {
  auto result = parse("bit c;");
  ASSERT_TRUE(result.isOk());
  EXPECT_EQ(result.circuit().bitRegisters()[0].size, 1U);
}

TEST(Parser, MultipleRegisters) {
  auto result = parse("qubit[2] a;\nqubit[3] b;\nbit[2] c;");
  ASSERT_TRUE(result.isOk());
  EXPECT_EQ(result.circuit().qubitRegisters().size(), 2U);
  EXPECT_EQ(result.circuit().bitRegisters().size(), 1U);
}

TEST(Parser, DuplicateQubitRegisterIsError) {
  auto result = parse("qubit[2] q;\nqubit[1] q;");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, DuplicateBitRegisterIsError) {
  auto result = parse("bit[2] c;\nbit[1] c;");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, QubitAndBitSameNameIsError) {
  auto result = parse("qubit q;\nbit q;");
  EXPECT_FALSE(result.isOk());
}

// ---- old-style declarations (OpenQASM 2.0 qreg / creg) ---------------------

TEST(Parser, QregDeclared) {
  auto result = parse("qreg q[3];");
  ASSERT_TRUE(result.isOk());
  ASSERT_EQ(result.circuit().qubitRegisters().size(), 1U);
  EXPECT_EQ(result.circuit().qubitRegisters()[0].name, "q");
  EXPECT_EQ(result.circuit().qubitRegisters()[0].size, 3U);
}

TEST(Parser, CregDeclared) {
  auto result = parse("creg c[2];");
  ASSERT_TRUE(result.isOk());
  ASSERT_EQ(result.circuit().bitRegisters().size(), 1U);
  EXPECT_EQ(result.circuit().bitRegisters()[0].name, "c");
  EXPECT_EQ(result.circuit().bitRegisters()[0].size, 2U);
}

TEST(Parser, DuplicateQregIsError) {
  auto result = parse("qreg q[2];\nqreg q[1];");
  EXPECT_FALSE(result.isOk());
}

// ---- gate calls: single-qubit -----------------------------------------------

TEST(Parser, HGateOnIndexedQubit) {
  auto result = parse("qubit[2] q;\nh q[0];");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, HGateOnSingleQubit) {
  auto result = parse("qubit q;\nh q;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, XGate) {
  auto result = parse("qubit q;\nx q;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, GateCallProducesGateOperation) {
  auto result = parse("qubit q;\nh q;");
  ASSERT_TRUE(result.isOk());
  ASSERT_EQ(result.circuit().operations().size(), 1U);
  EXPECT_EQ(result.circuit().operations()[0].type, qde::OperationType::kGate);
}

TEST(Parser, MultipleGateCalls) {
  auto result = parse("qubit q;\nh q;\nx q;\nh q;");
  ASSERT_TRUE(result.isOk());
  EXPECT_EQ(result.circuit().operations().size(), 3U);
}

// ---- gate calls: parameterized single-qubit ---------------------------------

TEST(Parser, RxWithLiteral) {
  auto result = parse("qubit q;\nrx(pi) q;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, RxWithArithmeticExpr) {
  auto result = parse("qubit q;\nrx(2.0 * pi) q;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, RxWithConstParam) {
  auto result = parse("qubit q;\nconst float a = pi / 2;\nrx(a) q;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, RyGate) {
  auto result = parse("qubit q;\nry(pi / 2) q;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, RzGate) {
  auto result = parse("qubit q;\nrz(pi / 4) q;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, RxMissingParamIsError) {
  auto result = parse("qubit q;\nrx q;");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, HWithExtraParamIsError) {
  auto result = parse("qubit q;\nh(pi) q;");
  EXPECT_FALSE(result.isOk());
}

// ---- gate calls: multi-qubit ------------------------------------------------

TEST(Parser, CxTwoIndexedQubits) {
  auto result = parse("qubit[2] q;\ncx q[0], q[1];");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, CxTwoSeparateRegisters) {
  auto result = parse("qubit a;\nqubit b;\ncx a, b;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, CxWrongQubitCountIsError) {
  auto result = parse("qubit q;\ncx q;");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, CcxThreeQubits) {
  auto result = parse("qubit[3] q;\nccx q[0], q[1], q[2];");
  EXPECT_TRUE(result.isOk());
}

// ---- gate calls: register broadcast -----------------------------------------

TEST(Parser, SingleQubitBroadcastExpandsToN) {
  auto result = parse("qubit[3] q;\nh q;");
  ASSERT_TRUE(result.isOk());
  EXPECT_EQ(result.circuit().operations().size(), 3U);
}

TEST(Parser, PairwiseBroadcastEqualRegisters) {
  auto result = parse("qubit[2] a;\nqubit[2] b;\ncx a, b;");
  ASSERT_TRUE(result.isOk());
  EXPECT_EQ(result.circuit().operations().size(), 2U);
}

TEST(Parser, PairwiseBroadcastMismatchedSizesFallsThrough) {
  // Unequal registers skip broadcast; each whole-register operand
  // resolves to qubit 0 of its register, giving one cx operation.
  auto result = parse("qubit[2] a;\nqubit[3] b;\ncx a, b;");
  ASSERT_TRUE(result.isOk());
  EXPECT_EQ(result.circuit().operations().size(), 1U);
}

// ---- gate call modifiers ----------------------------------------------------

TEST(Parser, InvModifier) {
  auto result = parse("qubit q;\ninv @ h q;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, InvModifierProducesOperation) {
  auto result = parse("qubit q;\ninv @ h q;");
  ASSERT_TRUE(result.isOk());
  ASSERT_EQ(result.circuit().operations().size(), 1U);
  EXPECT_EQ(result.circuit().operations()[0].type, qde::OperationType::kGate);
}

TEST(Parser, PowModifier) {
  auto result = parse("qubit q;\npow(2) @ x q;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, CtrlModifierOneControl) {
  auto result = parse("qubit[2] q;\nctrl @ x q[0], q[1];");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, CtrlModifierCount) {
  auto result = parse("qubit[3] q;\nctrl(2) @ x q[0], q[1], q[2];");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, NegCtrlModifier) {
  auto result = parse("qubit[2] q;\nnegctrl @ x q[0], q[1];");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, ModifierChain) {
  auto result = parse("qubit[2] q;\nctrl @ inv @ h q[0], q[1];");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, ModifierProducesOneOperation) {
  auto result = parse("qubit[2] q;\nctrl @ x q[0], q[1];");
  ASSERT_TRUE(result.isOk());
  EXPECT_EQ(result.circuit().operations().size(), 1U);
}

TEST(Parser, ParametricBodyGateWithDeferredModifier) {
  // ctrl @ rx(theta) in a gate body takes the deferred-modifier lambda
  // path: modifiers are recorded and applied at each call site.
  auto result = parse(
      "gate my_crx(theta) c, t { ctrl @ rx(theta) c, t; }\n"
      "qubit[2] q;\n"
      "my_crx(pi / 2) q[0], q[1];\n");
  ASSERT_TRUE(result.isOk());
  EXPECT_EQ(result.circuit().operations().size(), 1U);
}

// ---- gphase -----------------------------------------------------------------

TEST(Parser, BareGphaseAddsNoOperation) {
  auto result = parse("qubit q;\ngphase(pi);");
  ASSERT_TRUE(result.isOk());
  EXPECT_EQ(result.circuit().operations().size(), 0U);
}

TEST(Parser, CtrlGphaseAddsOperation) {
  auto result = parse("qubit q;\nctrl @ gphase(pi) q;");
  ASSERT_TRUE(result.isOk());
  ASSERT_EQ(result.circuit().operations().size(), 1U);
  EXPECT_EQ(result.circuit().operations()[0].type, qde::OperationType::kGate);
}

// ---- gate definitions -------------------------------------------------------

TEST(Parser, UserGateDefinedAndCalled) {
  auto result = parse(
      "gate my_h q { h q; }\n"
      "qubit q;\n"
      "my_h q;\n");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, UserGateProducesOneOperation) {
  auto result = parse(
      "gate my_h q { h q; }\n"
      "qubit q;\n"
      "my_h q;\n");
  ASSERT_TRUE(result.isOk());
  EXPECT_EQ(result.circuit().operations().size(), 1U);
}

TEST(Parser, ParametricUserGate) {
  auto result = parse(
      "gate my_rx(a) q { rx(a) q; }\n"
      "qubit q;\n"
      "my_rx(pi) q;\n");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, UserGateWithInvBodyModifier) {
  auto result = parse(
      "gate my_hinv q { inv @ h q; }\n"
      "qubit q;\n"
      "my_hinv q;\n");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, GateRedefinitionIsError) {
  auto result = parse(
      "gate my_g q { h q; }\n"
      "gate my_g q { x q; }\n");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, UndefinedGateInBodyIsError) {
  auto result = parse("gate bad q { nosuchgate q; }\n");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, GateNotVisibleBeforeDefinition) {
  auto result = parse(
      "qubit q;\n"
      "my_g q;\n"
      "gate my_g q { h q; }\n");
  EXPECT_FALSE(result.isOk());
}

// ---- measurements -----------------------------------------------------------

TEST(Parser, MeasureArrowIndexed) {
  auto result = parse("qubit q;\nbit c;\nmeasure q -> c;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, MeasureArrowWholeRegister) {
  auto result = parse("qubit[2] q;\nbit[2] c;\nmeasure q -> c;");
  ASSERT_TRUE(result.isOk());
  EXPECT_EQ(result.circuit().operations().size(), 2U);
}

TEST(Parser, MeasureArrowNoTarget) {
  auto result = parse("qubit q;\nmeasure q;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, MeasureArrowProducesMeasureOperation) {
  auto result = parse("qubit q;\nbit c;\nmeasure q -> c;");
  ASSERT_TRUE(result.isOk());
  ASSERT_EQ(result.circuit().operations().size(), 1U);
  EXPECT_EQ(result.circuit().operations()[0].type,
            qde::OperationType::kMeasure);
}

TEST(Parser, MeasureArrowRegisterSizeMismatchIsError) {
  auto result = parse("qubit[2] q;\nbit[3] c;\nmeasure q -> c;");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, MeasureArrowUndefinedQubitIsError) {
  auto result = parse("bit c;\nmeasure q -> c;");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, MeasureArrowUndefinedBitIsError) {
  auto result = parse("qubit q;\nmeasure q -> c;");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, QuantumCallExpressionInMeasureArrowIsOk) {
  // quantumCallExpression form: "gateName qubits -> c;" is parsed as
  // measureArrowAssignment. Currently not handled; no operation added.
  auto result = parse("qubit q;\nbit c;\nh q -> c;");
  ASSERT_TRUE(result.isOk());
  EXPECT_EQ(result.circuit().operations().size(), 0U);
}

TEST(Parser, MeasureAssignmentForm) {
  auto result = parse("qubit q;\nbit c;\nc = measure q;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, MeasureAssignmentIndexed) {
  auto result = parse("qubit[2] q;\nbit[2] c;\nc[0] = measure q[0];");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, MeasureAssignmentUndefinedBitIsError) {
  auto result = parse("qubit q;\nc = measure q;");
  EXPECT_FALSE(result.isOk());
}

// ---- reset ------------------------------------------------------------------

TEST(Parser, ResetIndexedQubit) {
  auto result = parse("qubit[2] q;\nreset q[0];");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, ResetWholeRegisterExpandsToN) {
  auto result = parse("qubit[3] q;\nreset q;");
  ASSERT_TRUE(result.isOk());
  EXPECT_EQ(result.circuit().operations().size(), 3U);
}

TEST(Parser, ResetProducesResetOperation) {
  auto result = parse("qubit q;\nreset q;");
  ASSERT_TRUE(result.isOk());
  ASSERT_EQ(result.circuit().operations().size(), 1U);
  EXPECT_EQ(result.circuit().operations()[0].type, qde::OperationType::kReset);
}

TEST(Parser, ResetUndefinedQubitIsError) {
  auto result = parse("reset q;");
  EXPECT_FALSE(result.isOk());
}

// ---- barrier ----------------------------------------------------------------

TEST(Parser, BarrierGlobalExpandsOverAllQubits) {
  auto result = parse("qubit[2] q;\nbarrier;");
  ASSERT_TRUE(result.isOk());
  ASSERT_EQ(result.circuit().operations().size(), 1U);
  EXPECT_EQ(result.circuit().operations()[0].type,
            qde::OperationType::kBarrier);
  EXPECT_EQ(result.circuit().operations()[0].qubits.size(), 2U);
}

TEST(Parser, BarrierNoRegistersHasNoQubits) {
  auto result = parse("barrier;");
  ASSERT_TRUE(result.isOk());
  EXPECT_TRUE(result.circuit().operations()[0].qubits.empty());
}

TEST(Parser, BarrierWholeRegister) {
  auto result = parse("qubit[2] q;\nbarrier q;");
  ASSERT_TRUE(result.isOk());
  EXPECT_EQ(result.circuit().operations()[0].qubits.size(), 2U);
}

TEST(Parser, BarrierIndexedQubits) {
  auto result = parse("qubit[3] q;\nbarrier q[0], q[2];");
  ASSERT_TRUE(result.isOk());
  EXPECT_EQ(result.circuit().operations()[0].qubits.size(), 2U);
}

TEST(Parser, BarrierUndefinedQubitIsError) {
  auto result = parse("barrier q;");
  EXPECT_FALSE(result.isOk());
}

// ---- classical declarations -------------------------------------------------

TEST(Parser, FloatDeclarationWithInit) {
  auto result = parse("float x = 3.14;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, IntDeclarationWithInit) {
  auto result = parse("int n = 42;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, UintDeclarationWithInit) {
  auto result = parse("uint n = 7;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, AngleDeclaration) {
  auto result = parse("angle a = pi / 4;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, DurationDeclaration) {
  auto result = parse("duration d = 100ns;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, StretchDeclaration) {
  auto result = parse("stretch s;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, ComplexDeclaration) {
  auto result = parse("complex[float] z;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, ComplexDeclarationPureImaginary) {
  auto result = parse("complex[float] z = 2.5im;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, ComplexDeclarationRealPlusImaginary) {
  auto result = parse("complex[float] z = 1.0 + 2.5im;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, ComplexVariableAssignment) {
  auto result = parse(
      "complex[float] z = 0.0;\n"
      "z = 1.0 + 2.5im;\n");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, DurationCompoundAssignment) {
  auto result = parse("duration d = 100ns;\nd += 50ns;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, StretchCompoundAssignment) {
  auto result = parse("stretch s;\ns += 1.0;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, DuplicateVariableIsError) {
  auto result = parse("float x = 1.0;\nfloat x = 2.0;");
  EXPECT_FALSE(result.isOk());
}

// ---- classical assignments --------------------------------------------------

TEST(Parser, ScalarAssignment) {
  auto result = parse("float x = 0.0;\nx = pi;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, CompoundAddAssignment) {
  auto result = parse("float x = 1.0;\nx += 2.0;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, CompoundSubAssignment) {
  auto result = parse("float x = 5.0;\nx -= 3.0;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, CompoundMulAssignment) {
  auto result = parse("float x = 2.0;\nx *= pi;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, CompoundDivAssignment) {
  auto result = parse("float x = pi;\nx /= 2.0;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, CompoundModAssignment) {
  auto result = parse("int x = 7;\nx %= 3;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, CompoundPowAssignment) {
  auto result = parse("float x = 2.0;\nx **= 8;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, CompoundBitwiseAndAssignment) {
  auto result = parse("int x = 0b1100;\nx &= 0b1010;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, CompoundBitwiseOrAssignment) {
  auto result = parse("int x = 0b1100;\nx |= 0b0011;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, CompoundLeftShiftAssignment) {
  auto result = parse("int x = 1;\nx <<= 3;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, CompoundRightShiftAssignment) {
  auto result = parse("int x = 8;\nx >>= 2;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, AssignmentUpdatedValueUsedAsGateParam) {
  auto result = parse(
      "qubit q;\n"
      "float a = pi / 4;\n"
      "a = pi / 2;\n"
      "rx(a) q;\n");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, AssignmentToUndefinedVariableIsError) {
  auto result = parse("x = 1.0;");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, ConstReassignmentIsError) {
  auto result = parse("const float x = 1.0;\nx = 2.0;");
  EXPECT_FALSE(result.isOk());
}

// ---- expression tree: literals ----------------------------------------------

TEST(Parser, DecimalIntegerLiteral) {
  auto result = parse("const int n = 42;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, FloatLiteral) {
  auto result = parse("const float x = 3.14;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, BooleanLiteralTrue) {
  auto result = parse("const float b = true;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, BooleanLiteralFalse) {
  auto result = parse("const float b = false;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, HexLiteral) {
  auto result = parse("const int n = 0xFF;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, BinaryLiteral) {
  auto result = parse("const int n = 0b1010;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, OctalLiteral) {
  auto result = parse("const int n = 0o17;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, PiConstant) {
  auto result = parse("const float x = pi;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, TauConstant) {
  auto result = parse("const float x = tau;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, EulerConstant) {
  auto result = parse("const float x = euler;");
  EXPECT_TRUE(result.isOk());
}

// ---- expression tree: arithmetic --------------------------------------------

TEST(Parser, Addition) {
  auto result = parse("const float x = 1.0 + 2.0;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, Subtraction) {
  auto result = parse("const float x = 5.0 - 3.0;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, Multiplication) {
  auto result = parse("const float x = 2.0 * pi;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, Division) {
  auto result = parse("const float x = pi / 2;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, Modulo) {
  auto result = parse("const int x = 7 % 3;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, Power) {
  auto result = parse("const float x = 2.0 ** 8;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, UnaryNegation) {
  auto result = parse("const float x = -pi;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, Parentheses) {
  auto result = parse("const float x = (1.0 + 2.0) * 3.0;");
  EXPECT_TRUE(result.isOk());
}

// ---- expression tree: bitwise / logical ------------------------------------

TEST(Parser, BitwiseAnd) {
  auto result = parse("const int x = 0b1100 & 0b1010;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, BitwiseOr) {
  auto result = parse("const int x = 0b1100 | 0b0011;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, BitwiseXor) {
  auto result = parse("const int x = 0b1010 ^ 0b0110;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, BitwiseNot) {
  auto result = parse("const int x = ~0;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, LeftShift) {
  auto result = parse("const int x = 1 << 3;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, RightShift) {
  auto result = parse("const int x = 8 >> 2;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, LogicalNot) {
  auto result = parse("const int x = !0;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, LogicalAnd) {
  auto result = parse("const int x = 1 && 0;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, LogicalOr) {
  auto result = parse("const int x = 0 || 1;");
  EXPECT_TRUE(result.isOk());
}

// ---- expression tree: comparison --------------------------------------------

TEST(Parser, LessThan) {
  auto result = parse("const int x = 1 < 2;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, GreaterThan) {
  auto result = parse("const int x = 2 > 1;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, LessOrEqual) {
  auto result = parse("const int x = 1 <= 2;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, GreaterOrEqual) {
  auto result = parse("const int x = 2 >= 1;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, EqualityCheck) {
  auto result = parse("const int x = 1 == 1;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, InequalityCheck) {
  auto result = parse("const int x = 1 != 2;");
  EXPECT_TRUE(result.isOk());
}

// ---- expression tree: math functions ----------------------------------------

TEST(Parser, SinFunction) {
  auto result = parse("const float x = sin(pi / 6);");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, CosFunction) {
  auto result = parse("const float x = cos(pi / 3);");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, TanFunction) {
  auto result = parse("const float x = tan(pi / 4);");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, ArcsinFunction) {
  auto result = parse("const float x = arcsin(1.0);");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, ArccosFunction) {
  auto result = parse("const float x = arccos(0.0);");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, ArctanFunction) {
  auto result = parse("const float x = arctan(1.0);");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, SqrtFunction) {
  auto result = parse("const float x = sqrt(2.0);");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, ExpFunction) {
  auto result = parse("const float x = exp(1.0);");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, LnFunction) {
  auto result = parse("const float x = ln(euler);");
  EXPECT_TRUE(result.isOk());
}

// ---- expression tree: cast --------------------------------------------------

TEST(Parser, IntCastTruncates) {
  auto result = parse("const int n = int(3.9);");
  EXPECT_TRUE(result.isOk());
}

// ---- expression tree: const chaining ----------------------------------------

TEST(Parser, ConstReferencesOtherConst) {
  auto result = parse("const float a = pi / 2;\nconst float b = a + 1.0;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, ConstUsedAsGateParam) {
  auto result = parse("qubit q;\nconst float a = pi / 2;\nrx(a) q;");
  EXPECT_TRUE(result.isOk());
}

// ---- expression tree: arrays ------------------------------------------------

TEST(Parser, ArrayDeclaration) {
  auto result = parse("array[float, 3] a = {1.0, 2.0, 3.0};");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, ArrayElementAccess) {
  auto result = parse(
      "qubit q;\n"
      "array[float, 3] a = {pi/4, pi/2, pi};\n"
      "rx(a[0]) q;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, Array2D) {
  auto result = parse(
      "qubit q;\n"
      "qubit t;\n"
      "array[float, 2, 2] m = {{pi/4, pi/2}, {pi, 0.0}};\n"
      "rx(m[0, 1]) q;\n"
      "rx(m[0][1]) t;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, Array3D) {
  auto result = parse(
      "qubit q;\n"
      "qubit t;\n"
      "array[float, 2, 2, 2] m ="
      "{{{pi/4, pi/2}, {pi, 0.0}}, {{pi/4, pi/2}, {pi, 0.0}}};\n"
      "rx(m[0, 1, 0]) q;"
      "rx(m[0][1][0]) t;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, UndefinedArrayIsError) {
  auto result = parse("qubit q;\nrx(undefined_arr[0]) q;");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, ArrayIndexOutOfBoundsIsError) {
  auto result = parse(
      "qubit q;\n"
      "array[float, 3] a = {1.0, 2.0, 3.0};\n"
      "rx(a[5]) q;");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, ArrayCastFromOtherArray) {
  // array[T, N](src) copies elements from another declared array
  auto result = parse(
      "qubit q;\n"
      "array[float, 3] a = {pi/4, pi/2, pi};\n"
      "array[float, 3] b = array[float, 3](a);\n"
      "rx(b[0]) q;\n");
  EXPECT_TRUE(result.isOk());

  auto result3d = parse(
      "qubit q;\n"
      "array[float, 2, 2, 2] a ="
      "{{{pi/4, pi/2}, {pi, 0.0}}, {{pi/4, pi/2}, {pi, 0.0}}};\n"
      "array[float, 2, 2, 2] b = array[float, 2, 2, 2](a);\n"
      "rx(b[0, 1, 0]) q;\n");
  EXPECT_TRUE(result3d.isOk());
}

// ---- semantic errors --------------------------------------------------------

TEST(Parser, UndefinedGateIsError) {
  auto result = parse("qubit q;\nfoo q;");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, UndefinedQubitRegisterIsError) {
  auto result = parse("h q;");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, WrongQubitCountIsError) {
  auto result = parse("qubit[2] q;\nh q[0], q[1];");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, UndefinedVariableIsError) {
  auto result = parse("float x = undef;");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, UndefinedVariableInGateParamIsError) {
  auto result = parse("qubit q;\nrx(undef) q;");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, UndefinedQubitInMeasureIsError) {
  auto result = parse("bit c;\nmeasure q -> c;");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, UndefinedBitInMeasureIsError) {
  auto result = parse("qubit q;\nmeasure q -> c;");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, UndefinedQubitInResetIsError) {
  auto result = parse("reset q;");
  EXPECT_FALSE(result.isOk());
}

TEST(Parser, HardwareQubitOperandIsError) {
  auto result = parse("h $0;");
  EXPECT_FALSE(result.isOk());
}

// ---- unimplemented stubs (accepted without semantic error) ------------------

TEST(Parser, IfStatementIsOk) {
  auto result = parse("qubit q;\nif (1 == 1) { h q; }");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, WhileStatementIsOk) {
  auto result = parse("qubit q;\nwhile (false) { h q; }");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, ForStatementIsOk) {
  auto result = parse("qubit q;\nfor int i in {0, 1} { h q; }");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, BoxStatementIsOk) {
  auto result = parse("qubit q;\nbox { h q; }");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, DelayStatementIsOk) {
  auto result = parse("qubit q;\ndelay[100ns] q;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, EndStatementIsOk) {
  auto result = parse("end;");
  EXPECT_TRUE(result.isOk());
}

TEST(Parser, DefStatementIsOk) {
  auto result = parse("def foo(float x) { }");
  EXPECT_TRUE(result.isOk());
}
