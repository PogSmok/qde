#include "qde/gui/app_controller.hpp"

#include <exception>
#include <utility>
#include <vector>

#include "qde/backend_config.hpp"
#include "qde/operation.hpp"
#include "qde/simulator/simulation_circuit.hpp"
#include "qde/simulator/simulator.hpp"

namespace qde::gui {

namespace {

qde::Circuit StripMeasurements(const qde::Circuit& circuit) {
  std::vector<qde::Operation> ops;
  ops.reserve(circuit.Operations().size());
  for (const auto& op : circuit.Operations()) {
    if (op.type == qde::OperationType::kMeasure) {
      continue;
    }
    ops.push_back(op);
  }
  return qde::Circuit{circuit.GetRegistry(), circuit.QubitRegisters(),
                      circuit.BitRegisters(), std::move(ops)};
}

}  // namespace

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
  const auto result = qde::Parser::Parse(textEditor_->PlainText().toStdString(),
                                         qde::BackendConfig{});
  if (result.IsOk()) {
    circuit_ = result.GetCircuit();
    circuitView_->RenderCircuit(*circuit_);
    textEditor_->ClearErrors();
    emit ParseSuccess();
    RunSimulation();
  } else {
    circuit_.reset();
    simulationState_.reset();
    circuitView_->ClearCircuit();
    textEditor_->SetErrors(result.Errors());

    QStringList error_list;
    for (const auto& err : result.Errors()) {
      error_list << QString::fromStdString(err.message);
    }
    emit ParseError(error_list);
  }
}

void AppController::RunSimulation() {
  if (!circuit_.has_value()) {
    return;
  }

  try {
    int simulated_qubits = 0;
    for (const auto& [_, qubits] : circuit_->QubitRegisters()) {
      simulated_qubits += qubits;
    }
    if (simulated_qubits > theme::kMaxSimQubits) {
      emit SimulationFailed(
          QString(
              "Cannot simulate: circuit exceeds the maximum qubit count of %1")
              .arg(theme::kMaxSimQubits));
      return;
    }

    const qde::SimulationCircuit sim_circuit{StripMeasurements(*circuit_)};
    simulationState_ = qde::Simulator::RunFinal(sim_circuit);
    emit SimulationComplete(*simulationState_);
  } catch (const std::exception& e) {
    simulationState_.reset();
    emit SimulationFailed(QString::fromStdString(e.what()));
  }
}

}  // namespace qde::gui
