#ifndef GUI_APP_CONTROLLER_HPP_
#define GUI_APP_CONTROLLER_HPP_

#include <QTimer>
#include <memory>
#include <optional>

#include "qde/circuit.hpp"
#include "qde/gui/quantum_circuit_view.hpp"
#include "qde/gui/text_editor.hpp"
#include "qde/parser.hpp"

namespace qde::gui {

class AppController : public QObject {
  Q_OBJECT
 public:
  explicit AppController(QuantumCircuitView* circuitView,
                         TextEditor* textEditor, QObject* parent = nullptr);
  [[nodiscard]] const qde::Parser* parser() const { return parser_.get(); }
  [[nodiscard]] const qde::Circuit* circuit() const {
    return circuit_.has_value() ? &circuit_.value() : nullptr;
  }

  void onTextChanged();
  void parseNow();

 signals:
  void parseSuccess();
  void parseError(const QStringList& errors);

 private:
  QPointer<QuantumCircuitView> circuitView_;
  QPointer<TextEditor> textEditor_;
  std::unique_ptr<qde::Parser> parser_;
  std::optional<qde::Circuit> circuit_;
  QTimer debounceTimer_;
};

}  // namespace qde::gui

#endif  // GUI_APP_CONTROLLER_HPP_