#include "qde/gui/quantum_circuit_view.hpp"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRect>
#include <QSize>

#include "qde/circuit.hpp"
#include "qde/gui/theme.hpp"
#include "qde/gui/view_helpers.hpp"
#include "qde/operation.hpp"

namespace qde::gui {

namespace {

// ---- Wire helpers ----------------------------------------------------------

struct WireInfo {
  QString label;
  bool classical{false};
};

std::vector<WireInfo> FlattenWires(const Circuit& c) {
  std::vector<WireInfo> wires;
  for (const auto& [name, size] : c.QubitRegisters()) {
    for (std::size_t i = 0; i < size; ++i) {
      wires.push_back(
          {QString::fromStdString(name) + "[" + QString::number(i) + "]",
           false});
    }
  }
  for (const auto& [name, size] : c.BitRegisters()) {
    for (std::size_t i = 0; i < size; ++i) {
      wires.push_back(
          {QString::fromStdString(name) + "[" + QString::number(i) + "]",
           true});
    }
  }
  return wires;
}

int FlatQubitIndex(const Circuit& c, QubitReference ref) {
  int idx = 0;
  const auto& regs = c.QubitRegisters();
  for (std::size_t r = 0; r < ref.reg; ++r) {
    idx += static_cast<int>(regs[r].size);
  }
  return idx + ref.qubit;
}

int FlatBitIndex(const Circuit& c, BitReference ref) {
  int qubit_total = 0;
  for (const auto& [name, size] : c.QubitRegisters()) {
    qubit_total += static_cast<int>(size);
  }
  int idx = 0;
  const auto& regs = c.BitRegisters();
  for (std::size_t r = 0; r < ref.reg; ++r) {
    idx += static_cast<int>(regs[r].size);
  }
  return qubit_total + idx + ref.bit;
}

// ---- Gate color lookup -----------------------------------------------------

QColor GateColor(const std::string& name) {
  static const std::unordered_map<std::string, QColor> kMap = {
      {"h", theme::kGateH},         {"x", theme::kGateX},
      {"y", theme::kGateY},         {"z", theme::kGateZ},
      {"s", theme::kGateS},         {"sdg", theme::kGateS},
      {"t", theme::kGateT},         {"tdg", theme::kGateT},
      {"rx", theme::kGateRotation}, {"ry", theme::kGateRotation},
      {"rz", theme::kGateRotation}, {"u", theme::kGateRotation},
      {"u1", theme::kGateRotation}, {"u2", theme::kGateRotation},
      {"u3", theme::kGateRotation}, {"p", theme::kGateRotation},
  };
  const auto it = kMap.find(name);
  return it != kMap.end() ? it->second : theme::kGateDefault;
}

// ---- Gate label (name + optional first param) ------------------------------

QString GateLabel(const Operation& op) {
  if (!op.gate) {
    return "?";
  }
  QString name = QString::fromStdString(op.gate->Name()).toUpper();
  if (op.gate_params.empty()) {
    return name;
  }
  return name + "(" + FormatAngle(op.gate_params[0]) + ")";
}

// ---- Drawing primitives ----------------------------------------------------

void DrawGateBox(QPainter& p, const int cx, const int cy, const QString& label,
                 const QColor fill) {
  const QRect box(cx - (theme::kGateSize / 2), cy - (theme::kGateSize / 2),
                  theme::kGateSize, theme::kGateSize);

  // shadow
  p.setPen(Qt::NoPen);
  p.setBrush(theme::kGateShadow);
  p.drawRoundedRect(
      box.adjusted(theme::kGateShadowOffset, theme::kGateShadowOffset,
                   theme::kGateShadowOffset, theme::kGateShadowOffset),
      theme::kCornerRadius, theme::kCornerRadius);

  // fill
  p.setBrush(fill);
  p.setPen(QPen(fill.lighter(theme::kGateBorderLighter), theme::kPenWidth));
  p.drawRoundedRect(box, theme::kCornerRadius, theme::kCornerRadius);

  // label
  p.setPen(theme::kGateLabelText);

  // check if the label is long (has params) and use a smaller font
  if (label.length() > theme::kLongGateLabelThreshold) {
    QFont f = p.font();
    f.setPointSizeF(f.pointSizeF() * theme::kLongGateLabelFontScale);
    p.setFont(f);
    p.drawText(box, Qt::AlignCenter, label);
    f.setPointSizeF(f.pointSizeF() / theme::kLongGateLabelFontScale);
    p.setFont(f);
  } else {
    p.drawText(box, Qt::AlignCenter, label);
  }
}

void DrawControlDot(QPainter& p, const int cx, const int cy) {
  p.setBrush(theme::kGateLabelText);
  p.setPen(Qt::NoPen);
  p.drawEllipse(QPoint(cx, cy), theme::kDotRadius, theme::kDotRadius);
}

void DrawTargetCircle(QPainter& p, const int cx, const int cy) {
  p.setBrush(Qt::NoBrush);
  p.setPen(QPen(theme::kGateLabelText, 2));
  p.drawEllipse(QPoint(cx, cy), theme::kTargetRadius, theme::kTargetRadius);
  p.drawLine(cx, cy - theme::kTargetRadius, cx, cy + theme::kTargetRadius);
  p.drawLine(cx - theme::kTargetRadius, cy, cx + theme::kTargetRadius, cy);
}

void DrawXMark(QPainter& p, const int cx, const int cy) {
  p.setPen(QPen(theme::kGateLabelText, 2));
  p.drawLine(cx - theme::kXMarkArm, cy - theme::kXMarkArm,
             cx + theme::kXMarkArm, cy + theme::kXMarkArm);
  p.drawLine(cx + theme::kXMarkArm, cy - theme::kXMarkArm,
             cx - theme::kXMarkArm, cy + theme::kXMarkArm);
}

void DrawVerticalConnector(QPainter& p, const int cx, const int y0,
                           const int y1) {
  p.setPen(QPen(theme::kGateLabelText, 2));
  p.drawLine(cx, y0, cx, y1);
}

void DrawCNOT(QPainter& p, const int cx, const int ctrl_y, const int tgt_y) {
  DrawVerticalConnector(p, cx, ctrl_y, tgt_y);
  DrawControlDot(p, cx, ctrl_y);
  DrawTargetCircle(p, cx, tgt_y);
}

void DrawCZ(QPainter& p, const int cx, const int y0, const int y1) {
  DrawVerticalConnector(p, cx, y0, y1);
  DrawControlDot(p, cx, y0);
  DrawControlDot(p, cx, y1);
}

void DrawSWAP(QPainter& p, const int cx, const int y0, const int y1) {
  DrawVerticalConnector(p, cx, y0, y1);
  DrawXMark(p, cx, y0);
  DrawXMark(p, cx, y1);
}

void DrawCCX(QPainter& p, const int cx, const int ctrl0_y, const int ctrl1_y,
             const int tgt_y) {
  const int min_y = std::min({ctrl0_y, ctrl1_y, tgt_y});
  const int max_y = std::max({ctrl0_y, ctrl1_y, tgt_y});
  DrawVerticalConnector(p, cx, min_y, max_y);
  DrawControlDot(p, cx, ctrl0_y);
  DrawControlDot(p, cx, ctrl1_y);
  DrawTargetCircle(p, cx, tgt_y);
}

void DrawGenericMultiGate(QPainter& p, const int cx,
                          const std::vector<int>& wire_ys,
                          const QString& label) {
  if (wire_ys.empty()) {
    return;
  }
  const int min_y = *std::min_element(wire_ys.begin(), wire_ys.end());
  const int max_y = *std::max_element(wire_ys.begin(), wire_ys.end());
  const int height = max_y - min_y + theme::kGateSize;
  const QRect box(cx - (theme::kGateSize / 2), min_y - (theme::kGateSize / 2),
                  theme::kGateSize, height);
  p.setBrush(theme::kGateDefault);
  p.setPen(QPen(theme::kGateDefault.lighter(theme::kGateBorderLighter),
                theme::kPenWidth));
  p.drawRoundedRect(box, theme::kCornerRadius, theme::kCornerRadius);
  p.setPen(theme::kGateLabelText);
  p.drawText(box, Qt::AlignCenter | Qt::TextWordWrap, label);
}

void DrawMeasureBox(QPainter& p, const int cx, const int cy) {
  const QRect box(cx - (theme::kGateSize / 2), cy - (theme::kGateSize / 2),
                  theme::kGateSize, theme::kGateSize);

  p.setPen(Qt::NoPen);
  p.setBrush(theme::kGateShadow);
  p.drawRoundedRect(
      box.adjusted(theme::kGateShadowOffset, theme::kGateShadowOffset,
                   theme::kGateShadowOffset, theme::kGateShadowOffset),
      theme::kCornerRadius, theme::kCornerRadius);

  p.setBrush(theme::kGateMeas);
  p.setPen(QPen(theme::kGateMeas.lighter(theme::kGateBorderLighter),
                theme::kPenWidth));
  p.drawRoundedRect(box, theme::kCornerRadius, theme::kCornerRadius);

  // D-shaped arc (bottom half of ellipse inside the box)
  const int mx = cx;
  const int my = cy + (theme::kGateSize / 8);
  constexpr int arc_w = (theme::kGateSize / 2) - theme::kMeasureArcInset;
  constexpr int arc_h = theme::kGateSize / 3;
  QPainterPath arc;
  arc.moveTo(mx - arc_w, my);
  arc.arcTo(mx - arc_w, my - arc_h, arc_w * 2, arc_h * 2, 0, 180);
  p.setPen(QPen(theme::kMeasureArcColor, 2));
  p.setBrush(Qt::NoBrush);
  p.drawPath(arc);

  // arrow from arc center pointing up-right
  const int ax = mx + (arc_w / 2);
  const int ay = my - (arc_h / 2);
  p.drawLine(mx, my, ax, ay);
  // arrowhead
  QPolygonF head;
  head << QPointF(ax, ay) << QPointF(ax - 5, ay - 1) << QPointF(ax - 1, ay + 5);
  p.setBrush(theme::kMeasureArcColor);
  p.setPen(Qt::NoPen);
  p.drawPolygon(head);
}

void DrawMeasureArrow(QPainter& p, const int cx, const int from_y,
                      const int to_y) {
  // dashed vertical line from qubit wire to classical wire
  const QPen dashed(theme::kClassWireColor, theme::kPenWidth, Qt::DashLine);
  p.setPen(dashed);
  p.drawLine(cx, from_y + (theme::kGateSize / 2), cx, to_y);

  // small arrowhead at the classical wire
  const QPen solid(theme::kClassWireColor, theme::kPenWidth, Qt::SolidLine);
  p.setPen(solid);
  p.setBrush(theme::kClassWireColor);
  QPolygonF tip;
  tip << QPointF(cx, to_y + 5) << QPointF(cx - 4, to_y - 3)
      << QPointF(cx + 4, to_y - 3);
  p.drawPolygon(tip);
}

void DrawBarrierColumn(QPainter& p, const int cx, const int top_y,
                       const int bottom_y) {
  // semi-transparent stripe
  p.setPen(Qt::NoPen);
  p.setBrush(theme::kBarrierFill);
  p.drawRect(cx - theme::kBarrierHalfWidth, top_y - (theme::kCellHeight / 2),
             theme::kBarrierHalfWidth * 2,
             bottom_y - top_y + theme::kCellHeight);

  // dashed line
  p.setBrush(Qt::NoBrush);
  p.setPen(QPen(theme::kBarrierLine, theme::kPenWidth, Qt::DashLine));
  p.drawLine(cx, top_y - (theme::kCellHeight / 2), cx,
             bottom_y + (theme::kCellHeight / 2));
}

void DrawResetBox(QPainter& p, const int cx, const int cy) {
  DrawGateBox(p, cx, cy, KetLabel(0, 1), theme::kGateDefault);
}

}  // namespace

// ---- QuantumCircuitView implementation -------------------------------------

QuantumCircuitView::QuantumCircuitView(QWidget* parent) : QWidget{parent} {
  QPalette pal = palette();
  pal.setColor(QPalette::Window, theme::kCircuitBackground);
  setAutoFillBackground(true);
  setPalette(pal);
}

void QuantumCircuitView::SetMargins(const int up, const int down,
                                    const int left, const int right) {
  margin_up_ = up;
  margin_down_ = down;
  margin_left_ = left;
  margin_right_ = right;
  resize(sizeHint());
  update();
}

void QuantumCircuitView::RenderCircuit(const Circuit& circuit) {
  circuit_ = circuit;
  resize(sizeHint());  // tells QScrollArea to update its scrollable extent
  update();
}

void QuantumCircuitView::ClearCircuit() {
  circuit_.reset();
  resize(sizeHint());
  update();
}

int QuantumCircuitView::WireY(const int flat_index) const {
  return margin_up_ + (flat_index * theme::kCellHeight) +
         (theme::kCellHeight / 2);
}

int QuantumCircuitView::ColX(const int col) const {
  return theme::kLabelWidth + margin_left_ + (col * theme::kCellWidth) +
         (theme::kCellWidth / 2);
}

int QuantumCircuitView::TotalWires() const {
  if (!circuit_) {
    return 0;
  }
  int n = 0;
  for (const auto& [name, size] : circuit_->QubitRegisters()) {
    n += static_cast<int>(size);
  }
  for (const auto& [name, size] : circuit_->BitRegisters()) {
    n += static_cast<int>(size);
  }
  return n;
}

QSize QuantumCircuitView::sizeHint() const {
  if (!circuit_) {
    return {theme::kDefaultWidth, theme::kDefaultHeight};
  }
  const int cols = static_cast<int>(circuit_->Operations().size());
  const int w = theme::kLabelWidth + margin_left_ + margin_right_ +
                ((cols + 1) * theme::kCellWidth);
  const int h = margin_up_ + margin_down_ + (TotalWires() * theme::kCellHeight);
  return {w, h};
}

void QuantumCircuitView::paintEvent(QPaintEvent* /*event*/) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // ---- placeholder ---------------------------------------------------
  if (!circuit_) {
    painter.setPen(theme::kPlaceholderText);
    QFont f = painter.font();
    f.setPointSize(theme::kPlaceholderFontSize);
    painter.setFont(f);
    painter.drawText(rect(), Qt::AlignCenter, "Parse QASM to view circuit");
    return;
  }

