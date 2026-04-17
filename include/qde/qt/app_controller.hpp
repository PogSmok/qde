#ifndef APP_CONTROLLER_HPP_
#define APP_CONTROLLER_HPP_

#include <QObject>
#include <QStringList>
#include <memory>
#include <optional>

#include "qde/qt/quantum_circuit_view.hpp"
#include "qde/qt/text_editor.hpp"
#include "qde/parser.hpp"
#include "qde/circuit.hpp"


class AppController : public QObject {
  Q_OBJECT
 public:
  explicit AppController(QuantumCircuitView* circuitView, TextEditor* textEditor, QObject* parent = nullptr);
  virtual ~AppController();
  [[nodiscard]] qde::Parser*  parser()  const { return parser_.get();  }
  [[nodiscard]] const qde::Circuit* circuit() const { return circuit_.has_value() ? &circuit_.value() : nullptr; }

 public slots:
  void parseNow();

 signals:
  void parseSuccess();
  void parseError(const QStringList& errors);

 private:
  QuantumCircuitView* circuitView_;
  TextEditor* textEditor_;
  std::unique_ptr<qde::Parser> parser_;
  std::optional<qde::Circuit> circuit_;
};

#endif // APP_CONTROLLER_HPP_