#include <gtest/gtest.h>
#include "qde/gui/quantum_circuit_view.hpp"

namespace qde::gui {

class QuantumCircuitViewTest : public ::testing::Test {
 protected:
  void SetUp() override { view_ = new QuantumCircuitView(); }

  void TearDown() override { delete view_; }

  QuantumCircuitView* view_{};
};

TEST_F(QuantumCircuitViewTest, Instantiation) {
  // Currently WIP, just testing that it constructs and destructs properly
  EXPECT_NE(view_, nullptr);
}

}  // namespace qde::gui