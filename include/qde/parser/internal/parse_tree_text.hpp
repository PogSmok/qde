#ifndef PARSER_INTERNAL_PARSE_TREE_TEXT_HPP_
#define PARSER_INTERNAL_PARSE_TREE_TEXT_HPP_

#include <cstddef>
#include <string>
#include <string_view>

#include <antlr4-runtime.h>

#include "qde/parser/internal/ast/node.hpp"

namespace qde::parser::internal {

inline ast::SourceLocation Loc(antlr4::Token* token) {
  return {token->getLine(), token->getCharPositionInLine()};
}

inline ast::SourceLocation Loc(antlr4::ParserRuleContext* ctx) {
  return Loc(ctx->getStart());
}

inline ast::SourceLocation Loc(antlr4::tree::TerminalNode* node) {
  return Loc(node->getSymbol());
}

// The text of the source from the start of `ctx` up to `end`, a position in
// the input, on one line and without the semicolon that ends a statement.
inline std::string SourceText(antlr4::ParserRuleContext* ctx, std::size_t end) {
  const std::size_t start = ctx->getStart()->getStartIndex();
  if (end <= start) {
    return {};
  }
  const std::string text = ctx->getStart()->getInputStream()->getText(
      antlr4::misc::Interval(start, end - 1));
  std::string line;
  line.reserve(text.size());
  for (const char c : text) {
    const bool is_space = c == ' ' || c == '\t' || c == '\n' || c == '\r';
    if (!is_space) {
      line.push_back(c);
    } else if (!line.empty() && line.back() != ' ') {
      line.push_back(' ');
    }
  }
  while (!line.empty() && (line.back() == ' ' || line.back() == ';')) {
    line.pop_back();
  }
  return line;
}

// All of `ctx` as it is written.
inline std::string SourceText(antlr4::ParserRuleContext* ctx) {
  return SourceText(ctx, ctx->getStop()->getStopIndex() + 1);
}

// The quotes of an include path or a defcalgrammar name are not part of it.
inline std::string Unquoted(std::string_view text) {
  return std::string(text.size() >= 2 ? text.substr(1, text.size() - 2) : text);
}

}  // namespace qde::parser::internal

#endif  // PARSER_INTERNAL_PARSE_TREE_TEXT_HPP_
