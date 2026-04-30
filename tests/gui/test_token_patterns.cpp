#include <gtest/gtest.h>
#include <QRegularExpression>
#include <QString>

#include "qde/gui/token_patterns.hpp"

namespace qde::gui {

namespace {

bool matches(const char* pattern, const char* text) {
  return QRegularExpression(QLatin1String(pattern))
      .match(QLatin1String(text))
      .hasMatch();
}

bool matchesUtf8(const char* pattern, const char* text) {
  return QRegularExpression(QString::fromUtf8(pattern))
      .match(QString::fromUtf8(text))
      .hasMatch();
}

}  // namespace

// --- Numbers -----------------------------------------------------------------

TEST(TokenPatternsNumbers, TimingLiteralMatchesIntegerUnits) {
  EXPECT_TRUE(matches(patterns::kTimingLiteral, "1dt"));
  EXPECT_TRUE(matches(patterns::kTimingLiteral, "100ns"));
  EXPECT_TRUE(matches(patterns::kTimingLiteral, "50us"));
  EXPECT_TRUE(matches(patterns::kTimingLiteral, "20ms"));
  EXPECT_TRUE(matches(patterns::kTimingLiteral, "2s"));
}

TEST(TokenPatternsNumbers, TimingLiteralMatchesFloatUnits) {
  EXPECT_TRUE(matches(patterns::kTimingLiteral, "1.5ns"));
  EXPECT_TRUE(matches(patterns::kTimingLiteral, "3.14ms"));
  EXPECT_TRUE(matchesUtf8(patterns::kTimingLiteral, "0.5µs"));
}

TEST(TokenPatternsNumbers, TimingLiteralNoMatchWithoutUnit) {
  EXPECT_FALSE(matches(patterns::kTimingLiteral, "100"));
  EXPECT_FALSE(matches(patterns::kTimingLiteral, "1.5"));
  EXPECT_FALSE(matches(patterns::kTimingLiteral, "ns"));
}

TEST(TokenPatternsNumbers, ImaginaryLiteralMatches) {
  EXPECT_TRUE(matches(patterns::kImaginaryLiteral, "1im"));
  EXPECT_TRUE(matches(patterns::kImaginaryLiteral, "1.5im"));
  EXPECT_TRUE(matches(patterns::kImaginaryLiteral, "3.14im"));
}

TEST(TokenPatternsNumbers, ImaginaryLiteralNoMatchWithoutPrefix) {
  EXPECT_FALSE(matches(patterns::kImaginaryLiteral, "im"));
  EXPECT_FALSE(matches(patterns::kImaginaryLiteral, "1"));
}

TEST(TokenPatternsNumbers, HexIntegerMatches) {
  EXPECT_TRUE(matches(patterns::kHexIntegerLiteral, "0xFF"));
  EXPECT_TRUE(matches(patterns::kHexIntegerLiteral, "0xABCD"));
  EXPECT_TRUE(matches(patterns::kHexIntegerLiteral, "0X1a"));
  EXPECT_TRUE(matches(patterns::kHexIntegerLiteral, "0xff_00"));
}

TEST(TokenPatternsNumbers, HexIntegerNoMatchForOtherBases) {
  EXPECT_FALSE(matches(patterns::kHexIntegerLiteral, "0b101"));
  EXPECT_FALSE(matches(patterns::kHexIntegerLiteral, "0o7"));
  EXPECT_FALSE(matches(patterns::kHexIntegerLiteral, "123"));
}

TEST(TokenPatternsNumbers, OctalIntegerMatches) {
  EXPECT_TRUE(matches(patterns::kOctalIntegerLiteral, "0o7"));
  EXPECT_TRUE(matches(patterns::kOctalIntegerLiteral, "0o755"));
  EXPECT_TRUE(matches(patterns::kOctalIntegerLiteral, "0o0"));
}

TEST(TokenPatternsNumbers, OctalIntegerNoMatchForOtherBases) {
  EXPECT_FALSE(matches(patterns::kOctalIntegerLiteral, "0xFF"));
  EXPECT_FALSE(matches(patterns::kOctalIntegerLiteral, "0b101"));
  EXPECT_FALSE(matches(patterns::kOctalIntegerLiteral, "755"));
}

TEST(TokenPatternsNumbers, BinaryIntegerMatches) {
  EXPECT_TRUE(matches(patterns::kBinaryIntegerLiteral, "0b101"));
  EXPECT_TRUE(matches(patterns::kBinaryIntegerLiteral, "0B1010"));
  EXPECT_TRUE(matches(patterns::kBinaryIntegerLiteral, "0b0"));
  EXPECT_TRUE(matches(patterns::kBinaryIntegerLiteral, "0b1_0_1"));
}

TEST(TokenPatternsNumbers, BinaryIntegerNoMatchForOtherBases) {
  EXPECT_FALSE(matches(patterns::kBinaryIntegerLiteral, "0xFF"));
  EXPECT_FALSE(matches(patterns::kBinaryIntegerLiteral, "0o7"));
  EXPECT_FALSE(matches(patterns::kBinaryIntegerLiteral, "101"));
}

TEST(TokenPatternsNumbers, FloatLiteralMatchesDotForms) {
  EXPECT_TRUE(matches(patterns::kFloatLiteral, "1.0"));
  EXPECT_TRUE(matches(patterns::kFloatLiteral, "1."));
  EXPECT_TRUE(matches(patterns::kFloatLiteral, ".5"));
  EXPECT_TRUE(matches(patterns::kFloatLiteral, "1.5e3"));
  EXPECT_TRUE(matches(patterns::kFloatLiteral, "1.5e-3"));
  EXPECT_TRUE(matches(patterns::kFloatLiteral, "1.5e+3"));
}

TEST(TokenPatternsNumbers, FloatLiteralMatchesExponentForm) {
  EXPECT_TRUE(matches(patterns::kFloatLiteral, "1e3"));
  EXPECT_TRUE(matches(patterns::kFloatLiteral, "1e-10"));
}

TEST(TokenPatternsNumbers, FloatLiteralNoMatchForInteger) {
  EXPECT_FALSE(matches(patterns::kFloatLiteral, "42"));
  EXPECT_FALSE(matches(patterns::kFloatLiteral, "0xFF"));
}

TEST(TokenPatternsNumbers, DecimalIntegerMatches) {
  EXPECT_TRUE(matches(patterns::kDecimalIntegerLiteral, "0"));
  EXPECT_TRUE(matches(patterns::kDecimalIntegerLiteral, "123"));
  EXPECT_TRUE(matches(patterns::kDecimalIntegerLiteral, "1_000"));
}

// --- Identifiers -------------------------------------------------------------

TEST(TokenPatternsIdentifiers, HardwareQubitMatches) {
  EXPECT_TRUE(matches(patterns::kHardwareQubit, "$0"));
  EXPECT_TRUE(matches(patterns::kHardwareQubit, "$123"));
}

TEST(TokenPatternsIdentifiers, HardwareQubitNoMatchWithoutDigits) {
  EXPECT_FALSE(matches(patterns::kHardwareQubit, "$"));
  EXPECT_FALSE(matches(patterns::kHardwareQubit, "$abc"));
}

TEST(TokenPatternsIdentifiers, AnnotationKeywordMatches) {
  EXPECT_TRUE(matches(patterns::kAnnotationKeyword, "@foo"));
  EXPECT_TRUE(matches(patterns::kAnnotationKeyword, "@foo.bar"));
  EXPECT_TRUE(matches(patterns::kAnnotationKeyword, "@foo.bar.baz"));
  EXPECT_TRUE(matches(patterns::kAnnotationKeyword, "@_private"));
}

TEST(TokenPatternsIdentifiers, AnnotationKeywordNoMatchBareAt) {
  EXPECT_FALSE(matches(patterns::kAnnotationKeyword, "@"));
  EXPECT_FALSE(matches(patterns::kAnnotationKeyword, "@123"));
}

TEST(TokenPatternsIdentifiers, PragmaMatches) {
  EXPECT_TRUE(matches(patterns::kPragma, "#pragma"));
  EXPECT_TRUE(matches(patterns::kPragma, "#dim"));
  EXPECT_TRUE(matches(patterns::kPragma, "# pragma"));
}

TEST(TokenPatternsIdentifiers, PragmaNoMatchOtherHash) {
  EXPECT_FALSE(matches(patterns::kPragma, "pragma"));
  EXPECT_FALSE(matches(patterns::kPragma, "#include"));
  EXPECT_FALSE(matches(patterns::kPragma, "#foo"));
}

// --- Keywords, types, gates --------------------------------------------------

TEST(TokenPatternsKeywords, KeywordMatches) {
  EXPECT_TRUE(matches(patterns::kKeyword, "gate"));
  EXPECT_TRUE(matches(patterns::kKeyword, "if"));
  EXPECT_TRUE(matches(patterns::kKeyword, "for"));
  EXPECT_TRUE(matches(patterns::kKeyword, "OPENQASM"));
  EXPECT_TRUE(matches(patterns::kKeyword, "measure"));
  EXPECT_TRUE(matches(patterns::kKeyword, "reset"));
  EXPECT_TRUE(matches(patterns::kKeyword, "barrier"));
}

TEST(TokenPatternsKeywords, KeywordRespectWordBoundary) {
  EXPECT_FALSE(matches(patterns::kKeyword, "gateway"));
  EXPECT_FALSE(matches(patterns::kKeyword, "iffy"));
  EXPECT_FALSE(matches(patterns::kKeyword, "forloop"));
  EXPECT_FALSE(matches(patterns::kKeyword, "defcalx"));
}

TEST(TokenPatternsKeywords, TypeMatches) {
  EXPECT_TRUE(matches(patterns::kType, "qubit"));
  EXPECT_TRUE(matches(patterns::kType, "int"));
  EXPECT_TRUE(matches(patterns::kType, "float"));
  EXPECT_TRUE(matches(patterns::kType, "bool"));
  EXPECT_TRUE(matches(patterns::kType, "angle"));
  EXPECT_TRUE(matches(patterns::kType, "duration"));
}

TEST(TokenPatternsKeywords, TypeRespectWordBoundary) {
  EXPECT_FALSE(matches(patterns::kType, "qubitregister"));
  EXPECT_FALSE(matches(patterns::kType, "integer"));
  EXPECT_FALSE(matches(patterns::kType, "floating"));
  EXPECT_FALSE(matches(patterns::kType, "boolean"));
}

TEST(TokenPatternsKeywords, GateMatchesBuiltinModifiers) {
  EXPECT_TRUE(matches(patterns::kGate, "gphase"));
  EXPECT_TRUE(matches(patterns::kGate, "inv"));
  EXPECT_TRUE(matches(patterns::kGate, "ctrl"));
  EXPECT_TRUE(matches(patterns::kGate, "negctrl"));
}

TEST(TokenPatternsKeywords, GateMatchesStdlibSingleQubit) {
  EXPECT_TRUE(matches(patterns::kGate, "h"));
  EXPECT_TRUE(matches(patterns::kGate, "x"));
  EXPECT_TRUE(matches(patterns::kGate, "rx"));
  EXPECT_TRUE(matches(patterns::kGate, "sdg"));
  EXPECT_TRUE(matches(patterns::kGate, "sx"));
  EXPECT_TRUE(matches(patterns::kGate, "sxdg"));
}

TEST(TokenPatternsKeywords, GateMatchesStdlibMultiQubit) {
  EXPECT_TRUE(matches(patterns::kGate, "cx"));
  EXPECT_TRUE(matches(patterns::kGate, "swap"));
  EXPECT_TRUE(matches(patterns::kGate, "iswap"));
  EXPECT_TRUE(matches(patterns::kGate, "ccx"));
  EXPECT_TRUE(matches(patterns::kGate, "cswap"));
}

TEST(TokenPatternsKeywords, GateRespectWordBoundary) {
  EXPECT_FALSE(matches(patterns::kGate, "hx"));
  EXPECT_FALSE(matches(patterns::kGate, "cxx"));
  EXPECT_FALSE(matches(patterns::kGate, "swapper"));
  EXPECT_FALSE(matches(patterns::kGate, "rxgate"));
}

// --- Literals ----------------------------------------------------------------

TEST(TokenPatternsLiterals, ConstantLiteralMatchesBooleans) {
  EXPECT_TRUE(matches(patterns::kConstantLiteral, "true"));
  EXPECT_TRUE(matches(patterns::kConstantLiteral, "false"));
}

TEST(TokenPatternsLiterals, ConstantLiteralMatchesMathConstants) {
  EXPECT_TRUE(matches(patterns::kConstantLiteral, "pi"));
  EXPECT_TRUE(matches(patterns::kConstantLiteral, "tau"));
  EXPECT_TRUE(matches(patterns::kConstantLiteral, "euler"));
}

TEST(TokenPatternsLiterals, ConstantLiteralRespectWordBoundary) {
  EXPECT_FALSE(matches(patterns::kConstantLiteral, "truefalse"));
  EXPECT_FALSE(matches(patterns::kConstantLiteral, "True"));
  EXPECT_FALSE(matches(patterns::kConstantLiteral, "pipeline"));
  EXPECT_FALSE(matches(patterns::kConstantLiteral, "eulerian"));
}

TEST(TokenPatternsLiterals, BitstringLiteralMatches) {
  EXPECT_TRUE(matches(patterns::kBitstringLiteral, "\"0\""));
  EXPECT_TRUE(matches(patterns::kBitstringLiteral, "\"0101\""));
  EXPECT_TRUE(matches(patterns::kBitstringLiteral, "\"1_0_1\""));
}

TEST(TokenPatternsLiterals, BitstringLiteralNoMatchNonBinary) {
  EXPECT_FALSE(matches(patterns::kBitstringLiteral, "\"012\""));
  EXPECT_FALSE(matches(patterns::kBitstringLiteral, "\"abc\""));
}

TEST(TokenPatternsLiterals, StringLiteralMatchesDoubleQuoted) {
  EXPECT_TRUE(matches(patterns::kStringLiteral, "\"hello\""));
  EXPECT_TRUE(matches(patterns::kStringLiteral, "\"path/to/file.inc\""));
}

TEST(TokenPatternsLiterals, StringLiteralMatchesSingleQuoted) {
  EXPECT_TRUE(matches(patterns::kStringLiteral, "'hello'"));
  EXPECT_TRUE(matches(patterns::kStringLiteral, "'path/to/file.inc'"));
}

// --- User gate call ----------------------------------------------------------

TEST(TokenPatternsIdentifiers, UserGateCallMatchesLineStart) {
  EXPECT_TRUE(matches(patterns::kUserGateCall, "h q;"));
  EXPECT_TRUE(matches(patterns::kUserGateCall, "cx q1, q2;"));
  EXPECT_TRUE(matches(patterns::kUserGateCall, "myGate q;"));
}

TEST(TokenPatternsIdentifiers, UserGateCallMatchesIndented) {
  EXPECT_TRUE(matches(patterns::kUserGateCall, "  myGate q;"));
  EXPECT_TRUE(matches(patterns::kUserGateCall, "\th q;"));
}

TEST(TokenPatternsIdentifiers, UserGateCallMatchesAfterBrace) {
  EXPECT_TRUE(matches(patterns::kUserGateCall, "{ majority q;"));
  EXPECT_TRUE(matches(patterns::kUserGateCall, "{h q;"));
}

TEST(TokenPatternsIdentifiers, UserGateCallMatchesHardwareQubit) {
  EXPECT_TRUE(matches(patterns::kUserGateCall, "h $0;"));
}

TEST(TokenPatternsIdentifiers, UserGateCallExcludesAssignment) {
  EXPECT_FALSE(matches(patterns::kUserGateCall, "theta = pi;"));
  EXPECT_FALSE(matches(patterns::kUserGateCall, "theta += pi;"));
  EXPECT_FALSE(matches(patterns::kUserGateCall, "theta *= 2;"));
}

TEST(TokenPatternsIdentifiers, UserGateCallExcludesFunctionCall) {
  EXPECT_FALSE(matches(patterns::kUserGateCall, "myFunc(theta);"));
}

TEST(TokenPatternsIdentifiers, UserGateCallExcludesArrayAccess) {
  EXPECT_FALSE(matches(patterns::kUserGateCall, "c[0] = measure q;"));
}

// --- Function call -----------------------------------------------------------

TEST(TokenPatternsIdentifiers, FunctionCallMatches) {
  EXPECT_TRUE(matches(patterns::kFunctionCall, "myFunc(x)"));
  EXPECT_TRUE(matches(patterns::kFunctionCall, "sin(theta)"));
  EXPECT_TRUE(matches(patterns::kFunctionCall, "rx(pi/2) q;"));
}

TEST(TokenPatternsIdentifiers, FunctionCallMatchesWithSpace) {
  EXPECT_TRUE(matches(patterns::kFunctionCall, "sin (x)"));
}

TEST(TokenPatternsIdentifiers, FunctionCallMatchesInsideExpression) {
  EXPECT_TRUE(matches(patterns::kFunctionCall, "result = myFunc(x);"));
}

TEST(TokenPatternsIdentifiers, FunctionCallNoMatchWithoutParen) {
  EXPECT_FALSE(matches(patterns::kFunctionCall, "theta"));
  EXPECT_FALSE(matches(patterns::kFunctionCall, "theta = x;"));
}

// --- Catch-all identifier ----------------------------------------------------

TEST(TokenPatternsIdentifiers, IdentifierMatches) {
  EXPECT_TRUE(matches(patterns::kIdentifier, "myQubit"));
  EXPECT_TRUE(matches(patterns::kIdentifier, "theta"));
  EXPECT_TRUE(matches(patterns::kIdentifier, "_private"));
  EXPECT_TRUE(matches(patterns::kIdentifier, "q0"));
}

TEST(TokenPatternsIdentifiers, IdentifierRespectWordBoundary) {
  // digits-only and bare operators are not identifiers
  EXPECT_FALSE(matches(patterns::kIdentifier, "123"));
  EXPECT_FALSE(matches(patterns::kIdentifier, "+"));
  EXPECT_FALSE(matches(patterns::kIdentifier, "$0"));
}

// --- Operators ---------------------------------------------------------------

TEST(TokenPatternsOperators, SingleCharOperatorsMatch) {
  EXPECT_TRUE(matches(patterns::kOperator, "+"));
  EXPECT_TRUE(matches(patterns::kOperator, "-"));
  EXPECT_TRUE(matches(patterns::kOperator, "*"));
  EXPECT_TRUE(matches(patterns::kOperator, "/"));
  EXPECT_TRUE(matches(patterns::kOperator, "="));
  EXPECT_TRUE(matches(patterns::kOperator, "<"));
  EXPECT_TRUE(matches(patterns::kOperator, ">"));
  EXPECT_TRUE(matches(patterns::kOperator, "!"));
  EXPECT_TRUE(matches(patterns::kOperator, "&"));
  EXPECT_TRUE(matches(patterns::kOperator, "|"));
  EXPECT_TRUE(matches(patterns::kOperator, "^"));
  EXPECT_TRUE(matches(patterns::kOperator, "~"));
  EXPECT_TRUE(matches(patterns::kOperator, "%"));
}

TEST(TokenPatternsOperators, BracketOperatorsMatch) {
  EXPECT_TRUE(matches(patterns::kOperator, "["));
  EXPECT_TRUE(matches(patterns::kOperator, "]"));
  EXPECT_TRUE(matches(patterns::kOperator, "{"));
  EXPECT_TRUE(matches(patterns::kOperator, "}"));
  EXPECT_TRUE(matches(patterns::kOperator, "("));
  EXPECT_TRUE(matches(patterns::kOperator, ")"));
}

TEST(TokenPatternsOperators, PunctuationMatch) {
  EXPECT_TRUE(matches(patterns::kOperator, ";"));
  EXPECT_TRUE(matches(patterns::kOperator, ":"));
  EXPECT_TRUE(matches(patterns::kOperator, ","));
  EXPECT_TRUE(matches(patterns::kOperator, "."));
}

TEST(TokenPatternsOperators, MultiCharOperatorsMatch) {
  EXPECT_TRUE(matches(patterns::kOperator, "->"));
  EXPECT_TRUE(matches(patterns::kOperator, "++"));
  EXPECT_TRUE(matches(patterns::kOperator, "**"));
  EXPECT_TRUE(matches(patterns::kOperator, "||"));
  EXPECT_TRUE(matches(patterns::kOperator, "&&"));
  EXPECT_TRUE(matches(patterns::kOperator, "=="));
  EXPECT_TRUE(matches(patterns::kOperator, "!="));
  EXPECT_TRUE(matches(patterns::kOperator, ">="));
  EXPECT_TRUE(matches(patterns::kOperator, "<="));
  EXPECT_TRUE(matches(patterns::kOperator, ">>"));
  EXPECT_TRUE(matches(patterns::kOperator, "<<"));
}

TEST(TokenPatternsOperators, CompoundAssignmentOperatorsMatch) {
  EXPECT_TRUE(matches(patterns::kOperator, "+="));
  EXPECT_TRUE(matches(patterns::kOperator, "-="));
  EXPECT_TRUE(matches(patterns::kOperator, "*="));
  EXPECT_TRUE(matches(patterns::kOperator, "/="));
  EXPECT_TRUE(matches(patterns::kOperator, "&="));
  EXPECT_TRUE(matches(patterns::kOperator, "|="));
  EXPECT_TRUE(matches(patterns::kOperator, "^="));
  EXPECT_TRUE(matches(patterns::kOperator, "%="));
  EXPECT_TRUE(matches(patterns::kOperator, "<<="));
  EXPECT_TRUE(matches(patterns::kOperator, ">>="));
  EXPECT_TRUE(matches(patterns::kOperator, "**="));
}

TEST(TokenPatternsOperators, MultiCharMatchedBeforeSingleChar) {
  // "<<=" should match as one token, not "<" + "<" + "="
  auto re = QRegularExpression(QLatin1String(patterns::kOperator));
  EXPECT_EQ(re.match(QLatin1String("<<=")).captured(), QLatin1String("<<="));
  EXPECT_EQ(re.match(QLatin1String(">>=")).captured(), QLatin1String(">>="));
  EXPECT_EQ(re.match(QLatin1String("**=")).captured(), QLatin1String("**="));
  EXPECT_EQ(re.match(QLatin1String("->")).captured(), QLatin1String("->"));
}

// --- Comments ----------------------------------------------------------------

TEST(TokenPatternsComments, LineCommentMatches) {
  EXPECT_TRUE(matches(patterns::kLineComment, "// comment"));
  EXPECT_TRUE(matches(patterns::kLineComment, "//"));
  EXPECT_TRUE(matches(patterns::kLineComment, "// gate x q;"));
}

TEST(TokenPatternsComments, LineCommentNoMatchBlockComment) {
  EXPECT_FALSE(matches(patterns::kLineComment, "/* comment */"));
}

TEST(TokenPatternsComments, BlockCommentDelimitersMatch) {
  EXPECT_TRUE(matches(patterns::kBlockCommentStart, "/*"));
  EXPECT_TRUE(matches(patterns::kBlockCommentEnd, "*/"));
}

TEST(TokenPatternsComments, BlockCommentDelimitersNoMatchLineComment) {
  EXPECT_FALSE(matches(patterns::kBlockCommentStart, "//"));
  EXPECT_FALSE(matches(patterns::kBlockCommentEnd, "//"));
}

}  // namespace qde::gui
