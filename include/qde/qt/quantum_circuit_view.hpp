#ifndef QUANTUM_CIRCUIT_VIEW_HPP_
#define QUANTUM_CIRCUIT_VIEW_HPP_

#include <QWidget>

// TODO: implement
class QuantumCircuitView : public QWidget {
  Q_OBJECT
 public:
  explicit QuantumCircuitView(QWidget* parent = nullptr);
  virtual ~QuantumCircuitView();
};

#endif // QUANTUM_CIRCUIT_VIEW_HPP_