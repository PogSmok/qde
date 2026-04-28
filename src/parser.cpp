// ---------------------------------------------------------------------------
// Unimplemented / incomplete features
//
// Statement visitors (stubs — visitChildren only, no semantic action):
//   aliasDeclarationStatement  (let alias = expr)
//   boxStatement               (box { ... })
//   breakStatement             (break)
//   calStatement               (cal { ... })
//   calibrationGrammarStatement(defcalgrammar "openpulse")
//   continueStatement          (continue)
//   defcalStatement            (defcal pulse-level gate)
//   defStatement               (subroutine definition)
//   delayStatement             (delay[t] q)
//   endStatement               (end)
//   expressionStatement        (standalone expression)
//   externStatement            (extern fn declaration)
//   forStatement               (for loop — body IS visited once)
//   ifStatement                (conditional — body IS visited)
//   includeStatement           (include "file")
//   ioDeclarationStatement     (input/output port)
//   nopStatement               (nop)
//   returnStatement            (return)
//   switchStatement            (switch/case)
//   whileStatement             (while — body IS visited once)
//
// Classical type system:
//   STRETCH: stored as 0.0; has no meaningful numeric value by spec.
//   Complex compound assignment (e.g. z += 1.0im): compound operators
//     work on the real component only; imaginary part is cleared.
//   Subroutine tracking: subroutine_map_ is never populated because
//     visitDefStatement is a stub.
//
// Expression evaluation:
//   DurationofExpressionContext: falls back to 0.0 (TODO in code).
//   User-defined function calls (CallExpressionContext for non-builtin
//     names): emits a parse-time error instead of deferring.
//   Gate-parameter reference as an array index (a[theta]): evaluates to
//     0.0 at parse time; unsupported by design since gate params are
//     rotation angles, not integer indices.
//
// Gate call semantics:
//   quantumCallExpression form of measureArrowAssignmentStatement is not
//     handled (returns early).
//   Hardware-qubit operands ($N) emit an error via resolveQubit.
//   Pairwise broadcast falls through to per-operand resolution when
//     register sizes differ; no explicit error is reported.
// ---------------------------------------------------------------------------

#include "qde/parser.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>

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

using C = std::complex<double>;
using Mat = std::vector<C>;

constexpr double kPi = 3.141592653589793238462643383;
constexpr double kTau = 6.283185307179586476925286766;
constexpr double kEuler = 2.718281828459045235360287471;

// ---------------------------------------------------------------------------
// ExprNode
//
// Compile-time expression tree built from ANTLR expression contexts.
// Stored independently of ANTLR so it can be evaluated inside gate
// definition lambdas after parsing ends.
// ---------------------------------------------------------------------------

class ExprNode {
 public:
  enum class ExprKind : std::uint8_t {
    kLiteral,
    kParamRef,
    // Arithmetic
    kNeg,
    kAdd,
    kSub,
    kMul,
    kDiv,
    kMod,
    kPow,
    // Unary
    kBitNot,
    kLogNot,
    // Bitwise
    kBitAnd,
    kBitOr,
    kBitXor,
    kLShift,
    kRShift,
    // Comparison (return 1.0 or 0.0)
    kLt,
    kGt,
    kLe,
    kGe,
    kEq,
    kNe,
    // Logical (return 1.0 or 0.0)
    kLogAnd,
    kLogOr,
    // Math functions
    kSin,
    kCos,
    kTan,
    kExp,
    kSqrt,
    kLn,
    kArcsin,
    kArccos,
    kArctan,
    kFloor,
  };

  ExprKind kind = ExprKind::kLiteral;
  double value = 0.0;  // Literal
  int param_idx = 0;   // ParamRef
  std::shared_ptr<ExprNode> lhs, rhs;

  static double eval(const std::shared_ptr<ExprNode>& n,
                     const std::vector<double>& params) {
    switch (n->kind) {
      case ExprKind::kLiteral:
        return n->value;
      case ExprKind::kParamRef:
        return params[n->param_idx];
      case ExprKind::kNeg:
        return -eval(n->lhs, params);
      case ExprKind::kAdd:
        return eval(n->lhs, params) + eval(n->rhs, params);
      case ExprKind::kSub:
        return eval(n->lhs, params) - eval(n->rhs, params);
      case ExprKind::kMul:
        return eval(n->lhs, params) * eval(n->rhs, params);
      case ExprKind::kDiv: {
        double r = eval(n->rhs, params);
        return r != 0.0 ? eval(n->lhs, params) / r : 0.0;
      }
      case ExprKind::kMod:
        return std::fmod(eval(n->lhs, params), eval(n->rhs, params));
      case ExprKind::kPow:
        return std::pow(eval(n->lhs, params), eval(n->rhs, params));
      case ExprKind::kBitNot:
        return static_cast<double>(
            ~static_cast<long long>(eval(n->lhs, params)));
      case ExprKind::kLogNot:
        return eval(n->lhs, params) != 0.0 ? 0.0 : 1.0;
      case ExprKind::kBitAnd: {
        auto l = static_cast<long long>(eval(n->lhs, params));
        auto r = static_cast<long long>(eval(n->rhs, params));
        return static_cast<double>(l & r);
      }
      case ExprKind::kBitOr: {
        auto l = static_cast<long long>(eval(n->lhs, params));
        auto r = static_cast<long long>(eval(n->rhs, params));
        return static_cast<double>(l | r);
      }
      case ExprKind::kBitXor: {
        auto l = static_cast<long long>(eval(n->lhs, params));
        auto r = static_cast<long long>(eval(n->rhs, params));
        return static_cast<double>(l ^ r);
      }
      case ExprKind::kLShift: {
        auto l = static_cast<long long>(eval(n->lhs, params));
        auto r = static_cast<long long>(eval(n->rhs, params));
        return static_cast<double>(l << r);
      }
      case ExprKind::kRShift: {
        auto l = static_cast<long long>(eval(n->lhs, params));
        auto r = static_cast<long long>(eval(n->rhs, params));
        return static_cast<double>(l >> r);
      }
      case ExprKind::kLt:
        return eval(n->lhs, params) < eval(n->rhs, params) ? 1.0 : 0.0;
      case ExprKind::kGt:
        return eval(n->lhs, params) > eval(n->rhs, params) ? 1.0 : 0.0;
      case ExprKind::kLe:
        return eval(n->lhs, params) <= eval(n->rhs, params) ? 1.0 : 0.0;
      case ExprKind::kGe:
        return eval(n->lhs, params) >= eval(n->rhs, params) ? 1.0 : 0.0;
      case ExprKind::kEq:
        return eval(n->lhs, params) == eval(n->rhs, params) ? 1.0 : 0.0;
      case ExprKind::kNe:
        return eval(n->lhs, params) != eval(n->rhs, params) ? 1.0 : 0.0;
      case ExprKind::kLogAnd:
        return (eval(n->lhs, params) != 0.0 && eval(n->rhs, params) != 0.0)
                   ? 1.0
                   : 0.0;
      case ExprKind::kLogOr:
        return (eval(n->lhs, params) != 0.0 || eval(n->rhs, params) != 0.0)
                   ? 1.0
                   : 0.0;
      case ExprKind::kSin:
        return std::sin(eval(n->lhs, params));
      case ExprKind::kCos:
        return std::cos(eval(n->lhs, params));
      case ExprKind::kTan:
        return std::tan(eval(n->lhs, params));
      case ExprKind::kExp:
        return std::exp(eval(n->lhs, params));
      case ExprKind::kSqrt:
        return std::sqrt(eval(n->lhs, params));
      case ExprKind::kLn:
        return std::log(eval(n->lhs, params));
      case ExprKind::kArcsin:
        return std::asin(eval(n->lhs, params));
      case ExprKind::kArccos:
        return std::acos(eval(n->lhs, params));
      case ExprKind::kArctan:
        return std::atan(eval(n->lhs, params));
      case ExprKind::kFloor:
        return std::floor(eval(n->lhs, params));
    }
    return 0.0;  // unreachable
  }
};
using ExprPtr = std::shared_ptr<ExprNode>;

// ---------------------------------------------------------------------------
// parseTimingLiteral
//
// Converts a timing literal string to nanoseconds.
// "dt" uses the device cycle time from BackendConfig.
// ---------------------------------------------------------------------------
double parseTimingLiteral(const std::string& text, double dt_ns) {
  std::size_t i = 0;
  while (i < text.size() && ((std::isdigit(text[i]) != 0) || text[i] == '.')) {
    ++i;
  }
  double num = std::stod(text.substr(0, i));
  std::string unit = text.substr(i);
  if (unit == "ns") {
    return num;
  }
  if (unit == "us") {
    return num * 1e3;
  }
  if (unit == "ms") {
    return num * 1e6;
  }
  if (unit == "s") {
    return num * 1e9;
  }
  if (unit == "dt") {
    return num * dt_ns;
  }
  assert(false && "unknown timing unit in parseTimingLiteral");
  return 0.0;  // unreachable
}

// ---------------------------------------------------------------------------
// extractIntLiteral
//
// Fast-path: reads a decimal integer literal directly off an expression node.
// Needed for modifier counts/exponents requiring a compile-time integer.
// Falls back to default_val for anything more complex.
// ---------------------------------------------------------------------------
std::uint8_t extractIntLiteral(qasm3Parser::ExpressionContext* expr,
                               std::uint8_t default_val = 1) {
  auto* lit = dynamic_cast<qasm3Parser::LiteralExpressionContext*>(expr);
  if ((lit != nullptr) && (lit->DecimalIntegerLiteral() != nullptr)) {
    return static_cast<std::uint8_t>(
        std::stoi(lit->DecimalIntegerLiteral()->getText()));
  }
  return default_val;
}

// Returns the integer count carried by a modifier expression, e.g. ctrl(2).
int modifierCount(qasm3Parser::GateModifierContext* mod) {
  return (mod->expression() != nullptr)
             ? static_cast<int>(extractIntLiteral(mod->expression(), 1))
             : 1;
}

