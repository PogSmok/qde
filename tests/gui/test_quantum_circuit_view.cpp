#include <gtest/gtest.h>
#include "qde/gui/quantum_circuit_view.hpp"

namespace qde::gui {

class QuantumCircuitViewTest : public ::testing::Test {
 protected:
  void SetUp() override { view = new QuantumCircuitView(); }

  void TearDown() override { delete view; }

  QuantumCircuitView* view{};
};

TEST_F(QuantumCircuitViewTest, Instantiation) {
  // Currently WIP, just testing that it constructs and destructs properly
  EXPECT_NE(view, nullptr);
}

}  // namespace qde::gui