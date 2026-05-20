#include <gtest/gtest.h>

#include "qde/circuit.hpp"
#include "qde/gate_registry.hpp"
#include "qde/qubit_allocator.hpp"

namespace qde {

namespace {
Circuit MakeCircuit(std::vector<QubitRegister> qregs) {
  return {GateRegistry::WithBuiltins(), std::move(qregs), {}, {}};
}
}  // namespace

TEST(QubitAllocatorTest, EmptyCircuitProducesEmptyMapping) {
  const auto mapping = QubitAllocator::Allocate(MakeCircuit({}));
  EXPECT_TRUE(mapping.empty());
}

TEST(QubitAllocatorTest, SingleRegisterMapsCorrectly) {
  const auto mapping = QubitAllocator::Allocate(MakeCircuit({{"q", 3}}));
  ASSERT_EQ(mapping.size(), 3U);
  EXPECT_EQ(mapping.at("q[0]"), 0U);
  EXPECT_EQ(mapping.at("q[1]"), 1U);
  EXPECT_EQ(mapping.at("q[2]"), 2U);
}

TEST(QubitAllocatorTest, MultipleRegistersAreFlattened) {
  const auto mapping =
      QubitAllocator::Allocate(MakeCircuit({{"a", 2}, {"b", 2}}));
  ASSERT_EQ(mapping.size(), 4U);
  EXPECT_EQ(mapping.at("a[0]"), 0U);
  EXPECT_EQ(mapping.at("a[1]"), 1U);
  EXPECT_EQ(mapping.at("b[0]"), 2U);
  EXPECT_EQ(mapping.at("b[1]"), 3U);
}

}  // namespace qde
