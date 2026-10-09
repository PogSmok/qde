#include "qde/simulator/internal/memory_guard.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace qde::simulator::internal {

std::uint64_t PhysicalMemoryBytes() noexcept {
#ifdef _WIN32
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  return GlobalMemoryStatusEx(&status) != 0 ? status.ullTotalPhys : 0;
#elif defined(_SC_PHYS_PAGES) && defined(_SC_PAGE_SIZE)
  const auto pages = sysconf(_SC_PHYS_PAGES);
  const auto page_size = sysconf(_SC_PAGE_SIZE);
  if (pages <= 0 || page_size <= 0) {
    return 0;
  }
  return SaturatingMul(static_cast<std::uint64_t>(pages),
                       static_cast<std::uint64_t>(page_size));
#else
  return 0;
#endif
}

bool FitsInMemory(std::size_t index_bits, std::uint64_t bytes_per_element,
                  std::uint64_t memory_limit) noexcept {
  if (index_bits > kMaxIndexBits) {
    return false;
  }
  const std::uint64_t bytes =
      SaturatingMul(std::uint64_t{1} << index_bits, bytes_per_element);
  // A saturated byte count has overflowed, here or in the caller's arithmetic.
  return bytes != std::numeric_limits<std::uint64_t>::max() &&
         bytes <= std::numeric_limits<std::size_t>::max() &&
         (memory_limit == 0 || bytes <= memory_limit);
}

void RequireMemory(std::size_t index_bits, std::uint64_t bytes_per_element,
                   const std::string& what) {
  if (!FitsInMemory(index_bits, bytes_per_element, PhysicalMemoryBytes())) {
    throw std::runtime_error("Simulator: " + what + " does not fit in memory");
  }
}

}  // namespace qde::simulator::internal
