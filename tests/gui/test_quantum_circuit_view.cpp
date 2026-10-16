#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QRect>
#include <QString>
#include <QWidget>

#include "qde/backend_config.hpp"
#include "qde/circuit.hpp"
#include "qde/gate_registry.hpp"
#include "qde/gui/quantum_circuit_view.hpp"
#include "qde/gui/theme.hpp"
#include "qde/gui/view_helpers.hpp"
#include "qde/operation.hpp"
#include "qde/parser.hpp"

namespace qde::gui {

namespace {

constexpr int kGateLabelHPadding = 6;
constexpr double kPi = 3.141592653589793238462643383;
constexpr double kWideAngle = 12.345;

int ColumnWidth(int box_width) {
  const int gap = std::max(0, theme::kCellWidth - theme::kGateSize);
  return std::max(theme::kCellWidth, box_width + gap);
}

int LabelColumnWidth(const std::vector<QString>& labels) {
  const QFont font(theme::kWireLabelFontFamily, theme::kWireLabelFontSize);
  const QFontMetrics metrics(font);
  const int padding =
      metrics.horizontalAdvance(QStringLiteral(" ")) * theme::kLabelPadding;
  int width = theme::kLabelWidth;
  for (const QString& label : labels) {
    width = std::max(width, metrics.horizontalAdvance(label) + (2 * padding));
  }
  return width;
}

int LabelBoxWidth(const QString& label) {
  const QFont font(theme::kWireLabelFontFamily, theme::kWireLabelFontSize);
  const QFontMetrics metrics(font);
  const int text_width =
      metrics.boundingRect(QRect(), Qt::AlignLeft, label).width();
  return std::max(theme::kGateSize, text_width + (2 * kGateLabelHPadding));
}

bool IsGlyphGate(const std::string& name) {
  return name == "swap" || name == "cswap" || name == "CX" || name == "cx" ||
         name == "cz" || name == "ccx";
}

QString GateText(const std::string& name, const std::vector<double>& params) {
  QString text = QString::fromStdString(name).toUpper();
  if (params.empty()) {
    return text;
  }
  QString formatted = FormatAngle(params[0]);
  for (std::size_t i = 1; i < params.size(); ++i) {
    formatted += QStringLiteral(", ") + FormatAngle(params[i]);
  }
  return text + QStringLiteral("\n(") + formatted + QStringLiteral(")");
}

int GateBoxWidth(const std::string& name, const std::vector<double>& params,
                 int qubit_count) {
  if (qubit_count != 1 && IsGlyphGate(name)) {
    return theme::kGateSize;
  }
  return LabelBoxWidth(GateText(name, params));
}

std::vector<QString> WireLabels(const std::string& name, int count) {
  std::vector<QString> labels;
  labels.reserve(static_cast<std::size_t>(count));
  for (int i = 0; i < count; ++i) {
    labels.push_back(QString::fromStdString(name) + QStringLiteral("[") +
                     QString::number(i) + QStringLiteral("]"));
  }
  return labels;
}

QSize ExpectedSizeHint(const std::vector<QString>& wire_labels,
                       const std::vector<int>& column_box_widths,
                       int margin_up = theme::kDefaultMargin,
                       int margin_down = theme::kDefaultMargin,
                       int margin_left = theme::kDefaultMargin,
                       int margin_right = theme::kDefaultMargin) {
  int grid_width = theme::kCellWidth;
  for (const int box_width : column_box_widths) {
    grid_width += ColumnWidth(box_width);
  }
  const int width =
      LabelColumnWidth(wire_labels) + margin_left + margin_right + grid_width;
  const int height =
      margin_up + margin_down +
      ((static_cast<int>(wire_labels.size()) + 1) * theme::kCellHeight);
  return {width, height};
}

Circuit MakeCircuit(std::vector<QubitRegister> qregs,
                    std::vector<BitRegister> bregs,
                    std::vector<Operation> ops) {
  return {GateRegistry::WithBuiltins(), std::move(qregs), std::move(bregs),
          std::move(ops)};
}

Operation HadamardOn(GateRegistry& reg, std::uint8_t qubit) {
  return {OperationType::kGate, reg.Find("h"), {}, {{0, qubit}}, {},
          std::nullopt};
}

Operation PauliXOn(GateRegistry& reg, std::uint8_t qubit) {
  return {OperationType::kGate, reg.Find("x"), {}, {{0, qubit}}, {},
          std::nullopt};
}

Operation RotationOn(GateRegistry& reg, double angle, std::uint8_t qubit) {
  return {OperationType::kGate, reg.Find("rx"), {angle}, {{0, qubit}}, {},
          std::nullopt};
}

Operation TwoQubitGate(GateRegistry& reg, const char* name,
                       std::uint8_t qubit_a, std::uint8_t qubit_b) {
  return {OperationType::kGate,
          reg.Find(name),
          {},
          {{0, qubit_a}, {0, qubit_b}},
          {},
          std::nullopt};
}

QImage RenderWidget(QWidget* widget) {
  widget->resize(widget->sizeHint());
  QImage image(widget->size(), QImage::Format_ARGB32);
  image.fill(Qt::black);
  widget->render(&image);
  return image;
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

  // X on q[1] shares column 0 with the first H. The second H starts column 1.
  const int column_0 =
      std::max(GateBoxWidth("h", {}, 1), GateBoxWidth("x", {}, 1));
  const int column_1 = GateBoxWidth("h", {}, 1);
  EXPECT_EQ(view_->sizeHint(),
            ExpectedSizeHint(WireLabels("q", 2), {column_0, column_1}));
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

  const int hadamard_column = ColumnWidth(GateBoxWidth("h", {}, 1));
  const int pauli_column = ColumnWidth(GateBoxWidth("x", {}, 1));
  EXPECT_EQ(one_op_hint.height(), three_op_hint.height());
  EXPECT_LT(one_op_hint.width(), three_op_hint.width());
  EXPECT_EQ(three_op_hint.width() - one_op_hint.width(),
            pauli_column + hadamard_column);
  EXPECT_EQ(three_op_hint,
            ExpectedSizeHint(WireLabels("q", 1), {GateBoxWidth("h", {}, 1),
                                                  GateBoxWidth("x", {}, 1),
                                                  GateBoxWidth("h", {}, 1)}));
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
            ExpectedSizeHint(WireLabels("q", 1), {GateBoxWidth("h", {}, 1)},
                             /*margin_up=*/20, /*margin_down=*/20,
                             /*margin_left=*/30,
                             /*margin_right=*/40));
}

TEST_F(QuantumCircuitViewTest, SetMarginsAfterRenderResizesWidget) {
  const Circuit circuit =
      MakeCircuit({{"q", 1}}, {}, {HadamardOn(registry_, 0)});

  view_->RenderCircuit(circuit);
  view_->SetMargins(20, 20, 30, 40);

  EXPECT_EQ(view_->size(),
            ExpectedSizeHint(WireLabels("q", 1), {GateBoxWidth("h", {}, 1)},
                             /*margin_up=*/20, /*margin_down=*/20,
                             /*margin_left=*/30,
                             /*margin_right=*/40));
}

TEST_F(QuantumCircuitViewTest, RenderParsedCircuit) {
  const auto result = Parser::Parse(
      "OPENQASM 3.0;\nqreg q[2];\nh q[0];\nx q[1];", BackendConfig{});

  ASSERT_TRUE(result.IsOk());
  view_->RenderCircuit(result.GetCircuit());

  const int column =
      std::max(GateBoxWidth("h", {}, 1), GateBoxWidth("x", {}, 1));
  EXPECT_EQ(view_->sizeHint(), ExpectedSizeHint(WireLabels("q", 2), {column}));
  EXPECT_EQ(view_->size(), view_->sizeHint());
}

TEST_F(QuantumCircuitViewTest, RenderAllOperationTypesProducesImage) {
  const Circuit circuit = MakeCircuit(
      {{"q", 2}, {"r", 2}}, {{"c", 2}},
      {
          {OperationType::kGate,
           registry_.Find("rx"),
           {kPi / 2},
           {{0, 0}},
           {},
           std::nullopt},
          {OperationType::kGate,
           registry_.Find("cx"),
           {},
           {{0, 0}, {0, 1}},
           {},
           std::nullopt},
          {OperationType::kGate,
           registry_.Find("cz"),
           {},
           {{0, 1}, {1, 0}},
           {},
           std::nullopt},
          {OperationType::kGate,
           registry_.Find("swap"),
           {},
           {{1, 0}, {1, 1}},
           {},
           std::nullopt},
          {OperationType::kGate,
           registry_.Find("ccx"),
           {},
           {{0, 0}, {0, 1}, {1, 0}},
           {},
           std::nullopt},
          {OperationType::kGate,
           registry_.Find("cswap"),
           {},
           {{0, 0}, {0, 1}, {1, 1}},
           {},
           std::nullopt},
          {OperationType::kMeasure,
           nullptr,
           {},
           {{0, 0}},
           {{0, 0}},
           std::nullopt},
          {OperationType::kReset, nullptr, {}, {{0, 1}}, {}, std::nullopt},
          {OperationType::kBarrier, nullptr, {}, {{1, 0}}, {}, std::nullopt},
      });

  view_->RenderCircuit(circuit);
  const QImage image = RenderWidget(view_);

  EXPECT_FALSE(image.isNull());
  EXPECT_EQ(image.size(), view_->size());
}

TEST_F(QuantumCircuitViewTest, RenderPlaceholderProducesImage) {
  const QImage image = RenderWidget(view_);

  EXPECT_FALSE(image.isNull());
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

TEST_F(QuantumCircuitViewTest, LongWireLabelWidensSizeHint) {
  const std::string long_name(32, 'n');
  const Circuit short_name =
      MakeCircuit({{"q", 1}}, {}, {HadamardOn(registry_, 0)});
  const Circuit long_label =
      MakeCircuit({{long_name, 1}}, {}, {HadamardOn(registry_, 0)});
  const std::vector<QString> short_labels = WireLabels("q", 1);
  const std::vector<QString> long_labels = WireLabels(long_name, 1);
  const int hadamard_box = GateBoxWidth("h", {}, 1);

  ASSERT_EQ(LabelColumnWidth(short_labels), theme::kLabelWidth);
  ASSERT_GT(LabelColumnWidth(long_labels), theme::kLabelWidth);

  view_->RenderCircuit(short_name);
  const QSize short_hint = view_->sizeHint();

  view_->RenderCircuit(long_label);
  const QSize long_hint = view_->sizeHint();

  EXPECT_EQ(long_hint.height(), short_hint.height());
  EXPECT_EQ(long_hint.width() - short_hint.width(),
            LabelColumnWidth(long_labels) - LabelColumnWidth(short_labels));
  EXPECT_EQ(long_hint, ExpectedSizeHint(long_labels, {hadamard_box}));
}

TEST_F(QuantumCircuitViewTest, ParallelGatesShareAColumn) {
  const Circuit parallel = MakeCircuit(
      {{"q", 2}}, {}, {HadamardOn(registry_, 0), PauliXOn(registry_, 1)});
  const Circuit single =
      MakeCircuit({{"q", 2}}, {}, {HadamardOn(registry_, 0)});
  const Circuit sequential = MakeCircuit(
      {{"q", 2}}, {}, {HadamardOn(registry_, 0), PauliXOn(registry_, 0)});

  view_->RenderCircuit(single);
  const QSize single_hint = view_->sizeHint();

  view_->RenderCircuit(parallel);
  EXPECT_EQ(view_->sizeHint().width(), single_hint.width());
  EXPECT_EQ(view_->sizeHint(),
            ExpectedSizeHint(WireLabels("q", 2),
                             {std::max(GateBoxWidth("h", {}, 1),
                                       GateBoxWidth("x", {}, 1))}));

  view_->RenderCircuit(sequential);
  EXPECT_GT(view_->sizeHint().width(), single_hint.width());
}

TEST_F(QuantumCircuitViewTest, ParameterizedGateWidensColumn) {
  const Circuit rotation =
      MakeCircuit({{"q", 1}}, {}, {RotationOn(registry_, kWideAngle, 0)});
  const Circuit hadamard =
      MakeCircuit({{"q", 1}}, {}, {HadamardOn(registry_, 0)});
  const int rotation_box = GateBoxWidth("rx", {kWideAngle}, 1);
  const int hadamard_box = GateBoxWidth("h", {}, 1);

  ASSERT_GT(ColumnWidth(rotation_box), ColumnWidth(hadamard_box));

  view_->RenderCircuit(rotation);
  const QSize rotation_hint = view_->sizeHint();
  EXPECT_EQ(rotation_hint,
            ExpectedSizeHint(WireLabels("q", 1), {rotation_box}));

  view_->RenderCircuit(hadamard);
  EXPECT_GT(rotation_hint.width(), view_->sizeHint().width());
  EXPECT_EQ(rotation_hint.height(), view_->sizeHint().height());
}

TEST_F(QuantumCircuitViewTest, GlyphGatesKeepMinimumColumnWidth) {
  const std::vector<QString> labels = WireLabels("q", 2);
  const Circuit controlled_not =
      MakeCircuit({{"q", 2}}, {}, {TwoQubitGate(registry_, "cx", 0, 1)});
  const Circuit swap =
      MakeCircuit({{"q", 2}}, {}, {TwoQubitGate(registry_, "swap", 0, 1)});

  EXPECT_EQ(ColumnWidth(theme::kGateSize), theme::kCellWidth);
  EXPECT_EQ(GateBoxWidth("cx", {}, 2), theme::kGateSize);
  EXPECT_EQ(GateBoxWidth("swap", {}, 2), theme::kGateSize);

  view_->RenderCircuit(controlled_not);
  EXPECT_EQ(view_->sizeHint(), ExpectedSizeHint(labels, {theme::kGateSize}));

  view_->RenderCircuit(swap);
  EXPECT_EQ(view_->sizeHint(), ExpectedSizeHint(labels, {theme::kGateSize}));
}

TEST_F(QuantumCircuitViewTest, ClassicalWireAddsHeight) {
  const Circuit qubits =
      MakeCircuit({{"q", 1}}, {}, {HadamardOn(registry_, 0)});
  const Circuit qubits_and_bits =
      MakeCircuit({{"q", 1}}, {{"c", 1}}, {HadamardOn(registry_, 0)});
  const int hadamard_box = GateBoxWidth("h", {}, 1);

  view_->RenderCircuit(qubits);
  const QSize qubit_hint = view_->sizeHint();

  view_->RenderCircuit(qubits_and_bits);
  const QSize both_hint = view_->sizeHint();

  EXPECT_EQ(both_hint.width(), qubit_hint.width());
  EXPECT_EQ(both_hint.height() - qubit_hint.height(), theme::kCellHeight);

  std::vector<QString> labels = WireLabels("q", 1);
  const std::vector<QString> bits = WireLabels("c", 1);
  labels.insert(labels.end(), bits.begin(), bits.end());
  EXPECT_EQ(both_hint, ExpectedSizeHint(labels, {hadamard_box}));
}

TEST_F(QuantumCircuitViewTest, DisplayCapLimitsQubitHeight) {
  const int capped = theme::kMaxDisplayedQubits;
  const Circuit shown = MakeCircuit({{"q", static_cast<std::size_t>(capped)}},
                                    {}, {HadamardOn(registry_, 0)});
  const Circuit extra =
      MakeCircuit({{"q", static_cast<std::size_t>(capped + 1)}}, {},
                  {HadamardOn(registry_, 0)});

  view_->RenderCircuit(shown);
  const int shown_height = view_->sizeHint().height();

  view_->RenderCircuit(extra);
  EXPECT_EQ(view_->sizeHint().height(), shown_height);
  EXPECT_EQ(view_->sizeHint(), ExpectedSizeHint(WireLabels("q", capped),
                                                {GateBoxWidth("h", {}, 1)}));
}

TEST_F(QuantumCircuitViewTest, DisplayCapLimitsBitHeight) {
  const int capped = theme::kMaxDisplayedBits;
  const Circuit shown =
      MakeCircuit({}, {{"c", static_cast<std::size_t>(capped)}}, {});
  const Circuit extra =
      MakeCircuit({}, {{"c", static_cast<std::size_t>(capped + 1)}}, {});

  view_->RenderCircuit(shown);
  const int shown_height = view_->sizeHint().height();

  view_->RenderCircuit(extra);
  EXPECT_EQ(view_->sizeHint().height(), shown_height);
  EXPECT_EQ(view_->sizeHint(), ExpectedSizeHint(WireLabels("c", capped), {}));
}

TEST_F(QuantumCircuitViewTest, HiddenQubitGateAddsNoColumn) {
  const int wires = theme::kMaxDisplayedQubits + 1;
  const auto hidden_qubit =
      static_cast<std::uint8_t>(theme::kMaxDisplayedQubits);
  const Circuit hidden_gate =
      MakeCircuit({{"q", static_cast<std::size_t>(wires)}}, {},
                  {HadamardOn(registry_, hidden_qubit)});
  const Circuit no_gate =
      MakeCircuit({{"q", static_cast<std::size_t>(wires)}}, {}, {});

  view_->RenderCircuit(no_gate);
  const QSize empty_hint = view_->sizeHint();

  view_->RenderCircuit(hidden_gate);
  EXPECT_EQ(view_->sizeHint(), empty_hint);
  EXPECT_EQ(view_->sizeHint(),
            ExpectedSizeHint(WireLabels("q", theme::kMaxDisplayedQubits), {}));

  const QImage image = RenderWidget(view_);
  EXPECT_FALSE(image.isNull());
  EXPECT_EQ(image.size(), view_->size());
}

TEST_F(QuantumCircuitViewTest, ClearThenRenderMatchesSizeHint) {
  const std::string long_name(32, 'n');
  const Circuit wide =
      MakeCircuit({{long_name, 1}}, {}, {RotationOn(registry_, kWideAngle, 0)});
  const Circuit again = MakeCircuit({{"q", 1}}, {}, {HadamardOn(registry_, 0)});

  view_->RenderCircuit(wide);
  ASSERT_GT(view_->sizeHint().width(), theme::kDefaultWidth);

  view_->ClearCircuit();
  EXPECT_EQ(view_->sizeHint(),
            QSize(theme::kDefaultWidth, theme::kDefaultHeight));
  EXPECT_EQ(view_->size(), view_->sizeHint());

  view_->RenderCircuit(again);
  EXPECT_EQ(view_->sizeHint(),
            ExpectedSizeHint(WireLabels("q", 1), {GateBoxWidth("h", {}, 1)}));
  EXPECT_EQ(view_->size(), view_->sizeHint());
}

}  // namespace qde::gui
