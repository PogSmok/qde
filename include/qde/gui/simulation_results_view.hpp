#ifndef GUI_SIMULATION_RESULTS_VIEW_HPP_
#define GUI_SIMULATION_RESULTS_VIEW_HPP_

#include <cstdint>
#include <optional>

#include <QComboBox>
#include <QPaintEvent>
#include <QPointer>
#include <QSize>
#include <QWidget>

#include "qde/simulator/simulation_state.hpp"

namespace qde::gui {

// Displays simulation results for a circuit as a bar histogram.
// A dropdown switches between the "Probabilities" view
// (|amplitude|^2 per basis state) and the "Statevector" view (amplitude
// magnitude, colored by phase).
class SimulationResultsView : public QWidget {
  Q_OBJECT
 public:
  enum class Mode : std::uint8_t { kProbabilities, kStatevector };

  explicit SimulationResultsView(QWidget* parent = nullptr);

  void ShowState(const qde::SimulationState& state);
  void ClearState();
  [[nodiscard]] Mode GetMode() const { return mode_; }
  void SetMode(Mode mode);
  QSize sizeHint() const override;

 public slots:
  void ShowError(const QString& message);

 protected:
  void paintEvent(QPaintEvent* event) override;

 private:
  // Sets the minimum width to the histogram's required width so that an
  // enclosing QScrollArea scrolls instead of clipping bars.
  void FitToContent();
  int BarWidth() const;

  QPointer<QComboBox> mode_selector_;
  Mode mode_{Mode::kProbabilities};
  std::optional<qde::SimulationState> state_;
  std::optional<QString> error_;
};

}  // namespace qde::gui

#endif  // GUI_SIMULATION_RESULTS_VIEW_HPP_
