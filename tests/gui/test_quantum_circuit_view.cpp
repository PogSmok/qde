#include <gtest/gtest.h>

#include <utility>
#include <vector>

#include "qde/backend_config.hpp"
#include "qde/circuit.hpp"
#include "qde/gate_registry.hpp"
#include "qde/gui/quantum_circuit_view.hpp"
#include "qde/gui/theme.hpp"
#include "qde/operation.hpp"
#include "qde/parser.hpp"

namespace qde::gui {

namespace {

QSize ExpectedSizeHint(int wire_count, int op_count,
                       int margin_up = theme::kDefaultMargin,
                       int margin_down = theme::kDefaultMargin,
                       int margin_left = theme::kDefaultMargin,
                       int margin_right = theme::kDefaultMargin) {
  const int width = theme::kLabelWidth + margin_left + margin_right +
                    (op_count + 1) * theme::kCellWidth;
  const int height = margin_up + margin_down + wire_count * theme::kCellHeight;
  return {width, height};
}

Circuit MakeCircuit(std::vector<QubitRegister> qregs,
                    std::vector<BitRegister> bregs,
                    std::vector<Operation> ops) {
  return {GateRegistry::WithBuiltins(), std::move(qregs), std::move(bregs),
          std::move(ops)};
}

Operation HadamardOn(GateRegistry& reg, std::uint8_t qubit) {
  return {OperationType::kGate, reg.Find("h"), {}, {{0, qubit}}, {}};
}

Operation PauliXOn(GateRegistry& reg, std::uint8_t qubit) {
  return {OperationType::kGate, reg.Find("x"), {}, {{0, qubit}}, {}};
}

}  // namespace

class QuantumCircuitViewTest : public ::testing::Test {
 protected:
  void SetUp() override { view_ = new QuantumCircuitView(); }

  void TearDown() override { delete view_; }

  GateRegistry registry_ = GateRegistry::WithBuiltins();
  QuantumCircuitView* view_{};
};

TEST_F(QuantumCircuitViewTest, SizeHintDefaultNoCircuit) {
  EXPECT_EQ(view_->sizeHint(),
            QSize(theme::kDefaultWidth, theme::kDefaultHeight));
}

TEST_F(QuantumCircuitViewTest, SizeHintScalesWithWiresAndOps) {
  const Circuit circuit =
      MakeCircuit({{"q", 2}}, {},
                  {HadamardOn(registry_, 0), PauliXOn(registry_, 1),
                   HadamardOn(registry_, 0)});

  view_->RenderCircuit(circuit);

  EXPECT_EQ(view_->sizeHint(),
            ExpectedSizeHint(/*wire_count=*/2, /*op_count=*/3));
}

TEST_F(QuantumCircuitViewTest, SizeHintHeightScalesWithWireCount) {
  const Circuit two_qubits =
      MakeCircuit({{"q", 2}}, {}, {HadamardOn(registry_, 0)});
  const Circuit three_qubits =
      MakeCircuit({{"q", 3}}, {}, {HadamardOn(registry_, 0)});

  view_->RenderCircuit(two_qubits);
  const QSize two_wire_hint = view_->sizeHint();

  view_->RenderCircuit(three_qubits);
  const QSize three_wire_hint = view_->sizeHint();

  EXPECT_EQ(two_wire_hint.width(), three_wire_hint.width());
  EXPECT_LT(two_wire_hint.height(), three_wire_hint.height());
  EXPECT_EQ(three_wire_hint.height() - two_wire_hint.height(),
            theme::kCellHeight);
}

TEST_F(QuantumCircuitViewTest, SizeHintWidthScalesWithOpCount) {
  const Circuit one_op =
      MakeCircuit({{"q", 1}}, {}, {HadamardOn(registry_, 0)});
  const Circuit three_ops =
      MakeCircuit({{"q", 1}}, {},
                  {HadamardOn(registry_, 0), PauliXOn(registry_, 0),
                   HadamardOn(registry_, 0)});

  view_->RenderCircuit(one_op);
  const QSize one_op_hint = view_->sizeHint();

  view_->RenderCircuit(three_ops);
  const QSize three_op_hint = view_->sizeHint();

  EXPECT_EQ(one_op_hint.height(), three_op_hint.height());
  EXPECT_LT(one_op_hint.width(), three_op_hint.width());
  EXPECT_EQ(three_op_hint.width() - one_op_hint.width(), 2 * theme::kCellWidth);
}

TEST_F(QuantumCircuitViewTest, RenderCircuitResizesWidget) {
  const Circuit circuit =
      MakeCircuit({{"q", 2}}, {}, {HadamardOn(registry_, 0)});

  view_->RenderCircuit(circuit);

  EXPECT_EQ(view_->size(), view_->sizeHint());
}

TEST_F(QuantumCircuitViewTest, SetMarginsAffectsSizeHint) {
  const Circuit circuit =
      MakeCircuit({{"q", 1}}, {}, {HadamardOn(registry_, 0)});

  view_->SetMargins(20, 20, 30, 40);
  view_->RenderCircuit(circuit);

  EXPECT_EQ(view_->sizeHint(),
            ExpectedSizeHint(/*wire_count=*/1, /*op_count=*/1,
                             /*margin_up=*/20, /*margin_down=*/20,
                             /*margin_left=*/30,
                             /*margin_right=*/40));
}

TEST_F(QuantumCircuitViewTest, RenderParsedCircuit) {
  const auto result = Parser::Parse(
      "OPENQASM 3.0;\nqreg q[2];\nh q[0];\nx q[1];", BackendConfig{});

  ASSERT_TRUE(result.IsOk());
  view_->RenderCircuit(result.GetCircuit());

  EXPECT_EQ(view_->sizeHint(),
            ExpectedSizeHint(/*wire_count=*/2, /*op_count=*/2));
  EXPECT_EQ(view_->size(), view_->sizeHint());
}

TEST_F(QuantumCircuitViewTest, ClearCircuitResetsPlaceholderSizeHint) {
  const Circuit circuit =
      MakeCircuit({{"q", 2}}, {}, {HadamardOn(registry_, 0)});

  view_->RenderCircuit(circuit);
  ASSERT_NE(view_->sizeHint(),
            QSize(theme::kDefaultWidth, theme::kDefaultHeight));

  view_->ClearCircuit();

  EXPECT_EQ(view_->sizeHint(),
            QSize(theme::kDefaultWidth, theme::kDefaultHeight));
  EXPECT_EQ(view_->size(), view_->sizeHint());
}

}  // namespace qde::gui
