#ifndef GUI_QUANTUM_CIRCUIT_VIEW_HPP_
#define GUI_QUANTUM_CIRCUIT_VIEW_HPP_

#include <optional>

#include <QSize>
#include <QWidget>

#include "qde/circuit.hpp"
#include "theme.hpp"

namespace qde::gui {

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
  int TotalWires() const;

  int margin_up_ = theme::kDefaultMargin;
  int margin_down_ = theme::kDefaultMargin;
  int margin_left_ = theme::kDefaultMargin;
  int margin_right_ = theme::kDefaultMargin;
  std::optional<Circuit> circuit_;
};

}  // namespace qde::gui

#endif  // GUI_QUANTUM_CIRCUIT_VIEW_HPP_
