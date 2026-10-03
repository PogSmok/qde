#ifndef GUI_SIMULATION_RESULTS_VIEW_HPP_
#define GUI_SIMULATION_RESULTS_VIEW_HPP_

#include <optional>

#include <QPointer>
#include <QSize>
#include <QWidget>

#include "qde/simulator/simulation_state.hpp"

class QComboBox;
class QPaintEvent;
class QResizeEvent;

namespace qde::gui {

// Displays simulation results for a circuit as a bar histogram.
// A dropdown switches between the "Probabilities" view
// (|amplitude|^2 per basis state) and the "Statevector" view (amplitude
// magnitude, colored by phase).
class SimulationResultsView : public QWidget {
  Q_OBJECT
 public:
  enum class Mode { kProbabilities, kStatevector };

  explicit SimulationResultsView(QWidget* parent = nullptr);

  void ShowState(const qde::SimulationState& state);

  void ClearState();

  [[nodiscard]] Mode GetMode() const { return mode_; }
  void SetMode(Mode mode);

  QSize sizeHint() const override;

 protected:
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

 private:
  void PaintHistogram(QPaintEvent* event);

  QPointer<QComboBox> modeSelector_;
  Mode mode_{Mode::kProbabilities};
  std::optional<qde::SimulationState> state_;
};

}  // namespace qde::gui

#endif  // GUI_SIMULATION_RESULTS_VIEW_HPP_
