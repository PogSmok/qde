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

  constexpr int kDebounceTimeMs = 400;

  debounceTimer_.setSingleShot(true);
  debounceTimer_.setInterval(kDebounceTimeMs);  // 400ms debounce

  connect(textEditor_, &TextEditor::textChanged, this,
          &AppController::onTextChanged);
  connect(&debounceTimer_, &QTimer::timeout, this, &AppController::parseNow);
}

void AppController::onTextChanged() { debounceTimer_.start(); }

void AppController::parseNow() {
  auto result = parser_->parse(textEditor_->plainText().toStdString(),
                               qde::BackendConfig{});
  if (result.isOk()) {
    circuit_ = result.circuit();
    textEditor_->clearErrors();
    emit parseSuccess();
  } else {
    circuit_.reset();
    textEditor_->setErrors(result.errors());

    QStringList error_list;
    for (const auto& err : result.errors()) {
      error_list << QString::fromStdString(err.message);
    }
    emit parseError(error_list);
  }
}

}  // namespace qde::gui
