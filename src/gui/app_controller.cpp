#include "qde/gui/app_controller.hpp"

namespace qde::gui {

AppController::AppController(QuantumCircuitView* circuitView,
                             TextEditor* textEditor, QObject* parent)
    : QObject{parent},
      circuitView_(circuitView),
      textEditor_(textEditor),
      parser_(std::make_unique<qde::Parser>()) {
  debounceTimer_.setSingleShot(true);
  debounceTimer_.setInterval(400);  // 400ms debounce

  connect(textEditor_, &TextEditor::textChanged, this,
          &AppController::onTextChanged);
  connect(&debounceTimer_, &QTimer::timeout, this, &AppController::parseNow);
}

void AppController::onTextChanged() { debounceTimer_.start(); }

void AppController::parseNow() {
  auto result = parser_->parse(textEditor_->plainText().toStdString());
  if (result.isOk()) {
    circuit_ = result.circuit();
    textEditor_->clearErrors();
    emit parseSuccess();
  } else {
    circuit_.reset();
    textEditor_->setErrors(result.errors());

    QStringList errorList;
    for (const auto& err : result.errors()) {
      errorList << QString::fromStdString(err.message);
    }
    emit parseError(errorList);
  }
}

}  // namespace qde::gui