// ---------------------------------------------------------------------------
// Matrix helpers for gate modifier derivation (inv, pow, ctrl, negctrl).
// Matrices are stored in row-major order; qubit k = bit (n-1-k) of the index.
// ---------------------------------------------------------------------------

Mat identityMatrix(std::uint8_t n) {
  const std::size_t dim = std::size_t{1} << n;
  Mat m(dim * dim, C{0.0, 0.0});
  for (std::size_t i = 0; i < dim; ++i) {
    m[(i * dim) + i] = C{1.0, 0.0};
  }
  return m;
}

Mat conjugateTranspose(const Mat& m, std::uint8_t n) {
  const std::size_t dim = std::size_t{1} << n;
  Mat result(dim * dim);
  for (std::size_t i = 0; i < dim; ++i) {
    for (std::size_t j = 0; j < dim; ++j) {
      result[(i * dim) + j] = std::conj(m[(j * dim) + i]);
    }
  }
  return result;
}

Mat matMul(const Mat& a, const Mat& b, std::size_t dim) {
  Mat result(dim * dim, C{0.0, 0.0});
  for (std::size_t i = 0; i < dim; ++i) {
    for (std::size_t j = 0; j < dim; ++j) {
      for (std::size_t k = 0; k < dim; ++k) {
        result[(i * dim) + j] += a[(i * dim) + k] * b[(k * dim) + j];
      }
    }
  }
  return result;
}

Mat matrixPow(const Mat& m,
              std::uint8_t n,  // NOLINT(bugprone-easily-swappable-parameters)
              int power        // NOLINT(bugprone-easily-swappable-parameters)
) {
  const std::size_t dim = std::size_t{1} << n;
  Mat result = identityMatrix(n);
  for (int p = 0; p < power; ++p) {
    result = matMul(result, m, dim);
  }
  return result;
}

// Prepends one control qubit: identity on |0⟩, U on |1⟩.
Mat addControl(const Mat& m, std::uint8_t n) {
  const std::size_t dim = std::size_t{1} << n;
  const std::size_t newDim = dim * 2;
  Mat result(newDim * newDim, C{0.0, 0.0});
  for (std::size_t i = 0; i < dim; ++i) {
    result[(i * newDim) + i] = C{1.0, 0.0};
  }
  for (std::size_t i = 0; i < dim; ++i) {
    for (std::size_t j = 0; j < dim; ++j) {
      result[((dim + i) * newDim) + (dim + j)] = m[(i * dim) + j];
    }
  }
  return result;
}

// Prepends one negated control qubit: U on |0⟩, identity on |1⟩.
Mat addNegControl(const Mat& m, std::uint8_t n) {
  const std::size_t dim = std::size_t{1} << n;
  const std::size_t newDim = dim * 2;
  Mat result(newDim * newDim, C{0.0, 0.0});
  for (std::size_t i = 0; i < dim; ++i) {
    for (std::size_t j = 0; j < dim; ++j) {
      result[(i * newDim) + j] = m[(i * dim) + j];
    }
  }
  for (std::size_t i = 0; i < dim; ++i) {
    result[((dim + i) * newDim) + (dim + i)] = C{1.0, 0.0};
  }
  return result;
}

