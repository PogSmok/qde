#include "qde/gui/quantum_circuit_view.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPen>
#include <QPolygonF>
#include <QRect>
#include <QSize>

#include "qde/circuit.hpp"
#include "qde/gui/config.hpp"
#include "qde/gui/theme.hpp"
#include "qde/gui/view_helpers.hpp"
#include "qde/operation.hpp"

namespace qde::gui {

namespace {
// ---- Standard Gates ----------------------------------------------
struct GateViewDef {
  int control_qubits;
  QColor color;
};

const std::unordered_map<std::string_view, GateViewDef> kStandardGates = {
    {"p", GateViewDef{0, theme::kGateP}},
    {"x", GateViewDef{0, theme::kGateX}},
    {"y", GateViewDef{0, theme::kGateY}},
    {"z", GateViewDef{0, theme::kGateZ}},
    {"h", GateViewDef{0, theme::kGateH}},
    {"s", GateViewDef{0, theme::kGateS}},
    {"sdg", GateViewDef{0, theme::kGateSDG}},
    {"t", GateViewDef{0, theme::kGateT}},
    {"tdg", GateViewDef{0, theme::kGateTDG}},
    {"sx", GateViewDef{0, theme::kGateSX}},
    {"rx", GateViewDef{0, theme::kGateRX}},
    {"ry", GateViewDef{0, theme::kGateRY}},
    {"rz", GateViewDef{0, theme::kGateRZ}},
    {"cx", GateViewDef{1, theme::kGateCX}},
    {"cy", GateViewDef{1, theme::kGateCY}},
    {"cz", GateViewDef{1, theme::kGateCZ}},
    {"cp", GateViewDef{1, theme::kGateCP}},
    {"crx", GateViewDef{1, theme::kGateCRX}},
    {"cry", GateViewDef{1, theme::kGateCRY}},
    {"crz", GateViewDef{1, theme::kGateCRZ}},
    {"ch", GateViewDef{1, theme::kGateCH}},
    {"swap", GateViewDef{0, theme::kGateSwap}},
    {"ccx", GateViewDef{2, theme::kGateCCX}},
    {"cswap", GateViewDef{1, theme::kGateCSwap}},
    {"cu", GateViewDef{1, theme::kGateCU}},
    {"U", GateViewDef{0, theme::kGateU}},
    {"CX", GateViewDef{1, theme::kGateCX}},
    {"phase", GateViewDef{0, theme::kGatePhase}},
    {"cphase", GateViewDef{1, theme::kGateCPhase}},
    {"id", GateViewDef{0, theme::kGateID}},
    {"u1", GateViewDef{0, theme::kGateU1}},
    {"u2", GateViewDef{0, theme::kGateU2}},
    {"u3", GateViewDef{0, theme::kGateU3}}};

// ---- Gate lookup -----------------------------------------------------

GateViewDef GetGateViewDef(const std::string& name) {
  const auto it = kStandardGates.find(name);
  if (it == kStandardGates.end()) {
    // TODO: implement user defined control gates
    return GateViewDef{0, theme::kGateDefault};
  }
  return it->second;
}

// ---- Gate label (name + parameters) ------------------------------

QString GateLabel(const Operation& op) {
  if (!op.gate) {
    return "?";
  }
  QString name = QString::fromStdString(op.gate->Name()).toUpper();
  if (op.gate_params.empty()) {
    return name;
  }
  QString params = FormatAngle(op.gate_params[0]);
  for (int i = 1; i < static_cast<int>(op.gate_params.size()); i++) {
    params += ", " + FormatAngle(op.gate_params[i]);
  }
  return name + "\n(" + params + ")";
}

// ---- Drawing primitives ----------------------------------------------------

constexpr int kGateLabelHPadding = 6;  // px on each side of the label

bool IsGlyphGate(const std::string& name) {  // gates drawn without a label box
  return name == "swap" || name == "cswap" || name == "CX" || name == "cx" ||
         name == "cz" || name == "ccx";
}

int LabelBoxWidth(const QString& label, const QFont& base) {
  const QFont& f = base;
  const QFontMetrics fm(f);
  const int w = fm.boundingRect(QRect(), Qt::AlignLeft, label).width();
  return std::max(theme::kGateSize, w + (2 * kGateLabelHPadding));
}

int OpBoxWidth(const Operation& op, const QFont& font) {
  switch (op.type) {
    case OperationType::kReset:
      return LabelBoxWidth(KetLabel(0, 1), font);
    case OperationType::kGate:
      if (op.gate && (op.qubits.size() == 1 || !IsGlyphGate(op.gate->Name()))) {
        return LabelBoxWidth(GateLabel(op), font);
      }
      return theme::kGateSize;
    default:
      return theme::kGateSize;
  }
}

void DrawGateBox(QPainter& p, const int cx, const int cy, const QString& label,
                 const QColor fill, const int width) {
  const QRect box(cx - (width / 2), cy - (theme::kGateSize / 2), width,
                  theme::kGateSize);

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
  p.drawText(box, Qt::AlignCenter, label);
}

void DrawControlDot(QPainter& p, const int cx, const int cy,
                    const QColor& color) {
  p.setBrush(color);
  p.setPen(Qt::NoPen);
  p.drawEllipse(QPoint(cx, cy), theme::kDotRadius, theme::kDotRadius);
}

void DrawTargetCircle(QPainter& p, const int cx, const int cy,
                      const QColor& color) {
  p.setBrush(Qt::NoBrush);
  p.setPen(QPen(color, 2));
  p.drawEllipse(QPoint(cx, cy), theme::kTargetRadius, theme::kTargetRadius);
  p.drawLine(cx, cy - theme::kTargetRadius, cx, cy + theme::kTargetRadius);
  p.drawLine(cx - theme::kTargetRadius, cy, cx + theme::kTargetRadius, cy);
}

void DrawXMark(QPainter& p, const int cx, const int cy, const QColor& color) {
  p.setPen(QPen(color, 2));
  p.drawLine(cx - theme::kXMarkArm, cy - theme::kXMarkArm,
             cx + theme::kXMarkArm, cy + theme::kXMarkArm);
  p.drawLine(cx + theme::kXMarkArm, cy - theme::kXMarkArm,
             cx - theme::kXMarkArm, cy + theme::kXMarkArm);
}

void DrawVerticalConnector(QPainter& p, const int cx, const int y0,
                           const int y1, const QColor& color) {
  p.setPen(QPen(color, 2));
  p.drawLine(cx, y0, cx, y1);
}

// Glyph gates

void DrawCNOT(QPainter& p, const int cx, const int ctrl_y, const int tgt_y,
              const QColor& color) {
  DrawVerticalConnector(p, cx, ctrl_y, tgt_y, color);
  DrawControlDot(p, cx, ctrl_y, color);
  DrawTargetCircle(p, cx, tgt_y, color);
}

void DrawCZ(QPainter& p, const int cx, const int y0, const int y1,
            const QColor& color) {
  DrawVerticalConnector(p, cx, y0, y1, color);
  DrawControlDot(p, cx, y0, color);
  DrawControlDot(p, cx, y1, color);
}

void DrawCCX(QPainter& p, const int cx, const int ctrl0_y, const int ctrl1_y,
             const int tgt_y, const QColor& color) {
  const int min_y = std::min({ctrl0_y, ctrl1_y, tgt_y});
  const int max_y = std::max({ctrl0_y, ctrl1_y, tgt_y});
  DrawVerticalConnector(p, cx, min_y, max_y, color);
  DrawControlDot(p, cx, ctrl0_y, color);
  DrawControlDot(p, cx, ctrl1_y, color);
  DrawTargetCircle(p, cx, tgt_y, color);
}

void DrawSWAP(QPainter& p, const int cx, const std::vector<int>& wire_ys,
              const int control_qubits, const QColor& color) {
  if (wire_ys.empty()) {
    return;
  }
  auto max_y = -1;
  auto min_y = INT_MAX;
  for (const auto y : wire_ys) {
    min_y = std::min(min_y, y);
    max_y = std::max(max_y, y);
  }
  DrawVerticalConnector(p, cx, max_y, min_y, color);
  for (int i = 0; i < control_qubits; i++) {
    DrawControlDot(p, cx, wire_ys[i], color);
  }
  for (int i = control_qubits; i < static_cast<int>(wire_ys.size()); i++) {
    DrawXMark(p, cx, wire_ys[i], color);
  }
}

void DrawMeasureBox(QPainter& p, const int cx, const int cy, const int width) {
  const QRect box(cx - (width / 2), cy - (theme::kGateSize / 2), width,
                  theme::kGateSize);
  p.setPen(Qt::NoPen);
  p.setBrush(theme::kGateShadow);
  p.drawRoundedRect(
      box.adjusted(theme::kGateShadowOffset, theme::kGateShadowOffset,
                   theme::kGateShadowOffset, theme::kGateShadowOffset),
      theme::kCornerRadius, theme::kCornerRadius);

  p.setBrush(theme::kGateMeasurement);
  p.setPen(QPen(theme::kGateMeasurement.lighter(theme::kGateBorderLighter),
                theme::kPenWidth));
  p.drawRoundedRect(box, theme::kCornerRadius, theme::kCornerRadius);

  // D-shaped arc (top half of ellipse inside the box)
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

void DrawBarrierColumn(QPainter& p, const int cx, const int top_y) {
  // semi-transparent stripe
  p.setPen(Qt::NoPen);
  p.setBrush(theme::kBarrierFill);
  p.drawRect(cx - theme::kBarrierHalfWidth, top_y - (theme::kCellHeight / 2),
             theme::kBarrierHalfWidth * 2, theme::kCellHeight);

  // dashed line
  p.setBrush(Qt::NoBrush);
  p.setPen(QPen(theme::kBarrierLine, theme::kPenWidth, Qt::DashLine));
  p.drawLine(cx, top_y - (theme::kCellHeight / 2), cx,
             top_y + (theme::kCellHeight / 2));
}

void DrawResetBox(QPainter& p, const int cx, const int cy, const int width) {
  DrawGateBox(p, cx, cy, KetLabel(0, 1), theme::kGateDefault, width);
}

void DrawMultiGate(QPainter& p, const int cx, const std::vector<int>& wire_ys,
                   const int control_qubits, const QString& label,
                   const QColor& color, const int width) {
  if (wire_ys.empty()) {
    return;
  }
  auto max_y = -1;
  auto min_y = INT_MAX;
  for (const auto y : wire_ys) {
    max_y = std::max(max_y, y);
    min_y = std::min(min_y, y);
  }
  DrawVerticalConnector(p, cx, min_y, max_y, color);
  for (int i = 0; i < control_qubits; i++) {
    DrawControlDot(p, cx, wire_ys[i], color);
  }
  DrawGateBox(p, cx, wire_ys[control_qubits], label, color, width);
  auto label_i = 2;
  for (int i = control_qubits + 1; i < static_cast<int>(wire_ys.size()); i++) {
    DrawGateBox(p, cx, wire_ys[i], QString("#%1").arg(label_i++), color, width);
  }
}

template <typename WireYFn, typename RowFn>
void DrawGateOperation(QPainter& painter, const int cx, const int box_w,
                       const Operation& op, WireYFn wire_y, RowFn row) {
  if (!op.gate) {
    return;
  }
  const int nq = op.qubits.size();
  const std::string& name = op.gate->Name();
  const auto& gate_def = GetGateViewDef(name);
  if (nq == 1) {
    DrawGateBox(painter, cx, wire_y(row(op.qubits[0])), GateLabel(op),
                gate_def.color, box_w);
    return;
  }

  std::vector<int> ys;
  ys.reserve(nq);
  for (const auto& qubit : op.qubits) {
    ys.push_back(wire_y(row(qubit)));
  }

  if (name == "swap" || name == "cswap") {
    DrawSWAP(painter, cx, ys, gate_def.control_qubits, gate_def.color);
  } else if (name == "CX" || name == "cx") {
    DrawCNOT(painter, cx, ys[0], ys[1], gate_def.color);
  } else if (name == "cz") {
    DrawCZ(painter, cx, ys[0], ys[1], gate_def.color);
  } else if (name == "ccx") {
    DrawCCX(painter, cx, ys[0], ys[1], ys[2], gate_def.color);
  } else {
    DrawMultiGate(painter, cx, ys, gate_def.control_qubits, GateLabel(op),
                  gate_def.color, box_w);
  }
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
  wires_.clear();
  qubit_rows_.clear();
  bit_rows_.clear();
  hidden_wires_ = 0;

  const auto build = [&](const auto& regs, const bool classical, auto& rows,
                         const int limit) {
    int shown = 0;
    for (const auto& [name, size] : regs) {
      auto& r = rows.emplace_back(size, -1);
      for (std::size_t i = 0; i < size; ++i) {
        if (shown >= limit) {
          ++hidden_wires_;
          continue;
        }
        r[i] = static_cast<int>(wires_.size());
        wires_.push_back(
            {QString::fromStdString(name) + "[" + QString::number(i) + "]",
             classical});
        ++shown;
      }
    }
  };
  build(circuit.QubitRegisters(), false, qubit_rows_,
        theme::kMaxDisplayedQubits);
  build(circuit.BitRegisters(), true, bit_rows_, theme::kMaxDisplayedBits);
  num_wires_ = static_cast<int>(wires_.size());

  max_wire_label_width_ = theme::kLabelWidth;
  const QFont f(theme::kWireLabelFontFamily, theme::kWireLabelFontSize);
  const QFontMetrics fm(f);
  padding_width_ = fm.horizontalAdvance(" ") * theme::kLabelPadding;
  for (const auto& [label, _] : wires_) {
    const auto label_width = fm.horizontalAdvance(label) + (2 * padding_width_);
    max_wire_label_width_ = std::max(label_width, max_wire_label_width_);
  }
  last_qubit_row_ = 0;
  for (const auto& reg : qubit_rows_) {
    for (int const r : reg) {
      last_qubit_row_ = std::max(last_qubit_row_, r);
    }
  }
  ComputeLayout();

  resize(sizeHint());  // tells QScrollArea to update its scrollable extent
  update();
}

void QuantumCircuitView::ClearCircuit() {
  circuit_.reset();
  wires_.clear();
  qubit_rows_.clear();
  bit_rows_.clear();
  op_cols_.clear();
  col_box_w_.clear();
  col_left_.assign(1, 0);
  last_qubit_row_ = 0;
  hidden_wires_ = 0;
  num_wires_ = 0;
  max_wire_label_width_ = theme::kLabelWidth;

  resize(sizeHint());
  update();
}

void QuantumCircuitView::ComputeLayout() {
  op_cols_.clear();
  col_box_w_.clear();
  col_left_.assign(1, 0);
  if (!circuit_) {
    return;
  }

  const QFont font(theme::kWireLabelFontFamily, theme::kWireLabelFontSize);
  std::vector<int> next_col(num_wires_, 0);
  std::set<std::pair<int, int>> occupied;

  for (const Operation& op : circuit_->Operations()) {
    op_cols_.push_back(-1);
    int& out_col = op_cols_.back();

    const bool hidden =
        std::any_of(op.qubits.begin(), op.qubits.end(),
                    [this](const auto& q) { return QubitRow(q) < 0; });
    if (hidden || op.qubits.empty()) {
      continue;
    }

    std::vector<int> rows;
    rows.reserve(op.qubits.size());
    for (const auto& q : op.qubits) {
      rows.push_back(QubitRow(q));
    }

    int br = -1;
    if (op.type == OperationType::kMeasure && !op.measure_target.empty()) {
      br = BitRow(op.measure_target[0]);
    }

    std::vector<int> deps = rows;
    std::vector<int> cells;
    switch (op.type) {
      case OperationType::kBarrier:
        cells = rows;
        break;
      case OperationType::kMeasure: {
        deps = {rows[0]};
        int lo = rows[0];
        int hi = rows[0];
        if (br >= 0) {
          deps.push_back(br);
          lo = std::min(lo, br);
          hi = std::max(hi, br);
        }
        for (int r = lo; r <= hi; ++r) {
          cells.push_back(r);
        }
        break;
      }
      default: {
        const auto [lo, hi] = std::minmax_element(rows.begin(), rows.end());
        for (int r = *lo; r <= *hi; ++r) {
          cells.push_back(r);
        }
        break;
      }
    }

    int col = 0;
    for (const int r : deps) {
      col = std::max(col, next_col[r]);
    }
    auto free_at = [&](const int c) {
      return std::all_of(cells.begin(), cells.end(), [&](const int r) {
        return occupied.count({c, r}) == 0;
      });
    };
    while (!free_at(col)) {
      ++col;
    }

    for (const int r : cells) {
      occupied.insert({col, r});
    }
    for (const int r : deps) {
      next_col[r] = col + 1;
    }

    out_col = col;
    if (col >= static_cast<int>(col_box_w_.size())) {
      col_box_w_.resize(col + 1, theme::kGateSize);
    }
    col_box_w_[col] = std::max(col_box_w_[col], OpBoxWidth(op, font));
  }

  const int gap = std::max(0, theme::kCellWidth - theme::kGateSize);
  col_left_.reserve(col_box_w_.size() + 1);
  for (const int bw : col_box_w_) {
    col_left_.push_back(col_left_.back() +
                        std::max(theme::kCellWidth, bw + gap));
  }
}

int QuantumCircuitView::WireY(const int flat_index) const {
  return margin_up_ + (flat_index * theme::kCellHeight) +
         (theme::kCellHeight / 2);
}

int QuantumCircuitView::ColWidth(const int col) const {
  return col_left_[col + 1] - col_left_[col];
}

int QuantumCircuitView::ColX(const int col) const {
  return max_wire_label_width_ + margin_left_ + col_left_[col] +
         (ColWidth(col) / 2);
}

QSize QuantumCircuitView::sizeHint() const {
  if (!circuit_) {
    return {theme::kDefaultWidth, theme::kDefaultHeight};
  }
  const int grid_w = col_left_.back() + theme::kCellWidth;  // +1 trailing cell
  const int w = max_wire_label_width_ + margin_left_ + margin_right_ + grid_w;
  const int h =
      margin_up_ + margin_down_ + ((TotalWires() + 1) * theme::kCellHeight);
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
  const int total_width = sizeHint().width();

  // ---- wire labels ---------------------------------------------------
  {
    const QFont f(theme::kWireLabelFontFamily, theme::kWireLabelFontSize);
    painter.setFont(f);

    for (int i = 0; i < num_wires_; ++i) {
      const int y = WireY(i);
      const QColor col =
          wires_[i].classical ? theme::kClassWireColor : theme::kWireColor;
      painter.setPen(col);
      const QRect label_rect(padding_width_, y - (theme::kCellHeight / 2),
                             max_wire_label_width_ - (2 * padding_width_),
                             theme::kCellHeight);
      painter.drawText(label_rect, Qt::AlignVCenter | Qt::AlignRight,
                       wires_[i].label);
    }
  }

  // ---- wires ---------------------------------------------------------
  for (int i = 0; i < num_wires_; ++i) {
    const int y = WireY(i);
    const int x0 = max_wire_label_width_;
    const int x1 = total_width - margin_right_;

    if (!wires_[i].classical) {
      painter.setPen(QPen(theme::kWireColor, theme::kPenWidth));
      painter.drawLine(x0, y, x1, y);
    } else {
      painter.setPen(QPen(theme::kClassWireColor, theme::kPenWidth));
      painter.drawLine(x0, y - theme::kClassGap, x1, y - theme::kClassGap);
      painter.drawLine(x0, y + theme::kClassGap, x1, y + theme::kClassGap);
    }
  }

  // ---- operations ----------------------------------------------------
  std::size_t op_idx = 0;
  for (const Operation& op : c.Operations()) {
    const int col = op_cols_[op_idx++];
    if (col < 0) {
      continue;
    }
    const int cx = ColX(col);
    const int box_w = col_box_w_[col];

    std::vector<int> rows;
    rows.reserve(op.qubits.size());
    for (const auto& q : op.qubits) {
      rows.push_back(QubitRow(q));
    }
    int br = -1;
    if (op.type == OperationType::kMeasure && !op.measure_target.empty()) {
      br = BitRow(op.measure_target[0]);
    }

    switch (op.type) {
      case OperationType::kBarrier:
        for (const int r : rows) {
          DrawBarrierColumn(painter, cx, WireY(r));
        }
        break;
      case OperationType::kReset:
        DrawResetBox(painter, cx, WireY(rows[0]), box_w);
        break;
      case OperationType::kMeasure: {
        const int qy = WireY(rows[0]);
        DrawMeasureBox(painter, cx, qy, box_w);
        if (br >= 0) {
          DrawMeasureArrow(painter, cx, qy, WireY(br));
        }
        break;
      }
      case OperationType::kGate:
        DrawGateOperation(
            painter, cx, box_w, op, [this](int i) { return WireY(i); },
            [this](QubitReference q) { return QubitRow(q); });
        break;
    }
  }
  if (hidden_wires_ > 0) {
    painter.setPen(theme::kPlaceholderText);
    painter.drawText(
        QPoint(padding_width_, WireY(num_wires_ - 1) + theme::kCellHeight),
        QString("+%1 wires hidden").arg(hidden_wires_));
  }
}

}  // namespace qde::gui