#ifndef BACKEND_CONFIG_HPP_
#define BACKEND_CONFIG_HPP_

#include <stdexcept>

namespace qde {

// Backend-specific parameters supplied by the user/GUI.
// Used by the parser to resolve hardware-dependent constructs, and by the
// simulator to model realistic noise and timing.
class BackendConfig {
 public:
  BackendConfig() = default;

  [[nodiscard]] double dtNs() const noexcept { return dt_ns_; }

  void setDtNs(double dt_ns) {
    if (dt_ns < 0.0) {
      throw std::invalid_argument("BackendConfig: dt_ns must be >= 0");
    }
    dt_ns_ = dt_ns;
  }

 private:
  double dt_ns_ = 0.0;  // device cycle time in nanoseconds; 0 = unknown
};

}  // namespace qde

#endif  // BACKEND_CONFIG_HPP_