// Lifts a gate_n-qubit gate acting on qubit_indices into total_n-qubit space.
Mat embedGate(const Mat& gate_mat, std::uint8_t gate_n,
              const std::vector<std::uint8_t>& qubit_indices,
              std::uint8_t total_n) {
  if (gate_n == total_n) {
    return gate_mat;
  }
  const std::size_t totalDim = std::size_t{1} << total_n;
  const std::size_t gateDim = std::size_t{1} << gate_n;
  Mat result(totalDim * totalDim, C{0.0, 0.0});
  for (std::size_t inState = 0; inState < totalDim; ++inState) {
    std::size_t gateIn = 0;
    for (std::uint8_t k = 0; k < gate_n; ++k) {
      std::uint8_t bit = total_n - 1 - qubit_indices[k];
      if ((inState & (std::size_t{1} << bit)) != 0U) {
        gateIn |= std::size_t{1} << (gate_n - 1 - k);
      }
    }
    for (std::size_t gateOut = 0; gateOut < gateDim; ++gateOut) {
      C amp = gate_mat[(gateOut * gateDim) + gateIn];
      std::size_t outState = inState;
      for (std::uint8_t k = 0; k < gate_n; ++k) {
        std::uint8_t bit = total_n - 1 - qubit_indices[k];
        if ((gateOut & (std::size_t{1} << (gate_n - 1 - k))) != 0U) {
          outState |= std::size_t{1} << bit;
        } else {
          outState &= ~(std::size_t{1} << bit);
        }
      }
      result[(outState * totalDim) + inState] = amp;
    }
  }
  return result;
}

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
  explicit CircuitBuilder(const BackendConfig& config) : config_(config) {}

  // May only be called once. std::move transfers ownership out of the object.
  Circuit buildCircuit() {
    return {std::move(registry_), std::move(qubit_registers_),
            std::move(bit_registers_), std::move(operations_)};
  }

  [[nodiscard]] const std::vector<SyntaxError>& semanticErrors() const {
    return semantic_errors_;
  }

  // ---- statement visitors (alphabetical, matching qasm3Parser.g4) ---------

  // LET Identifier EQUALS aliasExpression SEMICOLON;
  std::any visitAliasDeclarationStatement(
      qasm3Parser::AliasDeclarationStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  // indexedIdentifier op=(EQUALS | CompoundAssignmentOperator)
  //   (expression | measureExpression | quantumCallExpression) SEMICOLON;
  std::any visitAssignmentStatement(
      qasm3Parser::AssignmentStatementContext* ctx) override {
    // c[i] = measure q[i];
    if (auto* meas = ctx->measureExpression()) {
      const std::string lhsName =
          ctx->indexedIdentifier()->Identifier()->getText();
      if (checkUndefined(UndefinedKind::kBitRegister, lhsName, ctx)) {
        return visitChildren(ctx);
      }
      const QubitReference qubit = resolveQubit(meas->gateOperand(), ctx);
      const BitReference target = resolveBit(ctx->indexedIdentifier(), ctx);
      operations_.push_back(
          {OperationType::kMeasure, nullptr, {}, {qubit}, {target}});
      return visitChildren(ctx);
    }

    if (ctx->expression() == nullptr) {
      return visitChildren(ctx);
    }

    auto* lhs = ctx->indexedIdentifier();
    const std::string name = lhs->Identifier()->getText();
    if (checkConstReassignment(name, ctx)) {
      return visitChildren(ctx);
    }

    // Build lookup key: "name" for scalar, "name[i]" for array element
    std::string key = name;
    if (!lhs->indexOperator().empty() &&
        !lhs->indexOperator(0)->expression().empty()) {
      const int idx = static_cast<int>(ExprNode::eval(
          buildExprTree(lhs->indexOperator(0)->expression(0)), {}));
      key = name + "[" + std::to_string(idx) + "]";
    }

    const C* existing = lookupScalar(key);
    if (existing == nullptr) {
      addError(errUndefined(UndefinedKind::kVariable, name), ctx);
      return visitChildren(ctx);
    }

    const std::string op = ctx->op->getText();

    // Plain assignment preserves the full complex value via evalC.
    if (op == "=") {
      const C val = evalC(ctx->expression());
      for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->scalar_map.find(key);
        if (found != it->scalar_map.end()) {
          found->second = val;
          const_vals_[key] = val.real();
          break;
        }
      }
      return visitChildren(ctx);
    }

    // Compound operators work on the real component only.
    const double rhs = ExprNode::eval(buildExprTree(ctx->expression()), {});
    double cur = existing->real();

    if (op == "+=") {
      {
        cur += rhs;
      }
    } else if (op == "-=") {
      {
        cur -= rhs;
      }
    } else if (op == "*=") {
      {
        cur *= rhs;
      }
    } else if (op == "/=") {
      {
        cur = (rhs != 0.0) ? cur / rhs : 0.0;
      }
    } else if (op == "%=") {
      {
        cur = std::fmod(cur, rhs);
      }
    } else if (op == "**=") {
      {
        cur = std::pow(cur, rhs);
      }
    } else if (op == "&=") {
      auto l = static_cast<long long>(cur);
      auto r = static_cast<long long>(rhs);
      cur = static_cast<double>(l & r);
    } else if (op == "|=") {
      auto l = static_cast<long long>(cur);
      auto r = static_cast<long long>(rhs);
      cur = static_cast<double>(l | r);
    } else if (op == "^=") {
      auto l = static_cast<long long>(cur);
      auto r = static_cast<long long>(rhs);
      cur = static_cast<double>(l ^ r);
    } else if (op == "<<=") {
      auto l = static_cast<long long>(cur);
      auto r = static_cast<long long>(rhs);
      cur = static_cast<double>(l << r);
    } else if (op == ">>=") {
      auto l = static_cast<long long>(cur);
      auto r = static_cast<long long>(rhs);
      cur = static_cast<double>(l >> r);
    } else {
      addError("unknown assignment operator '" + op + "'", ctx);
      return visitChildren(ctx);
    }

    // Write back to the innermost scope that owns this key
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
      auto found = it->scalar_map.find(key);
      if (found != it->scalar_map.end()) {
        found->second = C{cur, 0.0};
        const_vals_[key] = cur;
        break;
      }
    }
    return visitChildren(ctx);
  }

  // BARRIER gateOperandList? SEMICOLON;
  std::any visitBarrierStatement(
      qasm3Parser::BarrierStatementContext* ctx) override {
    std::vector<QubitReference> qubits;
    if (ctx->gateOperandList() == nullptr) {
      // Global barrier: expand over every declared qubit
      for (std::size_t r = 0; r < qubit_registers_.size(); ++r) {
        for (std::size_t q = 0; q < qubit_registers_[r].size; ++q) {
          qubits.push_back(
              {static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(q)});
        }
      }
    } else {
      for (auto* op : ctx->gateOperandList()->gateOperand()) {
        auto* indexed = op->indexedIdentifier();
        // Whole-register barrier: expand without an index
        if ((indexed != nullptr) && indexed->indexOperator().empty()) {
          auto it = qubit_map_.find(indexed->Identifier()->getText());
          if (it != qubit_map_.end()) {
            for (std::size_t q = 0; q < it->second.size; ++q) {
              qubits.push_back({static_cast<std::uint8_t>(it->second.index),
                                static_cast<std::uint8_t>(q)});
            }
            continue;
          }
        }
        qubits.push_back(resolveQubit(op, ctx));
      }
    }
    operations_.push_back(
        {OperationType::kBarrier, nullptr, {}, std::move(qubits), {}});
    return visitChildren(ctx);
  }

  // BOX designator? scope;
  std::any visitBoxStatement(qasm3Parser::BoxStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  // BREAK SEMICOLON;
  std::any visitBreakStatement(
      qasm3Parser::BreakStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  // DEFCALGRAMMAR StringLiteral SEMICOLON;
  std::any visitCalibrationGrammarStatement(
      qasm3Parser::CalibrationGrammarStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  // CAL LBRACE CalibrationBlock? RBRACE;
  std::any visitCalStatement(qasm3Parser::CalStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  // (scalarType | arrayType) Identifier (EQUALS declarationExpression)?
  //   SEMICOLON;
  std::any visitClassicalDeclarationStatement(
      qasm3Parser::ClassicalDeclarationStatementContext* ctx) override {
    const std::string name = ctx->Identifier()->getText();
    if (checkRedeclared(name, ctx)) {
      return visitChildren(ctx);
    }

    // Array type: store elements as "name[i]", "name[i][j]", etc.
    if (ctx->arrayType() != nullptr) {
      if (auto* decl = ctx->declarationExpression()) {
        if (auto* cast = dynamic_cast<qasm3Parser::CastExpressionContext*>(
                decl->expression())) {
          // array[T](src) cast — convert source array elements
          if (cast->arrayType() != nullptr) {
            if (auto* srcLit =
                    dynamic_cast<qasm3Parser::LiteralExpressionContext*>(
                        cast->expression())) {
              if (srcLit->Identifier() != nullptr) {
                const std::string src = srcLit->Identifier()->getText();
                const std::string pfx = src + "[";
                for (const auto& kv : scopes_.back().scalar_map) {
                  if (kv.first.size() >= pfx.size() &&
                      kv.first.compare(0, pfx.size(), pfx) == 0) {
                    setScalar(name + kv.first.substr(src.size()),
                              kv.second.real());
                  }
                }
              }
            }
            return visitChildren(ctx);
          }
        }
        if (auto* arr = decl->arrayLiteral()) {
          // array[T, n] a = {v0, v1, ...};
          std::function<void(qasm3Parser::ArrayLiteralContext*,
                             const std::string&)>
              store;
          store = [&](qasm3Parser::ArrayLiteralContext* a,
                      const std::string& prefix) {
            auto subs = a->arrayLiteral();
            if (!subs.empty()) {
              for (int i = 0; i < static_cast<int>(subs.size()); ++i) {
                store(subs[i], prefix + "[" + std::to_string(i) + "]");
              }
            } else {
              int i = 0;
              for (auto* expr : a->expression()) {
                setScalar(prefix + "[" + std::to_string(i++) + "]",
                          ExprNode::eval(buildExprTree(expr), {}));
              }
            }
          };
          store(arr, name);
        }
      }
      return visitChildren(ctx);
    }

    auto* scalar = ctx->scalarType();
    if (scalar == nullptr) {
      return visitChildren(ctx);
    }

    if (scalar->BIT() != nullptr) {
      if ((bit_map_.count(name) != 0U) || (qubit_map_.count(name) != 0U)) {
        addError("redeclaration of '" + name + "'", ctx);
        return visitChildren(ctx);
      }
      const std::size_t size =
          (scalar->designator() != nullptr)
              ? static_cast<std::size_t>(ExprNode::eval(
                    buildExprTree(scalar->designator()->expression()), {}))
              : 1;
      bit_map_[name] = {bit_registers_.size(), size};
      bit_registers_.push_back({name, size});
    } else if ((scalar->FLOAT() != nullptr) || (scalar->INT() != nullptr) ||
               (scalar->UINT() != nullptr) || (scalar->ANGLE() != nullptr)) {
      if (ctx->declarationExpression() != nullptr) {
        if (auto* expr = ctx->declarationExpression()->expression()) {
          setScalar(name, ExprNode::eval(buildExprTree(expr), {}));
        }
      }
    } else if (scalar->COMPLEX() != nullptr) {
      if (ctx->declarationExpression() != nullptr) {
        if (auto* expr = ctx->declarationExpression()->expression()) {
          setComplexScalar(name, evalC(expr));
        }
      }
    } else if (scalar->DURATION() != nullptr) {
      setScalar(name, 0.0);
      if (ctx->declarationExpression() != nullptr) {
        if (auto* expr = ctx->declarationExpression()->expression()) {
          setScalar(name, ExprNode::eval(buildExprTree(expr), {}));
        }
      }
    } else if (scalar->STRETCH() != nullptr) {
      setScalar(name, 0.0);
    }
    return visitChildren(ctx);
  }

  // CONST scalarType Identifier EQUALS declarationExpression SEMICOLON;
  std::any visitConstDeclarationStatement(
      qasm3Parser::ConstDeclarationStatementContext* ctx) override {
    const std::string name = ctx->Identifier()->getText();
    if (checkRedeclared(name, ctx)) {
      return visitChildren(ctx);
    }

    if (auto* expr = ctx->declarationExpression()->expression()) {
      setScalar(name, ExprNode::eval(buildExprTree(expr), {}));
      scopes_.back().const_names.insert(name);
    } else {
      addError("const initializer must be a scalar expression", ctx);
    }
    return visitChildren(ctx);
  }

  // CONTINUE SEMICOLON;
  std::any visitContinueStatement(
      qasm3Parser::ContinueStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  // DEFCAL defcalTarget (LPAREN defcalArgumentDefinitionList? RPAREN)?
  //   defcalOperandList returnSignature? LBRACE CalibrationBlock? RBRACE;
  std::any visitDefcalStatement(
      qasm3Parser::DefcalStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  // DEF Identifier LPAREN argumentDefinitionList? RPAREN returnSignature?
  //   scope;
  std::any visitDefStatement(qasm3Parser::DefStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  // DELAY designator gateOperandList? SEMICOLON;
  std::any visitDelayStatement(
      qasm3Parser::DelayStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  // END SEMICOLON;
  std::any visitEndStatement(qasm3Parser::EndStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  // expression SEMICOLON;
  std::any visitExpressionStatement(
      qasm3Parser::ExpressionStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  // EXTERN Identifier LPAREN externArgumentList? RPAREN returnSignature?
  //   SEMICOLON;
  std::any visitExternStatement(
      qasm3Parser::ExternStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  // FOR scalarType Identifier IN (setExpression | LBRACKET rangeExpression
  //   RBRACKET | expression) body=statementOrScope;
  std::any visitForStatement(qasm3Parser::ForStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  // gateModifier* Identifier (LPAREN expressionList? RPAREN)? gateOperandList
  //   SEMICOLON | gateModifier* GPHASE (LPAREN expressionList? RPAREN)?
  //   gateOperandList? SEMICOLON;
  std::any visitGateCallStatement(
      qasm3Parser::GateCallStatementContext* ctx) override {
    // ---- gphase special case -----------------------------------------------
    if (ctx->GPHASE() != nullptr) {
      auto modifiers = ctx->gateModifier();
      // Bare gphase has no observable effect without qubit modifiers
      if (modifiers.empty()) {
        return visitChildren(ctx);
      }

      double theta = 0.0;
      if ((ctx->expressionList() != nullptr) &&
          !ctx->expressionList()->expression().empty()) {
        theta = ExprNode::eval(
            buildExprTree(ctx->expressionList()->expression(0)), {});
      }

      auto gate = deriveGphaseWithModifiers(theta, modifiers, ctx);
      if (!gate) {
        return visitChildren(ctx);
      }

      std::vector<QubitReference> qubits;
      if (auto* ol = ctx->gateOperandList()) {
        for (auto* op : ol->gateOperand()) {
          qubits.push_back(resolveQubit(op, ctx));
        }
      }

      if (qubits.size() != gate->numQubits()) {
        addError("ctrl@gphase expects " + std::to_string(gate->numQubits()) +
                     " qubit(s), got " + std::to_string(qubits.size()),
                 ctx);
        return visitChildren(ctx);
      }
      operations_.push_back(
          {OperationType::kGate, gate, {}, std::move(qubits), {}});
      return visitChildren(ctx);
    }

    // ---- normal gate call --------------------------------------------------
    const std::string gateName = ctx->Identifier()->getText();
    if (checkUndefined(UndefinedKind::kGate, gateName, ctx)) {
      return visitChildren(ctx);
    }
    auto gate = registry_.find(gateName);

    // Evaluate parameter expressions with the current scalar scope
    std::vector<double> params;
    if (auto* el = ctx->expressionList()) {
      for (auto* expr : el->expression()) {
        params.push_back(ExprNode::eval(buildExprTree(expr), {}));
      }
    }

    auto modifiers = ctx->gateModifier();

    if (modifiers.empty() && params.size() != gate->numParams()) {
      addError("gate '" + gateName + "' expects " +
                   std::to_string(gate->numParams()) + " parameter(s), got " +
                   std::to_string(params.size()),
               ctx);
      return visitChildren(ctx);
    }

    if (!modifiers.empty()) {
      gate = deriveModifiedGate(gate, params, modifiers, ctx);
      if (!gate) {
        return visitChildren(ctx);
      }
      params.clear();
    }

    auto* operandList = ctx->gateOperandList();
    if (operandList == nullptr) {
      addError("gate '" + gateName + "' called with no qubit operands", ctx);
      return visitChildren(ctx);
    }
    auto operands = operandList->gateOperand();

    // Single-qubit gate with one unindexed operand: broadcast over register
    if (gate->numQubits() == 1 && modifiers.empty() && operands.size() == 1) {
      auto* indexed = operands[0]->indexedIdentifier();
      if ((indexed != nullptr) && indexed->indexOperator().empty()) {
        auto it = qubit_map_.find(indexed->Identifier()->getText());
        if (it != qubit_map_.end()) {
          for (std::size_t q = 0; q < it->second.size; ++q) {
            operations_.push_back(
                {OperationType::kGate,
                 gate,
                 params,
                 {{static_cast<std::uint8_t>(it->second.index),
                   static_cast<std::uint8_t>(q)}},
                 {}});
          }
          return visitChildren(ctx);
        }
      }
    }

    // Pairwise broadcast: all operands are whole registers of equal size
    if (modifiers.empty() &&
        operands.size() == static_cast<std::size_t>(gate->numQubits())) {
      bool allWhole = true;
      std::vector<RegisterInfo*> regs;
      for (auto* op : operands) {
        auto* idx = op->indexedIdentifier();
        if ((idx == nullptr) || !idx->indexOperator().empty()) {
          allWhole = false;
          break;
        }
        auto it = qubit_map_.find(idx->Identifier()->getText());
        if (it == qubit_map_.end()) {
          allWhole = false;
          break;
        }
        regs.push_back(&it->second);
      }
      if (allWhole && regs.size() > 1) {
        const std::size_t sz = regs[0]->size;
        bool same = true;
        for (auto* r : regs) {
          if (r->size != sz) {
            same = false;
            break;
          }
        }
        if (same) {
          for (std::size_t q = 0; q < sz; ++q) {
            std::vector<QubitReference> qubits;
            qubits.reserve(regs.size());
            for (auto* r : regs) {
              qubits.push_back({static_cast<std::uint8_t>(r->index),
                                static_cast<std::uint8_t>(q)});
            }
            operations_.push_back(
                {OperationType::kGate, gate, params, std::move(qubits), {}});
          }
          return visitChildren(ctx);
        }
      }
    }

    // Normal case: resolve each operand individually
    std::vector<QubitReference> qubits;
    qubits.reserve(operands.size());
    for (auto* operand : operands) {
      qubits.push_back(resolveQubit(operand, ctx));
    }

    if (qubits.size() != gate->numQubits()) {
      addError(
          errWrongQubitCount(gate->name(), static_cast<int>(gate->numQubits()),
                             static_cast<int>(qubits.size())),
          ctx);
      return visitChildren(ctx);
    }
    operations_.push_back(
        {OperationType::kGate, gate, std::move(params), std::move(qubits), {}});
    return visitChildren(ctx);
  }

  // GATE Identifier (LPAREN params=identifierList? RPAREN)?
  //   qubits=identifierList scope;
  std::any visitGateStatement(qasm3Parser::GateStatementContext* ctx) override {
    const std::string name = ctx->Identifier()->getText();
    if (checkRedefined(RedefinedKind::kGate, name, ctx)) {
      return {};
    }

    std::vector<std::string> paramNames;
    if (ctx->params != nullptr) {
      for (auto* id : ctx->params->Identifier()) {
        paramNames.push_back(id->getText());
      }
    }

    std::vector<std::string> qubitNames;
    if (ctx->qubits != nullptr) {
      for (auto* id : ctx->qubits->Identifier()) {
        qubitNames.push_back(id->getText());
      }
    }

    if (qubitNames.empty()) {
      addError("gate '" + name + "' must operate on at least one qubit", ctx);
      return {};
    }

    std::unordered_map<std::string, int> paramIdx;
    for (int i = 0; i < static_cast<int>(paramNames.size()); ++i) {
      paramIdx[paramNames[i]] = i;
    }

    std::unordered_map<std::string, std::uint8_t> localQubitIdx;
    for (std::size_t i = 0; i < qubitNames.size(); ++i) {
      localQubitIdx[qubitNames[i]] = static_cast<std::uint8_t>(i);
    }

    const auto numQubits = static_cast<std::uint8_t>(qubitNames.size());
    const auto numParams = static_cast<std::uint8_t>(paramNames.size());

    // Deferred modifier kind — used when base gate is parametric
    struct BodyMod {
      enum class Kind { kInv, kPow, kCtrl, kNegCtrl };
      Kind kind;
      int count = 1;
    };
    struct BodyCall {
      std::shared_ptr<const GateDefinition> gate;
      std::vector<ExprPtr> param_exprs;
      std::vector<std::uint8_t> qubit_indices;
      std::vector<BodyMod> modifiers;
    };
    std::vector<BodyCall> body;

    for (auto* sos : ctx->scope()->statementOrScope()) {
      auto* stmt = sos->statement();
      if (stmt == nullptr) {
        continue;
      }
      auto* call = stmt->gateCallStatement();
      if ((call == nullptr) || (call->GPHASE() != nullptr)) {
        continue;
      }

      const std::string callName = call->Identifier()->getText();
      auto callGate = registry_.find(callName);
      if (!callGate) {
        addError(errUndefined(UndefinedKind::kGate, callName), call);
        continue;
      }

      auto bodyMods = call->gateModifier();
      BodyCall bc;
      bc.gate = callGate;

      if (!bodyMods.empty() && callGate->numParams() == 0) {
        // Non-parametric base: fold modifiers into the matrix now
        callGate = deriveModifiedGate(callGate, {}, bodyMods, call);
        if (!callGate) {
          continue;
        }
        bc.gate = callGate;
      } else if (!bodyMods.empty()) {
        // Parametric base: record modifiers, apply at call time
        for (auto* mod : bodyMods) {
          if (mod->INV() != nullptr) {
            bc.modifiers.push_back({BodyMod::Kind::kInv, 1});
          } else if (mod->POW() != nullptr) {
            bc.modifiers.push_back({BodyMod::Kind::kPow, modifierCount(mod)});
          } else if (mod->CTRL() != nullptr) {
            bc.modifiers.push_back({BodyMod::Kind::kCtrl, modifierCount(mod)});
          } else if (mod->NEGCTRL() != nullptr) {
            bc.modifiers.push_back(
                {BodyMod::Kind::kNegCtrl, modifierCount(mod)});
          }
        }
        if (auto* el = call->expressionList()) {
          for (auto* expr : el->expression()) {
            bc.param_exprs.push_back(buildExprTree(expr, paramIdx));
          }
        }
      } else {
        if (auto* el = call->expressionList()) {
          for (auto* expr : el->expression()) {
            bc.param_exprs.push_back(buildExprTree(expr, paramIdx));
          }
        }
      }

      if (auto* ol = call->gateOperandList()) {
        for (auto* operand : ol->gateOperand()) {
          auto* indexed = operand->indexedIdentifier();
          if (indexed == nullptr) {
            continue;
          }
          const std::string qname = indexed->Identifier()->getText();
          auto it = localQubitIdx.find(qname);
          if (it != localQubitIdx.end()) {
            bc.qubit_indices.push_back(it->second);
          } else {
            addError("undefined qubit parameter '" + qname +
                         "' in body of gate '" + name + "'",
                     call);
          }
        }
      }
      body.push_back(std::move(bc));
    }

    if (numParams == 0) {
      // Parameterless: compute full matrix immediately
      const std::size_t dim = std::size_t{1} << numQubits;
      Mat result = identityMatrix(numQubits);
      for (auto& bc : body) {
        Mat gateMat = bc.gate->matrix({});
        Mat embedded = embedGate(gateMat, bc.gate->numQubits(),
                                 bc.qubit_indices, numQubits);
        result = matMul(embedded, result, dim);
      }
      registry_.add(GateDefinition(name, numQubits, std::move(result)));
    } else {
      // Parametric: defer matrix computation to each call site
      registry_.add(GateDefinition(
          name, numQubits, numParams,
          [body, numQubits](const std::vector<double>& params) -> Mat {
            const std::size_t dim = std::size_t{1} << numQubits;
            Mat result = identityMatrix(numQubits);
            for (const auto& bc : body) {
              std::vector<double> callParams;
              callParams.reserve(bc.param_exprs.size());
              for (const auto& ep : bc.param_exprs) {
                callParams.push_back(ExprNode::eval(ep, params));
              }
              Mat gateMat = bc.gate->matrix(callParams);
              std::uint8_t gateN = bc.gate->numQubits();
              for (int i = static_cast<int>(bc.modifiers.size()) - 1; i >= 0;
                   --i) {
                const auto& mod = bc.modifiers[i];
                switch (mod.kind) {
                  case BodyMod::Kind::kInv:
                    gateMat = conjugateTranspose(gateMat, gateN);
                    break;
                  case BodyMod::Kind::kPow:
                    gateMat = matrixPow(gateMat, gateN, mod.count);
                    break;
                  case BodyMod::Kind::kCtrl:
                    for (int c = 0; c < mod.count; ++c) {
                      gateMat = addControl(gateMat, gateN);
                      ++gateN;
                    }
                    break;
                  case BodyMod::Kind::kNegCtrl:
                    for (int c = 0; c < mod.count; ++c) {
                      gateMat = addNegControl(gateMat, gateN);
                      ++gateN;
                    }
                    break;
                }
              }
              Mat embedded =
                  embedGate(gateMat, gateN, bc.qubit_indices, numQubits);
              result = matMul(embedded, result, dim);
            }
            return result;
          }));
    }
    // visitChildren is intentionally NOT called: recursing into the body
    // would invoke visitGateCallStatement for each body call, which would
    // attempt to resolve the gate's formal qubit parameters against the
    // circuit's qubit_map_ (where they don't exist) and emit spurious errors
    // while appending phantom operations to operations_.
    return {};
  }

  // IF LPAREN expression RPAREN if_body=statementOrScope
  //   (ELSE else_body=statementOrScope)?;
  std::any visitIfStatement(qasm3Parser::IfStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  // INCLUDE StringLiteral SEMICOLON;
  std::any visitIncludeStatement(
      qasm3Parser::IncludeStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  // (INPUT | OUTPUT) (scalarType | arrayType) Identifier SEMICOLON;
  std::any visitIoDeclarationStatement(
      qasm3Parser::IoDeclarationStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  // (measureExpression | quantumCallExpression) (ARROW indexedIdentifier)?
  //   SEMICOLON;
  std::any visitMeasureArrowAssignmentStatement(
      qasm3Parser::MeasureArrowAssignmentStatementContext* ctx) override {
    auto* meas = ctx->measureExpression();
    // quantumCallExpression form is not yet handled
    if (meas == nullptr) {
      return visitChildren(ctx);
    }

    auto* qubitOperand = meas->gateOperand();
    auto* bitTarget = ctx->indexedIdentifier();

    // Validate register names before resolving
    auto* qIndexed = qubitOperand->indexedIdentifier();
    if (qIndexed != nullptr) {
      const std::string qname = qIndexed->Identifier()->getText();
      if (qubit_map_.count(qname) == 0U) {
        addError(errUndefined(UndefinedKind::kQubitRegister, qname), ctx);
        return visitChildren(ctx);
      }
    }
    if (bitTarget != nullptr) {
      const std::string cname = bitTarget->Identifier()->getText();
      if (bit_map_.count(cname) == 0U) {
        addError(errUndefined(UndefinedKind::kBitRegister, cname), ctx);
        return visitChildren(ctx);
      }
    }

    // Whole-register broadcast: measure q -> c expands to N per-qubit ops
    if ((qIndexed != nullptr) && qIndexed->indexOperator().empty() &&
        (bitTarget != nullptr) && bitTarget->indexOperator().empty()) {
      auto qIt = qubit_map_.find(qIndexed->Identifier()->getText());
      auto cIt = bit_map_.find(bitTarget->Identifier()->getText());
      if (qIt != qubit_map_.end() && cIt != bit_map_.end()) {
        if (qIt->second.size != cIt->second.size) {
          addError("register size mismatch in measurement: '" + qIt->first +
                       "' (" + std::to_string(qIt->second.size) +
                       " qubits) vs '" + cIt->first + "' (" +
                       std::to_string(cIt->second.size) + " bits)",
                   ctx);
          return visitChildren(ctx);
        }
        for (std::size_t i = 0; i < qIt->second.size; ++i) {
          operations_.push_back({OperationType::kMeasure,
                                 nullptr,
                                 {},
                                 {{static_cast<std::uint8_t>(qIt->second.index),
                                   static_cast<std::uint8_t>(i)}},
                                 {{static_cast<std::uint8_t>(cIt->second.index),
                                   static_cast<std::uint8_t>(i)}}});
        }
        return visitChildren(ctx);
      }
    }

    const QubitReference qubit = resolveQubit(qubitOperand, ctx);
    std::vector<BitReference> targets;
    if (bitTarget != nullptr) {
      targets.push_back(resolveBit(bitTarget, ctx));
    }
    operations_.push_back(
        {OperationType::kMeasure, nullptr, {}, {qubit}, std::move(targets)});
    return visitChildren(ctx);
  }

  // NOP gateOperandList? SEMICOLON;
  std::any visitNopStatement(qasm3Parser::NopStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  // (CREG | QREG) Identifier designator? SEMICOLON;
  std::any visitOldStyleDeclarationStatement(
      qasm3Parser::OldStyleDeclarationStatementContext* ctx) override {
    const std::string name = ctx->Identifier()->getText();
    if (checkRedeclared(name, ctx)) {
      return visitChildren(ctx);
    }
    const std::size_t size =
        (ctx->designator() != nullptr)
            ? static_cast<std::size_t>(ExprNode::eval(
                  buildExprTree(ctx->designator()->expression()), {}))
            : 1;
    if (ctx->QREG() != nullptr) {
      qubit_map_[name] = {qubit_registers_.size(), size};
      qubit_registers_.push_back({name, size});
    } else {  // CREG
      bit_map_[name] = {bit_registers_.size(), size};
      bit_registers_.push_back({name, size});
    }
    return visitChildren(ctx);
  }

  // qubitType Identifier SEMICOLON;
  std::any visitQuantumDeclarationStatement(
      qasm3Parser::QuantumDeclarationStatementContext* ctx) override {
    const std::string name = ctx->Identifier()->getText();
    if (checkRedeclared(name, ctx)) {
      return visitChildren(ctx);
    }
    const std::size_t size =
        (ctx->qubitType()->designator() != nullptr)
            ? static_cast<std::size_t>(ExprNode::eval(
                  buildExprTree(ctx->qubitType()->designator()->expression()),
                  {}))
            : 1;
    qubit_map_[name] = {qubit_registers_.size(), size};
    qubit_registers_.push_back({name, size});
    return visitChildren(ctx);
  }

  // RESET gateOperand SEMICOLON;
  std::any visitResetStatement(
      qasm3Parser::ResetStatementContext* ctx) override {
    auto* indexed = ctx->gateOperand()->indexedIdentifier();
    // Whole-register reset: reset q; expands to N per-qubit resets
    if ((indexed != nullptr) && indexed->indexOperator().empty()) {
      const std::string name = indexed->Identifier()->getText();
      auto it = qubit_map_.find(name);
      if (it != qubit_map_.end()) {
        for (std::size_t q = 0; q < it->second.size; ++q) {
          operations_.push_back({OperationType::kReset,
                                 nullptr,
                                 {},
                                 {{static_cast<std::uint8_t>(it->second.index),
                                   static_cast<std::uint8_t>(q)}},
                                 {}});
        }
        return visitChildren(ctx);
      }
      checkUndefined(UndefinedKind::kQubitRegister, name, ctx);
      return visitChildren(ctx);
    }
    const QubitReference qubit = resolveQubit(ctx->gateOperand(), ctx);
    operations_.push_back({OperationType::kReset, nullptr, {}, {qubit}, {}});
    return visitChildren(ctx);
  }

  // RETURN (expression | measureExpression | quantumCallExpression)? SEMICOLON;
  std::any visitReturnStatement(
      qasm3Parser::ReturnStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  // SWITCH LPAREN expression RPAREN LBRACE switchCaseItem* RBRACE;
  std::any visitSwitchStatement(
      qasm3Parser::SwitchStatementContext* ctx) override {
    return visitChildren(ctx);
  }

  // WHILE LPAREN expression RPAREN body=statementOrScope;
  std::any visitWhileStatement(
      qasm3Parser::WhileStatementContext* ctx) override {
    return visitChildren(ctx);
  }

 private:
  // ---- types --------------------------------------------------------------
  struct RegisterInfo {
    std::size_t index;
    std::size_t size;
  };

  struct SubroutineInfo {
    std::uint8_t param_count = 0;
    bool is_extern = false;   // true = extern, no body
    bool has_return = false;  // true = returns a value
  };

  struct Scope {  // only classical scalars are scoped in OpenQASM 3.0
    std::unordered_map<std::string, C> scalar_map;
    std::unordered_set<std::string> const_names;
  };

  enum class UndefinedKind {
    kGate,
    kSubroutine,
    kQubitRegister,
    kBitRegister,
    kVariable
  };
  enum class RedeclaredKind { kQubitRegister, kBitRegister, kVariable };
  enum class RedefinedKind { kGate, kSubroutine };

  // ---- circuit state ------------------------------------------------------

  BackendConfig config_;
  GateRegistry registry_ = GateRegistry::withBuiltins();
  std::vector<QubitRegister> qubit_registers_;
  std::vector<BitRegister> bit_registers_;
  std::vector<Operation> operations_;

  // ---- symbol tables ------------------------------------------------------

  std::unordered_map<std::string, RegisterInfo> qubit_map_;
  std::unordered_map<std::string, RegisterInfo> bit_map_;
  std::unordered_map<std::string, SubroutineInfo> subroutine_map_;
  std::vector<Scope> scopes_ = {{}};
  // Flat mirror of every scalar_map entry across all scopes.
  // Kept in sync via setScalar() to avoid rebuilding on every expression.
  std::unordered_map<std::string, double> const_vals_;
  // Modifier-derived gates (e.g. "ctrl@h"). Kept separate from registry_
  // so synthetic names are never visible as user-callable definitions.
  std::unordered_map<std::string, std::shared_ptr<GateDefinition>>
      derived_gates_;

  // ---- errors -------------------------------------------------------------

  std::vector<SyntaxError> semantic_errors_;

  // ---- error helpers ------------------------------------------------------

  void addError(const std::string& msg, antlr4::ParserRuleContext* ctx) {
    semantic_errors_.push_back(
        {static_cast<std::size_t>(ctx->getStart()->getLine()),
         static_cast<std::size_t>(ctx->getStart()->getCharPositionInLine()),
         msg});
  }

  static std::string errUndefined(UndefinedKind kind, const std::string& name) {
    switch (kind) {
      case UndefinedKind::kGate:
        return "undefined gate '" + name + "'";
      case UndefinedKind::kSubroutine:
        return "undefined subroutine '" + name + "'";
      case UndefinedKind::kQubitRegister:
        return "undefined qubit register '" + name + "'";
      case UndefinedKind::kBitRegister:
        return "undefined bit register '" + name + "'";
      case UndefinedKind::kVariable:
        return "undefined variable '" + name + "'";
    }

    return "";
  }

  static std::string errRedeclared(RedeclaredKind kind,
                                   const std::string& name) {
    switch (kind) {
      case RedeclaredKind::kQubitRegister:
        return "redeclaration of qubit register '" + name + "'";
      case RedeclaredKind::kBitRegister:
        return "redeclaration of bit register '" + name + "'";
      case RedeclaredKind::kVariable:
        return "redeclaration of variable '" + name + "'";
    }

    return "";
  }

  static std::string errRedefined(RedefinedKind kind, const std::string& name) {
    switch (kind) {
      case RedefinedKind::kGate:
        return "redefinition of gate '" + name + "'";
      case RedefinedKind::kSubroutine:
        return "redefinition of subroutine '" + name + "'";
    }

    return "";
  }

  static std::string errWrongQubitCount(const std::string& name, int expected,
                                        int got) {
    return "gate '" + name + "' expects " + std::to_string(expected) +
           " qubit(s), got " + std::to_string(got);
  }

  static std::string errConstReassignment(const std::string& name) {
    return "cannot assign to const '" + name + "'";
  }

  static std::string errOutOfBounds(const std::string& arr,
                                    const std::string& key) {
    return "array index '" + key + "' out of bounds for array '" + arr + "'";
  }

  void setScalar(const std::string& key, double val) {
    scopes_.back().scalar_map[key] = C{val, 0.0};
    const_vals_[key] = val;
  }

  void setComplexScalar(const std::string& key, C val) {
    scopes_.back().scalar_map[key] = val;
    const_vals_[key] = val.real();
  }

  [[nodiscard]] const C* lookupScalar(const std::string& name) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
      auto found = it->scalar_map.find(name);
      if (found != it->scalar_map.end()) {
        return &found->second;
      }
    }
    return nullptr;
  }

  [[nodiscard]] bool isConst(const std::string& name) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
      if (it->const_names.count(name) != 0U) {
        return true;
      }
    }
    return false;
  }

  bool checkRedeclared(const std::string& name,
                       antlr4::ParserRuleContext* ctx) {
    if (qubit_map_.count(name) != 0U) {
      addError(errRedeclared(RedeclaredKind::kQubitRegister, name), ctx);
      return true;
    }
    if (bit_map_.count(name) != 0U) {
      addError(errRedeclared(RedeclaredKind::kBitRegister, name), ctx);
      return true;
    }
    if (lookupScalar(name) != nullptr) {
      addError(errRedeclared(RedeclaredKind::kVariable, name), ctx);
      return true;
    }
    return false;
  }

  bool checkUndefined(UndefinedKind kind, const std::string& name,
                      antlr4::ParserRuleContext* ctx) {
    bool isUndefined = false;
    switch (kind) {
      case UndefinedKind::kGate:
        isUndefined = !registry_.contains(name);
        break;
      case UndefinedKind::kSubroutine:
        isUndefined = (subroutine_map_.count(name) == 0U);
        break;
      case UndefinedKind::kQubitRegister:
        isUndefined = (qubit_map_.count(name) == 0U);
        break;
      case UndefinedKind::kBitRegister:
        isUndefined = (bit_map_.count(name) == 0U);
        break;
      case UndefinedKind::kVariable:
        isUndefined = (lookupScalar(name) == nullptr);
        break;
    }

    if (!isUndefined) {
      return false;
    }

    addError(errUndefined(kind, name), ctx);
    return true;
  }

  bool checkRedefined(RedefinedKind kind, const std::string& name,
                      antlr4::ParserRuleContext* ctx) {
    bool isRedefined = false;
    switch (kind) {
      case RedefinedKind::kGate:
        isRedefined = registry_.contains(name);
        break;
      case RedefinedKind::kSubroutine:
        isRedefined = (subroutine_map_.count(name) != 0U);
        break;
    }

    if (!isRedefined) {
      return false;
    }

    addError(errRedefined(kind, name), ctx);
    return true;
  }

  bool checkConstReassignment(const std::string& name,
                              antlr4::ParserRuleContext* ctx) {
    if (!isConst(name)) {
      return false;
    }

    addError(errConstReassignment(name), ctx);
    return true;
  }

  // ---- qubit / bit reference resolution ----------------------------------

  // Resolves a gateOperand to a QubitReference. Emits an error and returns
  // {0,0} for hardware qubits or unknown register names.
  QubitReference resolveQubit(qasm3Parser::GateOperandContext* operand,
                              antlr4::ParserRuleContext* error_ctx) {
    auto* indexed = operand->indexedIdentifier();
    if (indexed == nullptr) {
      addError("hardware qubit references ($N) are not supported", error_ctx);
      return {0, 0};
    }
    const std::string name = indexed->Identifier()->getText();
    auto it = qubit_map_.find(name);
    if (it == qubit_map_.end()) {
      addError(errUndefined(UndefinedKind::kQubitRegister, name), error_ctx);
      return {0, 0};
    }
    std::size_t qubitIdx = 0;
    if (!indexed->indexOperator().empty()) {
      auto* idxOp = indexed->indexOperator(0);
      if (!idxOp->expression().empty()) {
        qubitIdx = static_cast<std::size_t>(
            ExprNode::eval(buildExprTree(idxOp->expression(0)), {}));
      }
    }
    return {static_cast<std::uint8_t>(it->second.index),
            static_cast<std::uint8_t>(qubitIdx)};
  }

  BitReference resolveBit(qasm3Parser::IndexedIdentifierContext* indexed,
                          antlr4::ParserRuleContext* error_ctx) {
    const std::string name = indexed->Identifier()->getText();
    auto it = bit_map_.find(name);
    if (it == bit_map_.end()) {
      addError(errUndefined(UndefinedKind::kBitRegister, name), error_ctx);
      return {0, 0};
    }
    std::size_t bitIdx = 0;
    if (!indexed->indexOperator().empty()) {
      auto* idxOp = indexed->indexOperator(0);
      if (!idxOp->expression().empty()) {
        bitIdx = static_cast<std::size_t>(
            ExprNode::eval(buildExprTree(idxOp->expression(0)), {}));
      }
    }
    return {static_cast<std::uint8_t>(it->second.index),
            static_cast<std::uint8_t>(bitIdx)};
  }

  // ---- gate modifier helpers ----------------------------------------------

  // Builds a canonical name for a modifier-derived gate, e.g. "ctrl@inv@h".
  static std::string buildModifiedName(
      const std::string& base_name, const std::vector<double>& base_params,
      const std::vector<qasm3Parser::GateModifierContext*>& modifiers) {
    std::string name;
    for (auto* mod : modifiers) {
      if (mod->INV() != nullptr) {
        name += "inv@";
      } else if (mod->POW() != nullptr) {
        name += "pow(" + std::to_string(modifierCount(mod)) + ")@";
      } else if (mod->CTRL() != nullptr) {
        for (int i = 0; i < modifierCount(mod); ++i) {
          name += "ctrl@";
        }
      } else if (mod->NEGCTRL() != nullptr) {
        for (int i = 0; i < modifierCount(mod); ++i) {
          name += "negctrl@";
        }
      }
    }
    name += base_name;
    if (!base_params.empty()) {
      name += "(";
      for (std::size_t i = 0; i < base_params.size(); ++i) {
        if (i > 0) {
          name += ",";
        }
        name += std::to_string(base_params[i]);
      }
      name += ")";
    }
    return name;
  }

  // Returns a gate derived from `base` by applying modifiers.
  // Results are cached in derived_gates_ (not registry_) so synthetic names
  // like "ctrl@h" are never visible as user-callable gate definitions.
  // Modifiers are applied in reverse list order per OpenQASM 3.0 §6.5.
  std::shared_ptr<const GateDefinition> deriveModifiedGate(
      const std::shared_ptr<const GateDefinition>& base,
      const std::vector<double>& base_params,
      const std::vector<qasm3Parser::GateModifierContext*>& modifiers,
      antlr4::ParserRuleContext* error_ctx) {
    const std::string name =
        buildModifiedName(base->name(), base_params, modifiers);
    auto it = derived_gates_.find(name);
    if (it != derived_gates_.end()) {
      return it->second;
    }

    Mat mat = base->matrix(base_params);
    std::uint8_t n = base->numQubits();

    for (int i = static_cast<int>(modifiers.size()) - 1; i >= 0; --i) {
      auto* mod = modifiers[i];
      if (mod->INV() != nullptr) {
        mat = conjugateTranspose(mat, n);
      } else if (mod->POW() != nullptr) {
        mat = matrixPow(mat, n, modifierCount(mod));
      } else if (mod->CTRL() != nullptr) {
        const int count = modifierCount(mod);
        for (int c = 0; c < count; ++c) {
          mat = addControl(mat, n);
          ++n;
        }
      } else if (mod->NEGCTRL() != nullptr) {
        const int count = modifierCount(mod);
        for (int c = 0; c < count; ++c) {
          mat = addNegControl(mat, n);
          ++n;
        }
      } else {
        addError("unknown gate modifier", error_ctx);
        return nullptr;
      }
    }

    auto gate = std::make_shared<GateDefinition>(name, n, std::move(mat));
    derived_gates_[name] = gate;
    return gate;
  }

  // Returns a ctrl/inv-modified gphase gate, cached in derived_gates_.
  // Returns nullptr when no qubit modifiers are present (unobservable).
  std::shared_ptr<const GateDefinition> deriveGphaseWithModifiers(
      double theta,
      const std::vector<qasm3Parser::GateModifierContext*>& modifiers,
      antlr4::ParserRuleContext* /*error_ctx*/) {
    std::string name;
    for (auto* mod : modifiers) {
      if (mod->INV() != nullptr) {
        name += "inv@";
      } else if (mod->POW() != nullptr) {
        name += "pow(" + std::to_string(modifierCount(mod)) + ")@";
      } else if (mod->CTRL() != nullptr) {
        for (int i = 0; i < modifierCount(mod); ++i) {
          name += "ctrl@";
        }
      } else if (mod->NEGCTRL() != nullptr) {
        for (int i = 0; i < modifierCount(mod); ++i) {
          name += "negctrl@";
        }
      }
    }
    name += "gphase(" + std::to_string(theta) + ")";

    auto it = derived_gates_.find(name);
    if (it != derived_gates_.end()) {
      return it->second;
    }

    Mat mat = {std::exp(C{0.0, theta})};
    std::uint8_t n = 0;

    for (int i = static_cast<int>(modifiers.size()) - 1; i >= 0; --i) {
      auto* mod = modifiers[i];
      if (mod->INV() != nullptr) {
        mat[0] = std::conj(mat[0]);
      } else if (mod->POW() != nullptr) {
        C result{1.0, 0.0};
        for (int p = 0; p < modifierCount(mod); ++p) {
          result *= mat[0];
        }
        mat[0] = result;
      } else if (mod->CTRL() != nullptr) {
        const int count = modifierCount(mod);
        for (int c = 0; c < count; ++c) {
          mat = addControl(mat, n);
          ++n;
        }
      } else if (mod->NEGCTRL() != nullptr) {
        const int count = modifierCount(mod);
        for (int c = 0; c < count; ++c) {
          mat = addNegControl(mat, n);
          ++n;
        }
      }
    }

    if (n == 0) {
      return nullptr;  // no qubit modifiers — unobservable
    }
    auto gate = std::make_shared<GateDefinition>(name, n, std::move(mat));
    derived_gates_[name] = gate;
    return gate;
  }

  // ---- expression evaluation ----------------------------------------------
  // Converts an ANTLR ExpressionContext into an ExprNode tree that can be
  // evaluated independently of ANTLR after parsing ends.
  //
  // TODO:
  // implement DurationofExpressionContext
  // implement user functions CallExpressionContext

  // Entry point — delegates with the pre-maintained const_vals_ mirror.
  ExprPtr buildExprTree(
      qasm3Parser::ExpressionContext* expr,
      const std::unordered_map<std::string, int>& param_idx = {}) {
    return buildExprTreeImpl(expr, param_idx, const_vals_);
  }

  ExprPtr buildExprTreeImpl(
      qasm3Parser::ExpressionContext* expr,
      const std::unordered_map<std::string, int>& param_idx,
      const std::unordered_map<std::string, double>& const_vals) {
    auto n = std::make_shared<ExprNode>();
    const double dtNs = config_.dtNs();

    // LPAREN expression RPAREN
    if (auto* e =
            dynamic_cast<qasm3Parser::ParenthesisExpressionContext*>(expr)) {
      return buildExprTreeImpl(e->expression(), param_idx, const_vals);

      // expression indexOperator
    }
    if (auto* e = dynamic_cast<qasm3Parser::IndexExpressionContext*>(expr)) {
      std::vector<int> indices;
      std::string arrName;
      auto* cur = e;
      while (cur != nullptr) {
        if (cur->indexOperator() == nullptr) {
          break;
        }

        auto* idxOp = cur->indexOperator();
        // Gate param refs in array indices evaluate to 0.0 here since actual
        // param values are only known at call time. a[theta] in a gate body
        // is unsupported, gate params are rotation angles, not integer indices.
        std::vector<double> dummy(param_idx.size(), 0.0);

        // handle both [] and comma-separated indices (arr[i][j] and arr[i, j])
        for (size_t i = 0; i < idxOp->expression().size(); ++i) {
          auto idxTree =
              buildExprTreeImpl(idxOp->expression(i), param_idx, const_vals);

          int idx = static_cast<int>(ExprNode::eval(idxTree, dummy));
          indices.push_back(idx);
        }

        auto* base = cur->expression();
        if (auto* lit =
                dynamic_cast<qasm3Parser::LiteralExpressionContext*>(base)) {
          if (lit->Identifier() != nullptr) {
            arrName = lit->Identifier()->getText();
          }
          break;
        }
        if (auto* next =
                dynamic_cast<qasm3Parser::IndexExpressionContext*>(base)) {
          cur = next;
        } else {
          addError("cannot index a non-array expression", expr);
          break;
        }
      }
      if (!arrName.empty()) {
        std::string key = arrName;
        for (auto it = indices.rbegin(); it != indices.rend(); ++it) {
          key += "[" + std::to_string(*it) + "]";
        }

        // Check whether any element of this array was declared.
        // "m[0]" works for 1D; for N-D, look for any "m[..." key.
        const std::string pfx = arrName + "[";
        bool declared = const_vals.count(arrName) > 0;
        if (!declared) {
          for (const auto& [k, _] : const_vals) {
            if (k.size() >= pfx.size() && k.compare(0, pfx.size(), pfx) == 0) {
              declared = true;
              break;
            }
          }
        }
        if (!declared) {
          addError(errUndefined(UndefinedKind::kVariable, arrName), expr);
        } else {
          auto it = const_vals.find(key);
          if (it != const_vals.end()) {
            n->value = it->second;
            return n;
          }
          addError(errOutOfBounds(arrName, key), expr);
        }
      }

      // <assoc=right> expression op=DOUBLE_ASTERISK expression
    } else if (auto* e =
                   dynamic_cast<qasm3Parser::PowerExpressionContext*>(expr)) {
      n->kind = ExprNode::ExprKind::kPow;
      n->lhs = buildExprTreeImpl(e->expression(0), param_idx, const_vals);
      n->rhs = buildExprTreeImpl(e->expression(1), param_idx, const_vals);

      // op=(TILDE | EXCLAMATION_POINT | MINUS) expression
    } else if (auto* e =
                   dynamic_cast<qasm3Parser::UnaryExpressionContext*>(expr)) {
      if (e->TILDE() != nullptr) {
        n->kind = ExprNode::ExprKind::kBitNot;
      } else if (e->EXCLAMATION_POINT() != nullptr) {
        n->kind = ExprNode::ExprKind::kLogNot;
      } else if (e->MINUS() != nullptr) {
        n->kind = ExprNode::ExprKind::kNeg;
      } else {
        assert(false && "unhandled operator in UnaryExpressionContext");
      }
      n->lhs = buildExprTreeImpl(e->expression(), param_idx, const_vals);

      // expression op=(ASTERISK | SLASH | PERCENT) expression
    } else if (auto* e =
                   dynamic_cast<qasm3Parser::MultiplicativeExpressionContext*>(
                       expr)) {
      if (e->ASTERISK() != nullptr) {
        n->kind = ExprNode::ExprKind::kMul;
      } else if (e->SLASH() != nullptr) {
        n->kind = ExprNode::ExprKind::kDiv;
      } else if (e->PERCENT() != nullptr) {
        n->kind = ExprNode::ExprKind::kMod;
      } else {
        assert(false &&
               "unhandled operator in MultiplicativeExpressionContext");
      }
      n->lhs = buildExprTreeImpl(e->expression(0), param_idx, const_vals);
      n->rhs = buildExprTreeImpl(e->expression(1), param_idx, const_vals);

      // expression op=(PLUS | MINUS) expression
    } else if (auto* e = dynamic_cast<qasm3Parser::AdditiveExpressionContext*>(
                   expr)) {
      if (e->PLUS() != nullptr) {
        n->kind = ExprNode::ExprKind::kAdd;
      } else if (e->MINUS() != nullptr) {
        n->kind = ExprNode::ExprKind::kSub;
      } else {
        assert(false && "unhandled operator in AdditiveExpressionContext");
      }
      n->lhs = buildExprTreeImpl(e->expression(0), param_idx, const_vals);
      n->rhs = buildExprTreeImpl(e->expression(1), param_idx, const_vals);

      // expression op=BitshiftOperator expression
    } else if (auto* e = dynamic_cast<qasm3Parser::BitshiftExpressionContext*>(
                   expr)) {
      if (e->op->getText() == "<<") {
        n->kind = ExprNode::ExprKind::kLShift;
      } else if (e->op->getText() == ">>") {
        n->kind = ExprNode::ExprKind::kRShift;
      } else {
        assert(false && "unhandled operator in BitshiftExpressionContext");
      }

      n->lhs = buildExprTreeImpl(e->expression(0), param_idx, const_vals);
      n->rhs = buildExprTreeImpl(e->expression(1), param_idx, const_vals);

      // expression op=ComparisonOperator expression
    } else if (auto* e =
                   dynamic_cast<qasm3Parser::ComparisonExpressionContext*>(
                       expr)) {
      if (e->op->getText() == ">") {
        n->kind = ExprNode::ExprKind::kGt;
      } else if (e->op->getText() == "<") {
        n->kind = ExprNode::ExprKind::kLt;
      } else if (e->op->getText() == ">=") {
        n->kind = ExprNode::ExprKind::kGe;
      } else if (e->op->getText() == "<=") {
        n->kind = ExprNode::ExprKind::kLe;
      } else {
        assert(false && "unhandled operator in ComparisonExpressionContext");
      }

      n->lhs = buildExprTreeImpl(e->expression(0), param_idx, const_vals);
      n->rhs = buildExprTreeImpl(e->expression(1), param_idx, const_vals);
      // expression op=EqualityOperator expression
    } else if (auto* e = dynamic_cast<qasm3Parser::EqualityExpressionContext*>(
                   expr)) {
      if (e->op->getText() == "==") {
        n->kind = ExprNode::ExprKind::kEq;
      } else if (e->op->getText() == "!=") {
        n->kind = ExprNode::ExprKind::kNe;
      } else {
        assert(false && "unhandled operator in EqualityExpressionContext");
      }

      n->lhs = buildExprTreeImpl(e->expression(0), param_idx, const_vals);
      n->rhs = buildExprTreeImpl(e->expression(1), param_idx, const_vals);

      // expression op=AMPERSAND expression
    } else if (auto* e =
                   dynamic_cast<qasm3Parser::BitwiseAndExpressionContext*>(
                       expr)) {
      n->kind = ExprNode::ExprKind::kBitAnd;
      n->lhs = buildExprTreeImpl(e->expression(0), param_idx, const_vals);
      n->rhs = buildExprTreeImpl(e->expression(1), param_idx, const_vals);

      // expression op=CARET expression
    } else if (auto* e =
                   dynamic_cast<qasm3Parser::BitwiseXorExpressionContext*>(
                       expr)) {
      n->kind = ExprNode::ExprKind::kBitXor;
      n->lhs = buildExprTreeImpl(e->expression(0), param_idx, const_vals);
      n->rhs = buildExprTreeImpl(e->expression(1), param_idx, const_vals);

      // expression op=PIPE expression
    } else if (auto* e = dynamic_cast<qasm3Parser::BitwiseOrExpressionContext*>(
                   expr)) {
      n->kind = ExprNode::ExprKind::kBitOr;
      n->lhs = buildExprTreeImpl(e->expression(0), param_idx, const_vals);
      n->rhs = buildExprTreeImpl(e->expression(1), param_idx, const_vals);

      // expression op=DOUBLE_AMPERSAND expression
    } else if (auto* e =
                   dynamic_cast<qasm3Parser::LogicalAndExpressionContext*>(
                       expr)) {
      n->kind = ExprNode::ExprKind::kLogAnd;
      n->lhs = buildExprTreeImpl(e->expression(0), param_idx, const_vals);
      n->rhs = buildExprTreeImpl(e->expression(1), param_idx, const_vals);

      // expression op=DOUBLE_PIPE expression
    } else if (auto* e = dynamic_cast<qasm3Parser::LogicalOrExpressionContext*>(
                   expr)) {
      n->kind = ExprNode::ExprKind::kLogOr;
      n->lhs = buildExprTreeImpl(e->expression(0), param_idx, const_vals);
      n->rhs = buildExprTreeImpl(e->expression(1), param_idx, const_vals);

      // (scalarType | arrayType) LPAREN expression RPAREN
    } else if (auto* e =
                   dynamic_cast<qasm3Parser::CastExpressionContext*>(expr)) {
      if (e->scalarType() != nullptr) {
        n = buildExprTreeImpl(e->expression(), param_idx, const_vals);
        if ((e->scalarType()->INT() != nullptr) ||
            (e->scalarType()->UINT() != nullptr)) {
          auto inner = n;
          n = std::make_shared<ExprNode>();
          n->kind = ExprNode::ExprKind::kFloor;
          n->lhs = inner;
        }
      } else {
        // Array casts are handled by visitClassicalDeclarationStatement
        // before buildExprTree is called. Reaching here is a parser bug.
        assert(false && "array cast reached buildExprTree");
      }

      // DURATIONOF LPAREN scope RPAREN
    } else if (auto* e =
                   dynamic_cast<qasm3Parser::DurationofExpressionContext*>(
                       expr)) {
      // TODO fallback to 0.0

      // Identifier LPAREN expressionList? RPAREN
    } else if (auto* e =
                   dynamic_cast<qasm3Parser::CallExpressionContext*>(expr)) {
      static const std::unordered_map<std::string, ExprNode::ExprKind> kFnMap =
          {
              {"sin", ExprNode::ExprKind::kSin},
              {"cos", ExprNode::ExprKind::kCos},
              {"tan", ExprNode::ExprKind::kTan},
              {"exp", ExprNode::ExprKind::kExp},
              {"sqrt", ExprNode::ExprKind::kSqrt},
              {"ln", ExprNode::ExprKind::kLn},
              {"arcsin", ExprNode::ExprKind::kArcsin},
              {"arccos", ExprNode::ExprKind::kArccos},
              {"arctan", ExprNode::ExprKind::kArctan},
          };
      auto it = kFnMap.find(e->Identifier()->getText());
      if (it != kFnMap.end() && (e->expressionList() != nullptr) &&
          !e->expressionList()->expression().empty()) {
        n->kind = it->second;
        n->lhs = buildExprTreeImpl(e->expressionList()->expression(0),
                                   param_idx, const_vals);
      } else {
        addError("cannot evaluate call to '" + e->Identifier()->getText() +
                     "' at parse time",
                 expr);
      }

      // Identifier | BinaryIntegerLiteral | OctalIntegerLiteral |
      // DecimalIntegerLiteral | HexIntegerLiteral | FloatLiteral |
      // ImaginaryLiteral | BooleanLiteral | BitstringLiteral |
      // TimingLiteral | HardwareQubit
    } else if (auto* e =
                   dynamic_cast<qasm3Parser::LiteralExpressionContext*>(expr)) {
      n->kind = ExprNode::ExprKind::kLiteral;
      // NOLINTBEGIN(bugprone-branch-clone)
      if (e->DecimalIntegerLiteral() != nullptr) {
        n->value = std::stod(e->DecimalIntegerLiteral()->getText());
      } else if (e->FloatLiteral() != nullptr) {
        n->value = std::stod(e->FloatLiteral()->getText());
      } else if (e->HexIntegerLiteral() != nullptr) {
        n->value = static_cast<double>(
            std::stoul(e->HexIntegerLiteral()->getText(), nullptr, 16));
      } else if (e->OctalIntegerLiteral() != nullptr) {
        std::string s = e->OctalIntegerLiteral()->getText();
        if (s.size() > 2 && s[1] == 'o') {
          s = s.substr(2);
        }
        n->value = static_cast<double>(std::stoul(s, nullptr, 8));
      } else if (e->BinaryIntegerLiteral() != nullptr) {
        std::string s = e->BinaryIntegerLiteral()->getText();
        n->value = static_cast<double>(std::stoul(s.substr(2), nullptr, 2));
      } else if (e->ImaginaryLiteral() != nullptr) {
        n->value = 0.0;  // real part of a purely imaginary number is 0
      } else if (e->TimingLiteral() != nullptr) {
        n->value = parseTimingLiteral(e->TimingLiteral()->getText(), dtNs);
      } else if (e->BooleanLiteral() != nullptr) {
        n->value = e->BooleanLiteral()->getText() == "true" ? 1.0 : 0.0;
      } else if (e->BitstringLiteral() != nullptr) {
        std::string bs = e->BitstringLiteral()->getText();
        bs = bs.substr(1, bs.size() - 2);  // strip surrounding quotes
        n->value = static_cast<double>(std::stoul(bs, nullptr, 2));
      } else if (e->HardwareQubit() != nullptr) {
        n->value = 0.0;  // hardware qubit refs not meaningful as doubles
      } else if (e->Identifier() != nullptr) {
        std::string id = e->Identifier()->getText();
        auto pit = param_idx.find(id);
        if (pit != param_idx.end()) {
          n->kind = ExprNode::ExprKind::kParamRef;
          n->param_idx = pit->second;
        } else {
          auto cit = const_vals.find(id);
          if (cit != const_vals.end()) {
            n->value = cit->second;
          } else if (id == "pi") {
            n->value = kPi;
          } else if (id == "tau") {
            n->value = kTau;
          } else if (id == "euler") {
            n->value = kEuler;
          } else {
            {
              addError(errUndefined(UndefinedKind::kVariable, id), expr);
            }
          }
        }
      } else {
        assert(false && "unhandled literal type in LiteralExpressionContext");
      }
      // NOLINTEND(bugprone-branch-clone)
    } else {
      [[maybe_unused]] const std::string msg =
          "unhandled expression type: " + std::string(typeid(*expr).name()) +
          " at line " + std::to_string(expr->getStart()->getLine()) + ":" +
          std::to_string(expr->getStart()->getCharPositionInLine());
      assert(false && msg.c_str());
    }

    return n;
  }

  // Evaluates expr as std::complex<double>.
  // ImaginaryLiteral (e.g. 2.5im) yields C{0.0, 2.5}; all other
  // expression types without complex semantics delegate to buildExprTree.
  C evalC(qasm3Parser::ExpressionContext* expr) {
    if (auto* e = dynamic_cast<qasm3Parser::LiteralExpressionContext*>(expr)) {
      if (e->ImaginaryLiteral() != nullptr) {
        const std::string t = e->ImaginaryLiteral()->getText();
        return {0.0, std::stod(t.substr(0, t.size() - 2))};
      }
      if (e->Identifier() != nullptr) {
        const std::string id = e->Identifier()->getText();
        if (const C* v = lookupScalar(id)) {
          return *v;
        }
        if (id == "pi") {
          return {kPi, 0.0};
        }
        if (id == "tau") {
          return {kTau, 0.0};
        }
        if (id == "euler") {
          return {kEuler, 0.0};
        }
        return {0.0, 0.0};
      }
      return {ExprNode::eval(buildExprTree(expr), {}), 0.0};
    }
    if (auto* e =
            dynamic_cast<qasm3Parser::ParenthesisExpressionContext*>(expr)) {
      return evalC(e->expression());
    }
    if (auto* e = dynamic_cast<qasm3Parser::UnaryExpressionContext*>(expr)) {
      if (e->MINUS() != nullptr) {
        return -evalC(e->expression());
      }
      return evalC(e->expression());
    }
    if (auto* e = dynamic_cast<qasm3Parser::AdditiveExpressionContext*>(expr)) {
      const C l = evalC(e->expression(0));
      const C r = evalC(e->expression(1));
      return (e->PLUS() != nullptr) ? l + r : l - r;
    }
    if (auto* e =
            dynamic_cast<qasm3Parser::MultiplicativeExpressionContext*>(expr)) {
      const C l = evalC(e->expression(0));
      const C r = evalC(e->expression(1));
      if (e->ASTERISK() != nullptr) {
        return l * r;
      }
      if (e->SLASH() != nullptr) {
        return r != C{0.0, 0.0} ? l / r : C{0.0, 0.0};
      }
      return {std::fmod(l.real(), r.real()), 0.0};
    }
    if (auto* e = dynamic_cast<qasm3Parser::PowerExpressionContext*>(expr)) {
      return std::pow(evalC(e->expression(0)), evalC(e->expression(1)));
    }
    if (auto* e = dynamic_cast<qasm3Parser::CallExpressionContext*>(expr)) {
      if ((e->expressionList() != nullptr) &&
          !e->expressionList()->expression().empty()) {
        const C arg = evalC(e->expressionList()->expression(0));
        const std::string fn = e->Identifier()->getText();
        if (fn == "sin") {
          return std::sin(arg);
        }
        if (fn == "cos") {
          return std::cos(arg);
        }
        if (fn == "tan") {
          return std::tan(arg);
        }
        if (fn == "exp") {
          return std::exp(arg);
        }
        if (fn == "sqrt") {
          return std::sqrt(arg);
        }
        if (fn == "ln") {
          return std::log(arg);
        }
        if (fn == "arcsin") {
          return std::asin(arg);
        }
        if (fn == "arccos") {
          return std::acos(arg);
        }
        if (fn == "arctan") {
          return std::atan(arg);
        }
      }
    }
    return {ExprNode::eval(buildExprTree(expr), {}), 0.0};
  }
};

}  // namespace

// ---------------------------------------------------------------------------
// Parser::parse
//
// Builds a Circuit from OpenQASM 3.0 source using the given backend config.
// Returns a failed ParseResult on syntax or semantic errors.
// ---------------------------------------------------------------------------

ParseResult Parser::parse(const std::string& source,
                          const BackendConfig& config) {
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
    return ParseResult::fail(listener.errors());
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
