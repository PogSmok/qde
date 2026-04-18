#ifndef GUI_QUANTUM_CIRCUIT_VIEW_HPP_
#define GUI_QUANTUM_CIRCUIT_VIEW_HPP_

#include <QWidget>

namespace qde::gui {

// TODO: implement
class QuantumCircuitView : public QWidget {
  Q_OBJECT
 public:
  explicit QuantumCircuitView(QWidget* parent = nullptr);
};

}  // namespace qde::gui

#endif  // GUI_QUANTUM_CIRCUIT_VIEW_HPP_