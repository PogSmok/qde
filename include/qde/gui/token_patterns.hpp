#ifndef GUI_TOKEN_PATTERNS_HPP_
#define GUI_TOKEN_PATTERNS_HPP_

namespace qde::gui::patterns {

// ---- Numbers --------------------------------------------------------------
inline constexpr auto kTimingLiteral =
    R"(\b\d[\d_]*(?:\.\d[\d_]*)?(?:[eE][+-]?\d[\d_]*)?)"
    R"(\s*(?:dt|ns|us|µs|ms|s)\b)";
inline constexpr auto kImaginaryLiteral = R"(\b\d[\d_]*(?:\.\d[\d_]*)?\s*im\b)";
inline constexpr auto kHexIntegerLiteral =
    R"(\b0[xX][0-9a-fA-F][0-9a-fA-F_]*\b)";
inline constexpr auto kOctalIntegerLiteral = R"(\b0o[0-7][0-7_]*\b)";
inline constexpr auto kBinaryIntegerLiteral = R"(\b0[bB][01][01_]*\b)";
inline constexpr auto kFloatLiteral =
    R"(\b\d[\d_]*\.(?:\d[\d_]*)?(?:[eE][+-]?\d[\d_]*)?(?!\w))"
    R"(|\b\d[\d_]*[eE][+-]?\d[\d_]*\b)"
    R"(|\.\d[\d_]*(?:[eE][+-]?\d[\d_]*)?\b)";
inline constexpr auto kDecimalIntegerLiteral = R"(\b\d[\d_]*\b)";

// ---- Identifiers ----------------------------------------------------------
inline constexpr auto kIdentifier = R"(\b[A-Za-z_]\w*\b)";
// First identifier on a line that is not an assignment target or array access.
// Keywords/types/known gates override this, so only user-defined names remain.
inline constexpr auto kUserGateCall =
    "(?:^[ \\t]*|(?<=\\{)[ \\t]*)\\K[A-Za-z_]\\w*(?=[ \\t]+[A-Za-z_$])";
// Any identifier immediately followed by '('
inline constexpr auto kFunctionCall = R"(\b[A-Za-z_]\w*(?=\s*\())";
inline constexpr auto kHardwareQubit = R"(\$\d+)";
inline constexpr auto kAnnotationKeyword =
    R"(@[A-Za-z_]\w*(?:\.[A-Za-z_]\w*)*)";
inline constexpr auto kPragma = R"(#\s*(?:pragma|dim)\b)";

// ---- Keywords, types, builtin operations ----------------------------------
inline constexpr auto kKeyword =
    R"(\b(?:OPENQASM|include|defcalgrammar|def|cal|defcal|)"
    R"(gate|extern|box|let|break|continue|if|else|end|)"
    R"(return|for|while|in|switch|case|default|nop|pragma|)"
    R"(durationof|delay|reset|measure|barrier)\b)";
inline constexpr auto kType = R"(\b(?:input|output|const|readonly|mutable|)"
                              R"(qreg|qubit|creg|bool|bit|int|uint|float|)"
                              R"(angle|complex|array|void|duration|stretch)\b)";
// Builtin gate modifiers/operations (qasm3Lexer.g4) and all gates
// registered by GateRegistry::withBuiltins()
inline constexpr auto kGate = R"(\b(?:gphase|inv|pow|ctrl|negctrl|)"
                              R"(id|h|x|y|z|s|sdg|t|tdg|sx|sxdg|)"
                              R"(p|u1|u2|u3|U|rx|ry|rz|)"
                              R"(cx|cz|cy|swap|iswap|cp|crx|cry|crz|)"
                              R"(ccx|ccz|cswap)\b)";

// ---- Literals -------------------------------------------------------------
inline constexpr auto kConstantLiteral = R"(\b(?:true|false|pi|tau|euler)\b)";
inline constexpr auto kBitstringLiteral = R"("[01][01_]*")";
inline constexpr auto kStringLiteral = R"("[^"\r\n]*"|'[^'\r\n]*')";

// ---- Operators ------------------------------------------------------------
inline constexpr auto kOperator = R"(<<=|>>=|\*\*=|->|\+\+|\*\*|\|\||&&|==|!=)"
                                  R"(|\+=|-=|\*=|/=|&=|\|=|\^=|%=|>=|<=|>>|<<)"
                                  R"(|[\[\]{}():;.,=+\-*/%|&^@~!><])";

// ---- Comments -------------------------------------------------------------
inline constexpr auto kLineComment = R"(//[^\n]*)";
inline constexpr auto kBlockCommentStart = R"(/\*)";
inline constexpr auto kBlockCommentEnd = R"(\*/)";

}  // namespace qde::gui::patterns

#endif  // GUI_TOKEN_PATTERNS_HPP_