  const Circuit& c = *circuit_;
  const auto wires = FlattenWires(c);
  const int num_wires = static_cast<int>(wires.size());
  const int total_width = sizeHint().width();

  // ---- wire labels ---------------------------------------------------
  {
    QFont f = painter.font();
    f.setFamily(theme::kWireLabelFontFamily);
    f.setPointSize(theme::kWireLabelFontSize);
    painter.setFont(f);

    for (int i = 0; i < num_wires; ++i) {
      const int y = WireY(i);
      const QColor col =
          wires[i].classical ? theme::kClassWireColor : theme::kWireColor;
      painter.setPen(col);
      const QRect label_rect(theme::kLabelPadding, y - (theme::kCellHeight / 2),
                             theme::kLabelWidth - (theme::kLabelPadding * 2),
                             theme::kCellHeight);
      painter.drawText(label_rect, Qt::AlignVCenter | Qt::AlignRight,
                       wires[i].label);
    }
  }

  // ---- wires ---------------------------------------------------------
  for (int i = 0; i < num_wires; ++i) {
    const int y = WireY(i);
    constexpr int x0 = theme::kLabelWidth;
    const int x1 = total_width - margin_right_;

    if (!wires[i].classical) {
      painter.setPen(QPen(theme::kWireColor, theme::kPenWidth));
      painter.drawLine(x0, y, x1, y);
    } else {
      painter.setPen(QPen(theme::kClassWireColor, theme::kPenWidth));
      painter.drawLine(x0, y - theme::kClassGap, x1, y - theme::kClassGap);
      painter.drawLine(x0, y + theme::kClassGap, x1, y + theme::kClassGap);
    }
  }

