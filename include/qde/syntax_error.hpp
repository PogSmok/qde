#ifndef SYNTAX_ERROR_HPP_
#define SYNTAX_ERROR_HPP_

#include <string>
#include <cstddef>

namespace qde {

struct SyntaxError {
  std::size_t line;
  std::size_t idx;
  std::string message;
};

} // namespace qde

#endif // SYNTAX_ERROR_HPP_