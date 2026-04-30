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

constexpr std::size_t idx(TokenType t) { return static_cast<std::size_t>(t); }

enum class BlockState : uint8_t { kCode = 0, kCommented = 1 };

}  // namespace

SyntaxHighlighter::SyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent) {
  std::array<QTextCharFormat, idx(TokenType::kCount)> formats;

  auto setColor = [&](TokenType t, const char* color_hex) {
    formats[idx(t)].setForeground(QColor(color_hex));
  };

  setColor(TokenType::kKeyword, theme::kKeywordColor);
  setColor(TokenType::kType, theme::kTypeColor);
  setColor(TokenType::kGate, theme::kGateColor);
  setColor(TokenType::kFunction, theme::kFunctionColor);
  setColor(TokenType::kConstant, theme::kConstantColor);
  setColor(TokenType::kNumber, theme::kNumberColor);
  setColor(TokenType::kString, theme::kStringColor);
  setColor(TokenType::kVariable, theme::kVariableColor);
  setColor(TokenType::kOperator, theme::kOperatorColor);
  setColor(TokenType::kComment, theme::kCommentColor);

  comment_format_ = formats[idx(TokenType::kComment)];

  auto addRule = [&](const char* pattern, TokenType type) {
    rules_.push_back(
        {QRegularExpression(QString::fromUtf8(pattern)), formats[idx(type)]});
  };

  // Catch-all identifier first — specific rules below override it
  addRule(patterns::kIdentifier, TokenType::kVariable);
  addRule(patterns::kFunctionCall, TokenType::kFunction);
  addRule(patterns::kUserGateCall, TokenType::kGate);

  addRule(patterns::kTimingLiteral, TokenType::kNumber);
  addRule(patterns::kImaginaryLiteral, TokenType::kNumber);
  addRule(patterns::kHexIntegerLiteral, TokenType::kNumber);
  addRule(patterns::kOctalIntegerLiteral, TokenType::kNumber);
  addRule(patterns::kBinaryIntegerLiteral, TokenType::kNumber);
  addRule(patterns::kFloatLiteral, TokenType::kNumber);
  addRule(patterns::kDecimalIntegerLiteral, TokenType::kNumber);

  addRule(patterns::kHardwareQubit, TokenType::kVariable);
  addRule(patterns::kAnnotationKeyword, TokenType::kKeyword);
  addRule(patterns::kPragma, TokenType::kKeyword);
  addRule(patterns::kKeyword, TokenType::kKeyword);
  addRule(patterns::kType, TokenType::kType);
  addRule(patterns::kGate, TokenType::kGate);
  addRule(patterns::kConstantLiteral, TokenType::kConstant);
  addRule(patterns::kBitstringLiteral, TokenType::kString);
  addRule(patterns::kStringLiteral, TokenType::kString);
  addRule(patterns::kOperator, TokenType::kOperator);
  addRule(patterns::kLineComment, TokenType::kComment);

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
