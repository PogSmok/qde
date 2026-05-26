#include "qde/gui/app_controller.hpp"
#include "qde/backend_config.hpp"

namespace qde::gui {

AppController::AppController(QuantumCircuitView* circuit_view,
                             TextEditor* text_editor, QObject* parent)
    : QObject{parent},
      circuitView_(circuit_view),
      textEditor_(text_editor),
      parser_(std::make_unique<qde::Parser>()) {
  Q_ASSERT(circuit_view);
  Q_ASSERT(text_editor);

  constexpr int debounce_time_ms = 400;

  debounceTimer_.setSingleShot(true);
  debounceTimer_.setInterval(debounce_time_ms);  // 400ms debounce

  connect(textEditor_, &TextEditor::TextChanged, this,
          &AppController::OnTextChanged);
  connect(&debounceTimer_, &QTimer::timeout, this, &AppController::ParseNow);
}

void AppController::OnTextChanged() { debounceTimer_.start(); }

void AppController::ParseNow() {
  auto result = qde::Parser::Parse(textEditor_->PlainText().toStdString(),
                               qde::BackendConfig{});
  if (result.IsOk()) {
    circuit_ = result.GetCircuit();
    textEditor_->ClearErrors();
    emit ParseSuccess();
  } else {
    circuit_.reset();
    textEditor_->SetErrors(result.Errors());

    QStringList error_list;
    for (const auto& err : result.Errors()) {
      error_list << QString::fromStdString(err.message);
    }
    emit ParseError(error_list);
  }
}

}  // namespace qde::gui
