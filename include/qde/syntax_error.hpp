#ifndef SYNTAX_ERROR_HPP_
#define SYNTAX_ERROR_HPP_

#include <cstddef>
#include <string>

namespace qde {

struct SyntaxError {
  std::size_t line;
  std::size_t column;
  std::string message;
};

} // namespace qde

#endif // SYNTAX_ERROR_HPP_