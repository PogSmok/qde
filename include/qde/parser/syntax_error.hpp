#ifndef PARSER_SYNTAX_ERROR_HPP_
#define PARSER_SYNTAX_ERROR_HPP_

#include <cstddef>
#include <string>

namespace qde::parser {

// A syntax or semantic error found while parsing an OpenQASM program.
struct SyntaxError {
  std::size_t line;    // 1-indexed
  std::size_t column;  // 0-indexed
  std::string message;
};

}  // namespace qde::parser

#endif  // PARSER_SYNTAX_ERROR_HPP_
