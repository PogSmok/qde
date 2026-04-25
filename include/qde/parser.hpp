#ifndef PARSER_HPP_
#define PARSER_HPP_

#include <string>

#include "qde/backend_config.hpp"
#include "qde/parse_result.hpp"

namespace qde {

class Parser {
 public:
  [[nodiscard]] ParseResult parse(const std::string& source,
                                  const BackendConfig& config) const;
};

}  // namespace qde

#endif  // PARSER_HPP_