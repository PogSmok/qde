// Literals and types.

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <locale>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <antlr4-runtime.h>
#include <qasm3Parser.h>

#include "qde/parser/internal/ast.hpp"
#include "qde/parser/internal/ast_builder.hpp"
#include "qde/parser/internal/parse_tree_text.hpp"

namespace qde::parser::internal {

namespace {

// Digit separators are legal in every numeric literal.
std::string WithoutUnderscores(const std::string& text) {
  std::string digits;
  digits.reserve(text.size());
  for (const char c : text) {
    if (c != '_') {
      digits.push_back(c);
    }
  }
  return digits;
}

// Parses a float independently of the global locale.
// Returns nullopt if it is too large for a double.
std::optional<double> ParseReal(const std::string& text) {
  std::istringstream stream(WithoutUnderscores(text));
  stream.imbue(std::locale::classic());
  double value = 0.0;
  stream >> value;
  // The stream fails for a number out of range.
  // One too small to tell from zero is rounded to it, which is not an error.
  if (stream.fail() && std::abs(value) >= std::numeric_limits<double>::max()) {
    return std::nullopt;
  }
  return value;
}

std::string Trimmed(std::string text) {
  while (!text.empty() && (text.back() == ' ' || text.back() == '\t')) {
    text.pop_back();
  }
  return text;
}

bool EndsWith(const std::string& text, const std::string& suffix) {
  return text.size() >= suffix.size() &&
         text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

}  // namespace

// ---- literals ---------------------------------------------------------------

// The magnitude of a float, imaginary or timing literal, given as the text
// of `node` without its suffix.
double AstBuilder::Real(antlr4::tree::TerminalNode* node,
                        const std::string& text) {
  const auto value = ParseReal(text);
  if (!value) {
    diagnostics_.Error(Loc(node), "literal '" + node->getText() +
                                      "' is too large for a float");
  }
  return value.value_or(0.0);
}

std::uint64_t AstBuilder::Integer(antlr4::tree::TerminalNode* node,
                                  std::size_t prefix, unsigned base) {
  const std::string digits = WithoutUnderscores(node->getText()).substr(prefix);
  const std::uint64_t max = std::numeric_limits<std::uint64_t>::max();
  std::uint64_t value = 0;
  for (const char c : digits) {
    unsigned digit = 0;
    if (c >= '0' && c <= '9') {
      digit = static_cast<unsigned>(c - '0');
    } else if (c >= 'a' && c <= 'f') {
      digit = static_cast<unsigned>(c - 'a') + 10;
    } else {
      digit = static_cast<unsigned>(c - 'A') + 10;
    }
    if (value > (max - digit) / base) {
      diagnostics_.Error(Loc(node), "integer literal '" + node->getText() +
                                        "' does not fit in 64 bits");
      return 0;
    }
    value = (value * base) + digit;
  }
  return value;
}

ast::ExprPtr AstBuilder::Literal(qasm3Parser::LiteralExpressionContext* ctx) {
  if (auto* node = ctx->Identifier()) {
    auto expr = Make<ast::Identifier>(Loc(ctx));
    expr->name = node->getText();
    return expr;
  }
  if (auto* node = ctx->DecimalIntegerLiteral()) {
    auto expr = Make<ast::IntegerLiteral>(Loc(ctx));
    expr->value = Integer(node, 0, 10);
    return expr;
  }
  if (auto* node = ctx->HexIntegerLiteral()) {
    auto expr = Make<ast::IntegerLiteral>(Loc(ctx));
    expr->value = Integer(node, 2, 16);
    return expr;
  }
  if (auto* node = ctx->OctalIntegerLiteral()) {
    auto expr = Make<ast::IntegerLiteral>(Loc(ctx));
    expr->value = Integer(node, 2, 8);
    return expr;
  }
  if (auto* node = ctx->BinaryIntegerLiteral()) {
    auto expr = Make<ast::IntegerLiteral>(Loc(ctx));
    expr->value = Integer(node, 2, 2);
    return expr;
  }
  if (auto* node = ctx->FloatLiteral()) {
    auto expr = Make<ast::FloatLiteral>(Loc(ctx));
    expr->value = Real(node, node->getText());
    return expr;
  }
  if (auto* node = ctx->ImaginaryLiteral()) {
    auto expr = Make<ast::ImaginaryLiteral>(Loc(ctx));
    std::string text = node->getText();
    text.resize(text.size() - 2);  // the "im" suffix
    expr->value = Real(node, Trimmed(text));
    return expr;
  }
  if (auto* node = ctx->BooleanLiteral()) {
    auto expr = Make<ast::BooleanLiteral>(Loc(ctx));
    expr->value = node->getText() == "true";
    return expr;
  }
  if (auto* node = ctx->BitstringLiteral()) {
    auto expr = Make<ast::BitstringLiteral>(Loc(ctx));
    expr->digits = WithoutUnderscores(Unquoted(node->getText()));
    return expr;
  }
  if (auto* node = ctx->TimingLiteral()) {
    auto expr = Make<ast::TimingLiteral>(Loc(ctx));
    std::string text = node->getText();
    // "s" is a suffix of every other unit and has to be tried last.
    const std::array<std::pair<const char*, ast::TimeUnit>, 6> units = {{
        {"dt", ast::TimeUnit::kDeviceCycle},
        {"ns", ast::TimeUnit::kNanosecond},
        {"us", ast::TimeUnit::kMicrosecond},
        {"\xC2\xB5s", ast::TimeUnit::kMicrosecond},  // µs
        {"ms", ast::TimeUnit::kMillisecond},
        {"s", ast::TimeUnit::kSecond},
    }};
    for (const auto& [suffix, unit] : units) {
      if (EndsWith(text, suffix)) {
        expr->unit = unit;
        text.resize(text.size() - std::string(suffix).size());
        break;
      }
    }
    expr->value = Real(node, Trimmed(text));
    return expr;
  }
  if (auto* node = ctx->HardwareQubit()) {
    return HardwareQubit(node);
  }
  return Make<ast::ErrorExpr>(ReportUnsupported(ctx, "literal"));
}

ast::ExprPtr AstBuilder::HardwareQubit(antlr4::tree::TerminalNode* node) {
  auto expr = Make<ast::HardwareQubit>(Loc(node));
  expr->number = Integer(node, 1, 10);  // skips the '$'
  return expr;
}

// ---- types -----------------------------------------------------------------

std::shared_ptr<const ast::ScalarTypeSpec> AstBuilder::ScalarType(
    qasm3Parser::ScalarTypeContext* ctx) {
  auto type = Make<ast::ScalarTypeSpec>(Loc(ctx));
  type->width = Designator(ctx->designator());
  if (ctx->BIT() != nullptr) {
    type->scalar = ast::ScalarKind::kBit;
  } else if (ctx->INT() != nullptr) {
    type->scalar = ast::ScalarKind::kInt;
  } else if (ctx->UINT() != nullptr) {
    type->scalar = ast::ScalarKind::kUint;
  } else if (ctx->FLOAT() != nullptr) {
    type->scalar = ast::ScalarKind::kFloat;
  } else if (ctx->ANGLE() != nullptr) {
    type->scalar = ast::ScalarKind::kAngle;
  } else if (ctx->BOOL() != nullptr) {
    type->scalar = ast::ScalarKind::kBool;
  } else if (ctx->DURATION() != nullptr) {
    type->scalar = ast::ScalarKind::kDuration;
  } else if (ctx->STRETCH() != nullptr) {
    type->scalar = ast::ScalarKind::kStretch;
  } else {
    type->scalar = ast::ScalarKind::kComplex;
    if (auto* component = ctx->scalarType()) {
      type->component = ScalarType(component);
    }
  }
  return type;
}

ast::TypeSpecPtr AstBuilder::ArrayType(qasm3Parser::ArrayTypeContext* ctx) {
  auto type = Make<ast::ArrayTypeSpec>(Loc(ctx));
  type->element = ScalarType(ctx->scalarType());
  type->dimensions = Expressions(ctx->expressionList());
  return type;
}

ast::TypeSpecPtr AstBuilder::ArrayReferenceType(
    qasm3Parser::ArrayReferenceTypeContext* ctx) {
  auto type = Make<ast::ArrayReferenceTypeSpec>(Loc(ctx));
  type->is_mutable = ctx->MUTABLE() != nullptr;
  type->element = ScalarType(ctx->scalarType());
  if (ctx->DIM() != nullptr) {
    type->dimension_count = Expression(ctx->expression());
  } else {
    type->dimensions = Expressions(ctx->expressionList());
  }
  return type;
}

std::shared_ptr<const ast::QubitTypeSpec> AstBuilder::QubitType(
    ast::SourceLocation loc, qasm3Parser::DesignatorContext* designator) {
  auto type = Make<ast::QubitTypeSpec>(loc);
  type->width = Designator(designator);
  return type;
}

ast::TypeSpecPtr AstBuilder::BitType(
    ast::SourceLocation loc, qasm3Parser::DesignatorContext* designator) {
  auto type = Make<ast::ScalarTypeSpec>(loc);
  type->scalar = ast::ScalarKind::kBit;
  type->width = Designator(designator);
  return type;
}

}  // namespace qde::parser::internal
