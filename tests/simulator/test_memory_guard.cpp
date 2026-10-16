#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <stdexcept>

#include "qde/simulator/internal/memory_guard.hpp"

namespace qde::simulator::internal {

namespace {

constexpr std::uint64_t kMax = std::numeric_limits<std::uint64_t>::max();
constexpr std::uint64_t kGiB = std::uint64_t{1} << 30;

}  // namespace

TEST(MemoryGuardTest, SaturatingMulMultipliesWhenRepresentable) {
  EXPECT_EQ(SaturatingMul(6, 7), 42U);
  EXPECT_EQ(SaturatingMul(0, kMax), 0U);
  EXPECT_EQ(SaturatingMul(kMax, 0), 0U);
  EXPECT_EQ(SaturatingMul(kMax, 1), kMax);
}

TEST(MemoryGuardTest, SaturatingMulClampsOnOverflow) {
  EXPECT_EQ(SaturatingMul(kMax, 2), kMax);
  EXPECT_EQ(SaturatingMul(std::uint64_t{1} << 32, std::uint64_t{1} << 32),
            kMax);
}

TEST(MemoryGuardTest, SaturatingAddClampsOnOverflow) {
  EXPECT_EQ(SaturatingAdd(40, 2), 42U);
  EXPECT_EQ(SaturatingAdd(kMax, 0), kMax);
  EXPECT_EQ(SaturatingAdd(kMax, 1), kMax);
  EXPECT_EQ(SaturatingAdd(kMax - 1, 5), kMax);
}

TEST(MemoryGuardTest, FitsWhenBelowLimit) {
  // 2^20 elements of 16 bytes = 16 MiB.
  EXPECT_TRUE(FitsInMemory(20, 16, kGiB));
  EXPECT_TRUE(FitsInMemory(26, 16, kGiB));  // exactly 1 GiB
  EXPECT_FALSE(FitsInMemory(27, 16, kGiB));
}

TEST(MemoryGuardTest, UnknownLimitIsNotEnforced) {
  EXPECT_TRUE(FitsInMemory(40, 16, 0));
}

TEST(MemoryGuardTest, RejectsIndicesThatCannotBeRepresented) {
  EXPECT_TRUE(FitsInMemory(kMaxIndexBits, 1, 0));
  EXPECT_FALSE(FitsInMemory(kMaxIndexBits + 1, 1, 0));
  EXPECT_FALSE(FitsInMemory(64, 1, 0));
  EXPECT_FALSE(FitsInMemory(1000, 0, 0));
}

TEST(MemoryGuardTest, RejectsByteCountsThatOverflow) {
  EXPECT_FALSE(FitsInMemory(kMaxIndexBits, kMax, 0));
  EXPECT_FALSE(FitsInMemory(60, 16, 0));  // 2^64 bytes
}

TEST(MemoryGuardTest, RequireMemoryAcceptsSmallBuffers) {
  EXPECT_NO_THROW(RequireMemory(10, 16, "a small buffer"));
}

TEST(MemoryGuardTest, RequireMemoryNamesWhatDoesNotFit) {
  try {
    RequireMemory(kMaxIndexBits + 1, 16, "the test buffer");
    FAIL() << "expected std::runtime_error";
  } catch (const std::runtime_error& error) {
    EXPECT_STREQ(error.what(),
                 "Simulator: the test buffer does not fit in memory");
  }
}

}  // namespace qde::simulator::internal
