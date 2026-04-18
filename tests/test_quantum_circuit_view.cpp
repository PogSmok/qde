#include <gtest/gtest.h>
#include "qde/qt/quantum_circuit_view.hpp"

class QuantumCircuitViewTest : public ::testing::Test {
protected:
 void SetUp() override {
  view = new QuantumCircuitView();
 }

 void TearDown() override {
  delete view;
 }

 QuantumCircuitView* view;
};

TEST_F(QuantumCircuitViewTest, Instantiation) {
 // Currently WIP, just testing that it constructs and destructs properly
 EXPECT_NE(view, nullptr);
}
