#include "qde/parser/internal/frontend.hpp"

#include <exception>
#include <memory>
#include <string>

#include <antlr4-runtime.h>
#include <qasm3Lexer.h>
#include <qasm3Parser.h>

#include "qde/parser/internal/ast.hpp"
#include "qde/parser/internal/ast_builder.hpp"
#include "qde/parser/internal/diagnostics.hpp"
#include "qde/parser/internal/syntax_error_listener.hpp"

namespace qde::parser::internal {

std::shared_ptr<const ast::Program> ParseToAst(const std::string& source,
                                               Diagnostics& diagnostics) {
  try {
    SyntaxErrorListener listener;

    antlr4::ANTLRInputStream input(source);
    qasm3Lexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    qasm3Parser parser(&tokens);

    lexer.removeErrorListeners();  // Remove default std::cerr error listener
    lexer.addErrorListener(&listener);
    parser.removeErrorListeners();  // Remove default std::cerr error listener
    parser.addErrorListener(&listener);

    auto* tree = parser.program();

    if (listener.HasErrors()) {
      for (const SyntaxError& error : listener.Errors()) {
        diagnostics.Error({error.line, error.column}, error.message);
      }
      return nullptr;
    }

    const std::size_t errors_before = diagnostics.Count();
    auto program = AstBuilder(diagnostics).Build(tree);
    return diagnostics.Count() == errors_before ? program : nullptr;
  } catch (const std::exception& e) {
    diagnostics.Error({1, 0}, std::string("cannot read source: ") + e.what());
    return nullptr;
  }
}

}  // namespace qde::parser::internal
