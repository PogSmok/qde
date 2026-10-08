#ifndef GUI_QUANTUM_CIRCUIT_VIEW_HPP_
#define GUI_QUANTUM_CIRCUIT_VIEW_HPP_

#include <optional>

#include <QPaintEvent>
#include <QSize>
#include <QWidget>

#include "qde/circuit.hpp"
#include "qde/gui/theme.hpp"

namespace qde::gui {

struct WireInfo {
  QString label;
  bool classical{false};
};

class QuantumCircuitView : public QWidget {
  Q_OBJECT
 public:
  explicit QuantumCircuitView(QWidget* parent = nullptr);

  void SetMargins(int up, int down, int left, int right);
  void RenderCircuit(const Circuit& circuit);
  void ClearCircuit();

  QSize sizeHint() const override;

 protected:
  void paintEvent(QPaintEvent* event) override;

 private:
  int WireY(int flat_index) const;
  int ColX(int col) const;
  int TotalWires() const { return num_wires_; }
  int QubitRow(const QubitReference& r) const {
    return qubit_rows_[r.reg][r.qubit];
  }
  int BitRow(const BitReference& r) const { return bit_rows_[r.reg][r.bit]; }

  int margin_up_ = theme::kDefaultMargin;
  int margin_down_ = theme::kDefaultMargin;
  int margin_left_ = theme::kDefaultMargin;
  int margin_right_ = theme::kDefaultMargin;
  int max_wire_label_width_ = theme::kLabelWidth;
  int num_wires_ = 0;
  int padding_width_ = 0;
  int last_qubit_row_ = 0;
  std::vector<std::vector<int>> qubit_rows_, bit_rows_;  // -1 = hidden
  int hidden_wires_ = 0;
  std::vector<WireInfo> wires_;
  std::optional<Circuit> circuit_;
};

}  // namespace qde::gui

#endif  // GUI_QUANTUM_CIRCUIT_VIEW_HPP_
