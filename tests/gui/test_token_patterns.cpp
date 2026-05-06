#include <gtest/gtest.h>
#include <QRegularExpression>
#include <QString>

#include "qde/gui/token_patterns.hpp"

namespace qde::gui {

namespace {

bool Matches(const char* pattern, const char* text) {
  return QRegularExpression(QLatin1String(pattern))
      .match(QLatin1String(text))
      .hasMatch();
}

bool MatchesUtf8(const char* pattern, const char* text) {
  return QRegularExpression(QString::fromUtf8(pattern))
      .match(QString::fromUtf8(text))
      .hasMatch();
}

}  // namespace

// --- Numbers -----------------------------------------------------------------

TEST(TokenPatternsNumbers, TimingLiteralMatchesIntegerUnits) {
  EXPECT_TRUE(Matches(patterns::kTimingLiteral, "1dt"));
  EXPECT_TRUE(Matches(patterns::kTimingLiteral, "100ns"));
  EXPECT_TRUE(Matches(patterns::kTimingLiteral, "50us"));
  EXPECT_TRUE(Matches(patterns::kTimingLiteral, "20ms"));
  EXPECT_TRUE(Matches(patterns::kTimingLiteral, "2s"));
}

TEST(TokenPatternsNumbers, TimingLiteralMatchesFloatUnits) {
  EXPECT_TRUE(Matches(patterns::kTimingLiteral, "1.5ns"));
  EXPECT_TRUE(Matches(patterns::kTimingLiteral, "3.14ms"));
  EXPECT_TRUE(MatchesUtf8(patterns::kTimingLiteral, "0.5µs"));
}

TEST(TokenPatternsNumbers, TimingLiteralNoMatchWithoutUnit) {
  EXPECT_FALSE(Matches(patterns::kTimingLiteral, "100"));
  EXPECT_FALSE(Matches(patterns::kTimingLiteral, "1.5"));
  EXPECT_FALSE(Matches(patterns::kTimingLiteral, "ns"));
}

TEST(TokenPatternsNumbers, ImaginaryLiteralMatches) {
  EXPECT_TRUE(Matches(patterns::kImaginaryLiteral, "1im"));
  EXPECT_TRUE(Matches(patterns::kImaginaryLiteral, "1.5im"));
  EXPECT_TRUE(Matches(patterns::kImaginaryLiteral, "3.14im"));
}

TEST(TokenPatternsNumbers, ImaginaryLiteralNoMatchWithoutPrefix) {
  EXPECT_FALSE(Matches(patterns::kImaginaryLiteral, "im"));
  EXPECT_FALSE(Matches(patterns::kImaginaryLiteral, "1"));
}

TEST(TokenPatternsNumbers, HexIntegerMatches) {
  EXPECT_TRUE(Matches(patterns::kHexIntegerLiteral, "0xFF"));
  EXPECT_TRUE(Matches(patterns::kHexIntegerLiteral, "0xABCD"));
  EXPECT_TRUE(Matches(patterns::kHexIntegerLiteral, "0X1a"));
  EXPECT_TRUE(Matches(patterns::kHexIntegerLiteral, "0xff_00"));
}

TEST(TokenPatternsNumbers, HexIntegerNoMatchForOtherBases) {
  EXPECT_FALSE(Matches(patterns::kHexIntegerLiteral, "0b101"));
  EXPECT_FALSE(Matches(patterns::kHexIntegerLiteral, "0o7"));
  EXPECT_FALSE(Matches(patterns::kHexIntegerLiteral, "123"));
}

TEST(TokenPatternsNumbers, OctalIntegerMatches) {
  EXPECT_TRUE(Matches(patterns::kOctalIntegerLiteral, "0o7"));
  EXPECT_TRUE(Matches(patterns::kOctalIntegerLiteral, "0o755"));
  EXPECT_TRUE(Matches(patterns::kOctalIntegerLiteral, "0o0"));
}

TEST(TokenPatternsNumbers, OctalIntegerNoMatchForOtherBases) {
  EXPECT_FALSE(Matches(patterns::kOctalIntegerLiteral, "0xFF"));
  EXPECT_FALSE(Matches(patterns::kOctalIntegerLiteral, "0b101"));
  EXPECT_FALSE(Matches(patterns::kOctalIntegerLiteral, "755"));
}

TEST(TokenPatternsNumbers, BinaryIntegerMatches) {
  EXPECT_TRUE(Matches(patterns::kBinaryIntegerLiteral, "0b101"));
  EXPECT_TRUE(Matches(patterns::kBinaryIntegerLiteral, "0B1010"));
  EXPECT_TRUE(Matches(patterns::kBinaryIntegerLiteral, "0b0"));
  EXPECT_TRUE(Matches(patterns::kBinaryIntegerLiteral, "0b1_0_1"));
}

TEST(TokenPatternsNumbers, BinaryIntegerNoMatchForOtherBases) {
  EXPECT_FALSE(Matches(patterns::kBinaryIntegerLiteral, "0xFF"));
  EXPECT_FALSE(Matches(patterns::kBinaryIntegerLiteral, "0o7"));
  EXPECT_FALSE(Matches(patterns::kBinaryIntegerLiteral, "101"));
}

TEST(TokenPatternsNumbers, FloatLiteralMatchesDotForms) {
  EXPECT_TRUE(Matches(patterns::kFloatLiteral, "1.0"));
  EXPECT_TRUE(Matches(patterns::kFloatLiteral, "1."));
  EXPECT_TRUE(Matches(patterns::kFloatLiteral, ".5"));
  EXPECT_TRUE(Matches(patterns::kFloatLiteral, "1.5e3"));
  EXPECT_TRUE(Matches(patterns::kFloatLiteral, "1.5e-3"));
  EXPECT_TRUE(Matches(patterns::kFloatLiteral, "1.5e+3"));
}

TEST(TokenPatternsNumbers, FloatLiteralMatchesExponentForm) {
  EXPECT_TRUE(Matches(patterns::kFloatLiteral, "1e3"));
  EXPECT_TRUE(Matches(patterns::kFloatLiteral, "1e-10"));
}

TEST(TokenPatternsNumbers, FloatLiteralNoMatchForInteger) {
  EXPECT_FALSE(Matches(patterns::kFloatLiteral, "42"));
  EXPECT_FALSE(Matches(patterns::kFloatLiteral, "0xFF"));
}

TEST(TokenPatternsNumbers, DecimalIntegerMatches) {
  EXPECT_TRUE(Matches(patterns::kDecimalIntegerLiteral, "0"));
  EXPECT_TRUE(Matches(patterns::kDecimalIntegerLiteral, "123"));
  EXPECT_TRUE(Matches(patterns::kDecimalIntegerLiteral, "1_000"));
}

// --- Identifiers -------------------------------------------------------------

TEST(TokenPatternsIdentifiers, HardwareQubitMatches) {
  EXPECT_TRUE(Matches(patterns::kHardwareQubit, "$0"));
  EXPECT_TRUE(Matches(patterns::kHardwareQubit, "$123"));
}

TEST(TokenPatternsIdentifiers, HardwareQubitNoMatchWithoutDigits) {
  EXPECT_FALSE(Matches(patterns::kHardwareQubit, "$"));
  EXPECT_FALSE(Matches(patterns::kHardwareQubit, "$abc"));
}

TEST(TokenPatternsIdentifiers, AnnotationKeywordMatches) {
  EXPECT_TRUE(Matches(patterns::kAnnotationKeyword, "@foo"));
  EXPECT_TRUE(Matches(patterns::kAnnotationKeyword, "@foo.bar"));
  EXPECT_TRUE(Matches(patterns::kAnnotationKeyword, "@foo.bar.baz"));
  EXPECT_TRUE(Matches(patterns::kAnnotationKeyword, "@_private"));
}

TEST(TokenPatternsIdentifiers, AnnotationKeywordNoMatchBareAt) {
  EXPECT_FALSE(Matches(patterns::kAnnotationKeyword, "@"));
  EXPECT_FALSE(Matches(patterns::kAnnotationKeyword, "@123"));
}

TEST(TokenPatternsIdentifiers, PragmaMatches) {
  EXPECT_TRUE(Matches(patterns::kPragma, "#pragma"));
  EXPECT_TRUE(Matches(patterns::kPragma, "#dim"));
  EXPECT_TRUE(Matches(patterns::kPragma, "# pragma"));
}

TEST(TokenPatternsIdentifiers, PragmaNoMatchOtherHash) {
  EXPECT_FALSE(Matches(patterns::kPragma, "pragma"));
  EXPECT_FALSE(Matches(patterns::kPragma, "#include"));
  EXPECT_FALSE(Matches(patterns::kPragma, "#foo"));
}

// --- Keywords, types, gates --------------------------------------------------

TEST(TokenPatternsKeywords, KeywordMatches) {
  EXPECT_TRUE(Matches(patterns::kKeyword, "gate"));
  EXPECT_TRUE(Matches(patterns::kKeyword, "if"));
  EXPECT_TRUE(Matches(patterns::kKeyword, "for"));
  EXPECT_TRUE(Matches(patterns::kKeyword, "OPENQASM"));
  EXPECT_TRUE(Matches(patterns::kKeyword, "measure"));
  EXPECT_TRUE(Matches(patterns::kKeyword, "reset"));
  EXPECT_TRUE(Matches(patterns::kKeyword, "barrier"));
}

TEST(TokenPatternsKeywords, KeywordRespectWordBoundary) {
  EXPECT_FALSE(Matches(patterns::kKeyword, "gateway"));
  EXPECT_FALSE(Matches(patterns::kKeyword, "iffy"));
  EXPECT_FALSE(Matches(patterns::kKeyword, "forloop"));
  EXPECT_FALSE(Matches(patterns::kKeyword, "defcalx"));
}

TEST(TokenPatternsKeywords, TypeMatches) {
  EXPECT_TRUE(Matches(patterns::kType, "qubit"));
  EXPECT_TRUE(Matches(patterns::kType, "int"));
  EXPECT_TRUE(Matches(patterns::kType, "float"));
  EXPECT_TRUE(Matches(patterns::kType, "bool"));
  EXPECT_TRUE(Matches(patterns::kType, "angle"));
  EXPECT_TRUE(Matches(patterns::kType, "duration"));
}

TEST(TokenPatternsKeywords, TypeRespectWordBoundary) {
  EXPECT_FALSE(Matches(patterns::kType, "qubitregister"));
  EXPECT_FALSE(Matches(patterns::kType, "integer"));
  EXPECT_FALSE(Matches(patterns::kType, "floating"));
  EXPECT_FALSE(Matches(patterns::kType, "boolean"));
}

TEST(TokenPatternsKeywords, GateMatchesBuiltinModifiers) {
  EXPECT_TRUE(Matches(patterns::kGate, "gphase"));
  EXPECT_TRUE(Matches(patterns::kGate, "inv"));
  EXPECT_TRUE(Matches(patterns::kGate, "ctrl"));
  EXPECT_TRUE(Matches(patterns::kGate, "negctrl"));
}

TEST(TokenPatternsKeywords, GateMatchesStdlibSingleQubit) {
  EXPECT_TRUE(Matches(patterns::kGate, "h"));
  EXPECT_TRUE(Matches(patterns::kGate, "x"));
  EXPECT_TRUE(Matches(patterns::kGate, "rx"));
  EXPECT_TRUE(Matches(patterns::kGate, "sdg"));
  EXPECT_TRUE(Matches(patterns::kGate, "sx"));
  EXPECT_TRUE(Matches(patterns::kGate, "sxdg"));
}

TEST(TokenPatternsKeywords, GateMatchesStdlibMultiQubit) {
  EXPECT_TRUE(Matches(patterns::kGate, "cx"));
  EXPECT_TRUE(Matches(patterns::kGate, "swap"));
  EXPECT_TRUE(Matches(patterns::kGate, "iswap"));
  EXPECT_TRUE(Matches(patterns::kGate, "ccx"));
  EXPECT_TRUE(Matches(patterns::kGate, "cswap"));
}

TEST(TokenPatternsKeywords, GateRespectWordBoundary) {
  EXPECT_FALSE(Matches(patterns::kGate, "hx"));
  EXPECT_FALSE(Matches(patterns::kGate, "cxx"));
  EXPECT_FALSE(Matches(patterns::kGate, "swapper"));
  EXPECT_FALSE(Matches(patterns::kGate, "rxgate"));
}

// --- Literals ----------------------------------------------------------------

TEST(TokenPatternsLiterals, ConstantLiteralMatchesBooleans) {
  EXPECT_TRUE(Matches(patterns::kConstantLiteral, "true"));
  EXPECT_TRUE(Matches(patterns::kConstantLiteral, "false"));
}

TEST(TokenPatternsLiterals, ConstantLiteralMatchesMathConstants) {
  EXPECT_TRUE(Matches(patterns::kConstantLiteral, "pi"));
  EXPECT_TRUE(Matches(patterns::kConstantLiteral, "tau"));
  EXPECT_TRUE(Matches(patterns::kConstantLiteral, "euler"));
}

TEST(TokenPatternsLiterals, ConstantLiteralRespectWordBoundary) {
  EXPECT_FALSE(Matches(patterns::kConstantLiteral, "truefalse"));
  EXPECT_FALSE(Matches(patterns::kConstantLiteral, "True"));
  EXPECT_FALSE(Matches(patterns::kConstantLiteral, "pipeline"));
  EXPECT_FALSE(Matches(patterns::kConstantLiteral, "eulerian"));
}

TEST(TokenPatternsLiterals, BitstringLiteralMatches) {
  EXPECT_TRUE(Matches(patterns::kBitstringLiteral, "\"0\""));
  EXPECT_TRUE(Matches(patterns::kBitstringLiteral, "\"0101\""));
  EXPECT_TRUE(Matches(patterns::kBitstringLiteral, "\"1_0_1\""));
}

TEST(TokenPatternsLiterals, BitstringLiteralNoMatchNonBinary) {
  EXPECT_FALSE(Matches(patterns::kBitstringLiteral, "\"012\""));
  EXPECT_FALSE(Matches(patterns::kBitstringLiteral, "\"abc\""));
}

TEST(TokenPatternsLiterals, StringLiteralMatchesDoubleQuoted) {
  EXPECT_TRUE(Matches(patterns::kStringLiteral, "\"hello\""));
  EXPECT_TRUE(Matches(patterns::kStringLiteral, "\"path/to/file.inc\""));
}

TEST(TokenPatternsLiterals, StringLiteralMatchesSingleQuoted) {
  EXPECT_TRUE(Matches(patterns::kStringLiteral, "'hello'"));
  EXPECT_TRUE(Matches(patterns::kStringLiteral, "'path/to/file.inc'"));
}

// --- User gate call ----------------------------------------------------------

TEST(TokenPatternsIdentifiers, UserGateCallMatchesLineStart) {
  EXPECT_TRUE(Matches(patterns::kUserGateCall, "h q;"));
  EXPECT_TRUE(Matches(patterns::kUserGateCall, "cx q1, q2;"));
  EXPECT_TRUE(Matches(patterns::kUserGateCall, "myGate q;"));
}

TEST(TokenPatternsIdentifiers, UserGateCallMatchesIndented) {
  EXPECT_TRUE(Matches(patterns::kUserGateCall, "  myGate q;"));
  EXPECT_TRUE(Matches(patterns::kUserGateCall, "\th q;"));
}

TEST(TokenPatternsIdentifiers, UserGateCallMatchesAfterBrace) {
  EXPECT_TRUE(Matches(patterns::kUserGateCall, "{ majority q;"));
  EXPECT_TRUE(Matches(patterns::kUserGateCall, "{h q;"));
}

TEST(TokenPatternsIdentifiers, UserGateCallMatchesHardwareQubit) {
  EXPECT_TRUE(Matches(patterns::kUserGateCall, "h $0;"));
}

TEST(TokenPatternsIdentifiers, UserGateCallExcludesAssignment) {
  EXPECT_FALSE(Matches(patterns::kUserGateCall, "theta = pi;"));
  EXPECT_FALSE(Matches(patterns::kUserGateCall, "theta += pi;"));
  EXPECT_FALSE(Matches(patterns::kUserGateCall, "theta *= 2;"));
}

TEST(TokenPatternsIdentifiers, UserGateCallExcludesFunctionCall) {
  EXPECT_FALSE(Matches(patterns::kUserGateCall, "myFunc(theta);"));
}

TEST(TokenPatternsIdentifiers, UserGateCallExcludesArrayAccess) {
  EXPECT_FALSE(Matches(patterns::kUserGateCall, "c[0] = measure q;"));
}

// --- Function call -----------------------------------------------------------

TEST(TokenPatternsIdentifiers, FunctionCallMatches) {
  EXPECT_TRUE(Matches(patterns::kFunctionCall, "myFunc(x)"));
  EXPECT_TRUE(Matches(patterns::kFunctionCall, "sin(theta)"));
  EXPECT_TRUE(Matches(patterns::kFunctionCall, "rx(pi/2) q;"));
}

TEST(TokenPatternsIdentifiers, FunctionCallMatchesWithSpace) {
  EXPECT_TRUE(Matches(patterns::kFunctionCall, "sin (x)"));
}

TEST(TokenPatternsIdentifiers, FunctionCallMatchesInsideExpression) {
  EXPECT_TRUE(Matches(patterns::kFunctionCall, "result = myFunc(x);"));
}

TEST(TokenPatternsIdentifiers, FunctionCallNoMatchWithoutParen) {
  EXPECT_FALSE(Matches(patterns::kFunctionCall, "theta"));
  EXPECT_FALSE(Matches(patterns::kFunctionCall, "theta = x;"));
}

// --- Catch-all identifier ----------------------------------------------------

TEST(TokenPatternsIdentifiers, IdentifierMatches) {
  EXPECT_TRUE(Matches(patterns::kIdentifier, "myQubit"));
  EXPECT_TRUE(Matches(patterns::kIdentifier, "theta"));
  EXPECT_TRUE(Matches(patterns::kIdentifier, "_private"));
  EXPECT_TRUE(Matches(patterns::kIdentifier, "q0"));
}

TEST(TokenPatternsIdentifiers, IdentifierRespectWordBoundary) {
  // digits-only and bare operators are not identifiers
  EXPECT_FALSE(Matches(patterns::kIdentifier, "123"));
  EXPECT_FALSE(Matches(patterns::kIdentifier, "+"));
  EXPECT_FALSE(Matches(patterns::kIdentifier, "$0"));
}

// --- Operators ---------------------------------------------------------------

TEST(TokenPatternsOperators, SingleCharOperatorsMatch) {
  EXPECT_TRUE(Matches(patterns::kOperator, "+"));
  EXPECT_TRUE(Matches(patterns::kOperator, "-"));
  EXPECT_TRUE(Matches(patterns::kOperator, "*"));
  EXPECT_TRUE(Matches(patterns::kOperator, "/"));
  EXPECT_TRUE(Matches(patterns::kOperator, "="));
  EXPECT_TRUE(Matches(patterns::kOperator, "<"));
  EXPECT_TRUE(Matches(patterns::kOperator, ">"));
  EXPECT_TRUE(Matches(patterns::kOperator, "!"));
  EXPECT_TRUE(Matches(patterns::kOperator, "&"));
  EXPECT_TRUE(Matches(patterns::kOperator, "|"));
  EXPECT_TRUE(Matches(patterns::kOperator, "^"));
  EXPECT_TRUE(Matches(patterns::kOperator, "~"));
  EXPECT_TRUE(Matches(patterns::kOperator, "%"));
}

TEST(TokenPatternsOperators, BracketOperatorsMatch) {
  EXPECT_TRUE(Matches(patterns::kOperator, "["));
  EXPECT_TRUE(Matches(patterns::kOperator, "]"));
  EXPECT_TRUE(Matches(patterns::kOperator, "{"));
  EXPECT_TRUE(Matches(patterns::kOperator, "}"));
  EXPECT_TRUE(Matches(patterns::kOperator, "("));
  EXPECT_TRUE(Matches(patterns::kOperator, ")"));
}

TEST(TokenPatternsOperators, PunctuationMatch) {
  EXPECT_TRUE(Matches(patterns::kOperator, ";"));
  EXPECT_TRUE(Matches(patterns::kOperator, ":"));
  EXPECT_TRUE(Matches(patterns::kOperator, ","));
  EXPECT_TRUE(Matches(patterns::kOperator, "."));
}

TEST(TokenPatternsOperators, MultiCharOperatorsMatch) {
  EXPECT_TRUE(Matches(patterns::kOperator, "->"));
  EXPECT_TRUE(Matches(patterns::kOperator, "++"));
  EXPECT_TRUE(Matches(patterns::kOperator, "**"));
  EXPECT_TRUE(Matches(patterns::kOperator, "||"));
  EXPECT_TRUE(Matches(patterns::kOperator, "&&"));
  EXPECT_TRUE(Matches(patterns::kOperator, "=="));
  EXPECT_TRUE(Matches(patterns::kOperator, "!="));
  EXPECT_TRUE(Matches(patterns::kOperator, ">="));
  EXPECT_TRUE(Matches(patterns::kOperator, "<="));
  EXPECT_TRUE(Matches(patterns::kOperator, ">>"));
  EXPECT_TRUE(Matches(patterns::kOperator, "<<"));
}

TEST(TokenPatternsOperators, CompoundAssignmentOperatorsMatch) {
  EXPECT_TRUE(Matches(patterns::kOperator, "+="));
  EXPECT_TRUE(Matches(patterns::kOperator, "-="));
  EXPECT_TRUE(Matches(patterns::kOperator, "*="));
  EXPECT_TRUE(Matches(patterns::kOperator, "/="));
  EXPECT_TRUE(Matches(patterns::kOperator, "&="));
  EXPECT_TRUE(Matches(patterns::kOperator, "|="));
  EXPECT_TRUE(Matches(patterns::kOperator, "^="));
  EXPECT_TRUE(Matches(patterns::kOperator, "%="));
  EXPECT_TRUE(Matches(patterns::kOperator, "<<="));
  EXPECT_TRUE(Matches(patterns::kOperator, ">>="));
  EXPECT_TRUE(Matches(patterns::kOperator, "**="));
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
  EXPECT_TRUE(Matches(patterns::kLineComment, "// comment"));
  EXPECT_TRUE(Matches(patterns::kLineComment, "//"));
  EXPECT_TRUE(Matches(patterns::kLineComment, "// gate x q;"));
}

TEST(TokenPatternsComments, LineCommentNoMatchBlockComment) {
  EXPECT_FALSE(Matches(patterns::kLineComment, "/* comment */"));
}

TEST(TokenPatternsComments, BlockCommentDelimitersMatch) {
  EXPECT_TRUE(Matches(patterns::kBlockCommentStart, "/*"));
  EXPECT_TRUE(Matches(patterns::kBlockCommentEnd, "*/"));
}

TEST(TokenPatternsComments, BlockCommentDelimitersNoMatchLineComment) {
  EXPECT_FALSE(Matches(patterns::kBlockCommentStart, "//"));
  EXPECT_FALSE(Matches(patterns::kBlockCommentEnd, "//"));
}

}  // namespace qde::gui
