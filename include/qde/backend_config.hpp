#ifndef BACKEND_CONFIG_HPP_
#define BACKEND_CONFIG_HPP_

#include <chrono>
#include <stdexcept>

namespace qde {

// Backend-specific parameters supplied by the user/GUI.
// Used by the parser to resolve hardware-dependent constructs, and by the
// simulator to model realistic noise and timing.
class BackendConfig {
 public:
  BackendConfig() = default;

  [[nodiscard]] std::chrono::nanoseconds DeviceCycleTimeNs() const noexcept {
    return device_cycle_time_ns_;
  }

  void SetDeviceCycleTimeNs(std::chrono::nanoseconds val) {
    if (val < std::chrono::nanoseconds{0}) {
      throw std::invalid_argument(
          "BackendConfig: device_cycle_time_ns must be >= 0");
    }
    device_cycle_time_ns_ = val;
  }

  void ResetDeviceCycleTimeNs() {
    device_cycle_time_ns_ = kDefaultDeviceCycleTimeNs;
  }

 private:
  // no timing modelling
  static constexpr std::chrono::nanoseconds kDefaultDeviceCycleTimeNs{0};
  std::chrono::nanoseconds device_cycle_time_ns_ = kDefaultDeviceCycleTimeNs;
};

}  // namespace qde

#endif  // BACKEND_CONFIG_HPP_
