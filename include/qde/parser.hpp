#ifndef PARSER_HPP_
#define PARSER_HPP_

#include <string>

#include "qde/parse_result.hpp"

namespace qde {

class Parser {
public:
  ParseResult parse(const std::string& source) const;

};

} // namespace qde

#endif // PARSER_HPP_