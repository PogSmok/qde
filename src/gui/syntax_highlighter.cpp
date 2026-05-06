#include "qde/gui/syntax_highlighter.hpp"

#include <array>

#include <QVector>

#include "qde/gui/theme.hpp"
#include "qde/gui/token_patterns.hpp"

namespace qde::gui {

namespace {

enum class TokenType : uint8_t {
  kKeyword,
  kType,
  kGate,
  kFunction,
  kConstant,
  kNumber,
  kString,
  kVariable,
  kOperator,
  kComment,
  kCount,
};

constexpr std::size_t Idx(TokenType t) { return static_cast<std::size_t>(t); }

enum class BlockState : uint8_t { kCode = 0, kCommented = 1 };

}  // namespace

SyntaxHighlighter::SyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent) {
  std::array<QTextCharFormat, Idx(TokenType::kCount)> formats;

  auto set_color = [&](TokenType t, const char* color_hex) {
    formats[Idx(t)].setForeground(QColor(color_hex));
  };

  set_color(TokenType::kKeyword, theme::kKeywordColor);
  set_color(TokenType::kType, theme::kTypeColor);
  set_color(TokenType::kGate, theme::kGateColor);
  set_color(TokenType::kFunction, theme::kFunctionColor);
  set_color(TokenType::kConstant, theme::kConstantColor);
  set_color(TokenType::kNumber, theme::kNumberColor);
  set_color(TokenType::kString, theme::kStringColor);
  set_color(TokenType::kVariable, theme::kVariableColor);
  set_color(TokenType::kOperator, theme::kOperatorColor);
  set_color(TokenType::kComment, theme::kCommentColor);

  comment_format_ = formats[Idx(TokenType::kComment)];

  auto add_rule = [&](const char* pattern, TokenType type) {
    rules_.push_back(
        {QRegularExpression(QString::fromUtf8(pattern)), formats[Idx(type)]});
  };

  // Catch-all identifier first — specific rules below override it
  add_rule(patterns::kIdentifier, TokenType::kVariable);
  add_rule(patterns::kFunctionCall, TokenType::kFunction);
  add_rule(patterns::kUserGateCall, TokenType::kGate);

  add_rule(patterns::kTimingLiteral, TokenType::kNumber);
  add_rule(patterns::kImaginaryLiteral, TokenType::kNumber);
  add_rule(patterns::kHexIntegerLiteral, TokenType::kNumber);
  add_rule(patterns::kOctalIntegerLiteral, TokenType::kNumber);
  add_rule(patterns::kBinaryIntegerLiteral, TokenType::kNumber);
  add_rule(patterns::kFloatLiteral, TokenType::kNumber);
  add_rule(patterns::kDecimalIntegerLiteral, TokenType::kNumber);

  add_rule(patterns::kHardwareQubit, TokenType::kVariable);
  add_rule(patterns::kAnnotationKeyword, TokenType::kKeyword);
  add_rule(patterns::kPragma, TokenType::kKeyword);
  add_rule(patterns::kKeyword, TokenType::kKeyword);
  add_rule(patterns::kType, TokenType::kType);
  add_rule(patterns::kGate, TokenType::kGate);
  add_rule(patterns::kConstantLiteral, TokenType::kConstant);
  add_rule(patterns::kBitstringLiteral, TokenType::kString);
  add_rule(patterns::kStringLiteral, TokenType::kString);
  add_rule(patterns::kOperator, TokenType::kOperator);
  add_rule(patterns::kLineComment, TokenType::kComment);

  block_comment_start_ = QStringLiteral("/*");
  block_comment_end_ = QStringLiteral("*/");
}

void SyntaxHighlighter::highlightBlock(const QString& text) {
  for (const auto& [pattern, format] : rules_) {
    auto it = pattern.globalMatch(text);
    while (it.hasNext()) {
      const auto m = it.next();
      setFormat(m.capturedStart(), m.capturedLength(), format);
    }
  }

  // Multi-line BlockComment — overrides all single-line rules above
  setCurrentBlockState(static_cast<int>(BlockState::kCode));
  int start = (previousBlockState() == static_cast<int>(BlockState::kCommented))
                  ? 0
                  : text.indexOf(block_comment_start_);
  while (start >= 0) {
    const int end = text.indexOf(block_comment_end_, start);
    if (end == -1) {
      setCurrentBlockState(static_cast<int>(BlockState::kCommented));
      setFormat(start, text.length() - start, comment_format_);
      break;
    }
    const int len = end + block_comment_end_.length() - start;
    setFormat(start, len, comment_format_);
    start = text.indexOf(block_comment_start_, start + len);
  }
}

}  // namespace qde::gui
