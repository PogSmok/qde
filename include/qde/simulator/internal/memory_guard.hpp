#ifndef SIMULATOR_INTERNAL_MEMORY_GUARD_HPP_
#define SIMULATOR_INTERNAL_MEMORY_GUARD_HPP_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

namespace qde::simulator::internal {

// Largest number of index bits a simulator buffer may have. Element indices
// must fit both std::size_t and the signed 64-bit counters of the parallel
// loops.
inline constexpr std::size_t kMaxIndexBits =
    std::min<std::size_t>(std::numeric_limits<std::size_t>::digits, 63) - 1;

// a * b, clamped to the largest representable value instead of wrapping.
[[nodiscard]] constexpr std::uint64_t SaturatingMul(std::uint64_t a,
                                                    std::uint64_t b) noexcept {
  constexpr std::uint64_t max = std::numeric_limits<std::uint64_t>::max();
  return (b != 0 && a > max / b) ? max : a * b;
}

// a + b, clamped to the largest representable value instead of wrapping.
[[nodiscard]] constexpr std::uint64_t SaturatingAdd(std::uint64_t a,
                                                    std::uint64_t b) noexcept {
  constexpr std::uint64_t max = std::numeric_limits<std::uint64_t>::max();
  return a > max - b ? max : a + b;
}

// Installed physical memory in bytes, or 0 if it cannot be determined.
[[nodiscard]] std::uint64_t PhysicalMemoryBytes() noexcept;

// Whether a buffer of 2^index_bits elements of bytes_per_element bytes each
// can be indexed without overflow and occupies at most memory_limit bytes.
// A memory_limit of 0 means the limit is unknown and is not enforced.
[[nodiscard]] bool FitsInMemory(std::size_t index_bits,
                                std::uint64_t bytes_per_element,
                                std::uint64_t memory_limit) noexcept;

// Throws std::runtime_error naming `what` unless FitsInMemory holds for the
// installed physical memory.
void RequireMemory(std::size_t index_bits, std::uint64_t bytes_per_element,
                   const std::string& what);

}  // namespace qde::simulator::internal

#endif  // SIMULATOR_INTERNAL_MEMORY_GUARD_HPP_