  // ---- operations ----------------------------------------------------
  const auto& ops = c.Operations();
  for (int col = 0; col < static_cast<int>(ops.size()); ++col) {
    const Operation& op = ops[col];
    const int cx = ColX(col);

    switch (op.type) {
      case OperationType::kBarrier: {
        std::vector<int> rows;
        rows.reserve(op.qubits.size());
        for (const auto& qr : op.qubits) {
          rows.push_back(FlatQubitIndex(c, qr));
        }
        for (const auto& [first, last] : ContiguousRuns(std::move(rows))) {
          DrawBarrierColumn(painter, cx, WireY(first), WireY(last));
        }
        break;
      }

      case OperationType::kReset: {
        if (op.qubits.empty()) {
          break;
        }
        const int wy = WireY(FlatQubitIndex(c, op.qubits[0]));
        DrawResetBox(painter, cx, wy);
        break;
      }

      case OperationType::kMeasure: {
        if (op.qubits.empty()) {
          break;
        }
        const int qy = WireY(FlatQubitIndex(c, op.qubits[0]));
        DrawMeasureBox(painter, cx, qy);
        if (!op.measure_target.empty()) {
          const int by = WireY(FlatBitIndex(c, op.measure_target[0]));
          DrawMeasureArrow(painter, cx, qy, by);
        }
        break;
      }

      case OperationType::kGate: {
        if (!op.gate) {
          break;
        }
        const std::string& name = op.gate->Name();

        if (const int nq = static_cast<int>(op.qubits.size()); nq == 1) {
          const int wy = WireY(FlatQubitIndex(c, op.qubits[0]));
          DrawGateBox(painter, cx, wy, GateLabel(op), GateColor(name));

        } else if (nq == 2) {
          const int y0 = WireY(FlatQubitIndex(c, op.qubits[0]));
          const int y1 = WireY(FlatQubitIndex(c, op.qubits[1]));
          if (name == "cx" || name == "cnot") {
            DrawCNOT(painter, cx, y0, y1);
          } else if (name == "cz") {
            DrawCZ(painter, cx, y0, y1);
          } else if (name == "swap") {
            DrawSWAP(painter, cx, y0, y1);
          } else {
            // generic 2-qubit box
            DrawGenericMultiGate(painter, cx, {y0, y1},
                                 QString::fromStdString(name).toUpper());
          }

        } else if (nq == 3 && (name == "ccx" || name == "toffoli")) {
          const int y0 = WireY(FlatQubitIndex(c, op.qubits[0]));
          const int y1 = WireY(FlatQubitIndex(c, op.qubits[1]));
          const int y2 = WireY(FlatQubitIndex(c, op.qubits[2]));
          DrawCCX(painter, cx, y0, y1, y2);

        } else {
          std::vector<int> ys;
          ys.reserve(nq);
          for (const auto& qr : op.qubits) {
            ys.push_back(WireY(FlatQubitIndex(c, qr)));
          }
          DrawGenericMultiGate(painter, cx, ys,
                               QString::fromStdString(name).toUpper());
        }
        break;
      }
    }
  }
}

}  // namespace qde::gui