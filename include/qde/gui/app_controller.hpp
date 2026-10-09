#ifndef GUI_APP_CONTROLLER_HPP_
#define GUI_APP_CONTROLLER_HPP_

#include <QTimer>
#include <memory>
#include <optional>

#include "qde/circuit.hpp"
#include "qde/gui/quantum_circuit_view.hpp"
#include "qde/gui/text_editor.hpp"
#include "qde/parser.hpp"
#include "qde/simulator/simulation_state.hpp"

namespace qde::gui {

class AppController : public QObject {
  Q_OBJECT
 public:
  explicit AppController(QuantumCircuitView* circuit_view,
                         TextEditor* text_editor, QObject* parent = nullptr);
  [[nodiscard]] const qde::Parser* Parser() const { return parser_.get(); }
  [[nodiscard]] const qde::Circuit* GetCircuit() const {
    return circuit_.has_value() ? &circuit_.value() : nullptr;
  }
  [[nodiscard]] const qde::simulator::SimulationState* GetSimulationState()
      const {
    return simulationState_.has_value() ? &simulationState_.value() : nullptr;
  }

  void OnTextChanged();
  void ParseNow();

 signals:
  void ParseSuccess();
  void ParseError(const QStringList& errors);
  void SimulationComplete(const qde::simulator::SimulationState& state);
  void SimulationFailed(const QString& message);

 private:
  void RunSimulation();

  QPointer<QuantumCircuitView> circuitView_;
  QPointer<TextEditor> textEditor_;
  std::unique_ptr<qde::Parser> parser_;
  std::optional<qde::Circuit> circuit_;
  std::optional<qde::simulator::SimulationState> simulationState_;
  QTimer debounceTimer_;
};

}  // namespace qde::gui

#endif  // GUI_APP_CONTROLLER_HPP_