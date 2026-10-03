#include "qde/gui/simulation_results_view.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include <QColor>
#include <QComboBox>
#include <QFont>
#include <QPainter>
#include <QPen>
#include <QRect>
#include <QResizeEvent>
#include <QString>

#include "qde/gui/theme.hpp"
#include "qde/gui/view_helpers.hpp"

namespace qde::gui {

namespace {

constexpr double kPi = 3.141592653589793238462643383;

int BarCount(const qde::SimulationState& state) {
  return static_cast<int>(state.basis_probabilities.size());
}

// Index of the basis state with the largest probability; used as the phase
// reference (its amplitude is taken to be real and positive).
std::size_t ReferenceIndex(const qde::SimulationState& state) {
  const auto& p = state.basis_probabilities;
  return static_cast<std::size_t>(
      std::distance(p.begin(), std::max_element(p.begin(), p.end())));
}

// Phase of amplitude i relative to the reference, recovered from the pure-state
// density matrix: rho[i][ref] = psi_i * conj(psi_ref). With psi_ref real and
// positive, arg(psi_i) = arg(rho[i][ref]).
double AmplitudePhase(const qde::SimulationState& state, const std::size_t i,
                      const std::size_t ref) {
  const std::size_t dim = state.basis_probabilities.size();
  if (state.density_matrix.size() != dim * dim) {
    return 0.0;
  }
  return std::arg(state.density_matrix[(i * dim) + ref]);
}

QColor PhaseColor(double phase) {
  double hue = (phase + kPi) / (2.0 * kPi);  // map [-pi, pi] -> [0, 1]
  hue = std::clamp(hue, 0.0, 1.0);
  return QColor::fromHsvF(hue, theme::kHistPhaseSaturation,
                          theme::kHistPhaseValue);
}

}  // namespace

SimulationResultsView::SimulationResultsView(QWidget* parent)
    : QWidget{parent}, modeSelector_(new QComboBox(this)) {
  QPalette pal = palette();
  pal.setColor(QPalette::Window, theme::kHistBackground);
  setAutoFillBackground(true);
  setPalette(pal);

  modeSelector_->addItem("Probabilities");
  modeSelector_->addItem("Statevector");
  modeSelector_->setStyleSheet(
      QString("QComboBox { background: %1; color: %2; border: 1px solid %3;"
              " padding: 1px 6px; }"
              "QComboBox QAbstractItemView { background: %1; color: %2;"
              " selection-background-color: %4; }")
          .arg(theme::kMenuBackground, theme::kMenuText,
               theme::kSplitterHandleBackground,
               theme::kMenuSelectedBackground));
  modeSelector_->setGeometry(theme::kHistMargin, theme::kHistSelectorTop,
                             theme::kHistSelectorWidth,
                             theme::kHistSelectorHeight);

  connect(modeSelector_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, [this](int index) {
            mode_ = index == 1 ? Mode::kStatevector : Mode::kProbabilities;
            update();
          });
}

void SimulationResultsView::ShowState(const qde::SimulationState& state) {
  state_ = state;
  FitToContent();
}

void SimulationResultsView::ClearState() {
  state_.reset();
  FitToContent();
}

void SimulationResultsView::FitToContent() {
  const QSize hint = sizeHint();
  setMinimumWidth(hint.width());
  resize(hint.width(), std::max(height(), hint.height()));
  update();
}

void SimulationResultsView::SetMode(const Mode mode) {
  mode_ = mode;
  if (modeSelector_) {
    modeSelector_->setCurrentIndex(mode == Mode::kStatevector ? 1 : 0);
  }
  update();
}

QSize SimulationResultsView::sizeHint() const {
  if (!state_) {
    return {theme::kHistDefaultWidth, theme::kHistDefaultHeight};
  }

  const int bars = BarCount(*state_);
  if (bars <= 0 || bars > theme::kHistMaxStates) {
    return {theme::kHistDefaultWidth, theme::kHistDefaultHeight};
  }

  const int width = (theme::kHistMargin * 2) + theme::kHistAxisLeftPad +
                    theme::kHistBarGap +
                    (bars * (theme::kHistBarWidth + theme::kHistBarGap));
  return {std::max(width, theme::kHistDefaultWidth), theme::kHistDefaultHeight};
}

void SimulationResultsView::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  if (modeSelector_) {
    modeSelector_->setGeometry(theme::kHistMargin, theme::kHistSelectorTop,
                               theme::kHistSelectorWidth,
                               theme::kHistSelectorHeight);
  }
}

void SimulationResultsView::paintEvent(QPaintEvent* event) {
  PaintHistogram(event);
}

void SimulationResultsView::PaintHistogram(QPaintEvent* /*event*/) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  constexpr int top_pad =
      theme::kHistSelectorTop + theme::kHistSelectorHeight + theme::kHistBarGap;

  // ---- placeholder ---------------------------------------------------
  if (!state_) {
    painter.setPen(theme::kHistPlaceholderText);
    QFont f = painter.font();
    f.setPointSize(theme::kHistTitleFontSize);
    painter.setFont(f);
    painter.drawText(QRect(0, top_pad, width(), height() - top_pad),
                     Qt::AlignCenter, "Parse a circuit to view results");
    return;
  }

  const qde::SimulationState& state = *state_;
  const int bars = BarCount(state);
  const bool statevector = mode_ == Mode::kStatevector;

  // ---- too-many-states fallback -------------------------------------
  if (bars <= 0 || bars > theme::kHistMaxStates) {
    painter.setPen(theme::kHistPlaceholderText);
    painter.drawText(QRect(0, top_pad, width(), height() - top_pad),
                     Qt::AlignCenter,
                     QString("Too many states to display (%1 qubits)")
                         .arg(state.qubit_count));
    return;
  }

  // ---- plot geometry -------------------------------------------------
  constexpr int plot_left = theme::kHistMargin + theme::kHistAxisLeftPad;
  constexpr int plot_top = top_pad;
  const int plot_right = width() - theme::kHistMargin;
  const int plot_bottom = height() - theme::kHistAxisBottomPad;
  const int plot_height = plot_bottom - plot_top;

  if (plot_height <= 0 || plot_right <= plot_left) {
    return;
  }

  // ---- y-axis gridlines and labels ----------------------------------
  QFont label_font = painter.font();
  label_font.setBold(false);
  label_font.setPointSize(theme::kHistLabelFontSize);
  painter.setFont(label_font);

  for (int t = 0; t <= theme::kHistTickCount; ++t) {
    const double frac = static_cast<double>(t) / theme::kHistTickCount;
    const int y = plot_bottom - static_cast<int>(frac * plot_height);

    painter.setPen(QPen(theme::kHistGridColor, 1));
    painter.drawLine(plot_left, y, plot_right, y);

    const QString text =
        statevector ? QString::number(frac, 'g', 2)
                    : QString::number(static_cast<int>(frac * 100)) + "%";
    painter.setPen(theme::kHistLabelColor);
    painter.drawText(QRect(0, y - (theme::kHistSelectorHeight / 2),
                           theme::kHistAxisLeftPad, theme::kHistSelectorHeight),
                     Qt::AlignRight | Qt::AlignVCenter, text);
  }

  // ---- axes ----------------------------------------------------------
  painter.setPen(QPen(theme::kHistAxisColor, 1));
  painter.drawLine(plot_left, plot_top, plot_left, plot_bottom);
  painter.drawLine(plot_left, plot_bottom, plot_right, plot_bottom);

  // ---- bars ----------------------------------------------------------
  const std::size_t ref = ReferenceIndex(state);
  constexpr int step = theme::kHistBarWidth + theme::kHistBarGap;
  for (int i = 0; i < bars; ++i) {
    const double prob = std::clamp(
        state.basis_probabilities[static_cast<std::size_t>(i)], 0.0, 1.0);
    // Probabilities view plots prob; statevector view plots amplitude
    // magnitude = sqrt(prob), both on a 0..1 axis.
    const double value = statevector ? std::sqrt(prob) : prob;
    const int bar_height = static_cast<int>(value * plot_height);
    const int x = plot_left + theme::kHistBarGap + (i * step);
    const int y = plot_bottom - bar_height;

    const QColor color = statevector
                             ? PhaseColor(AmplitudePhase(
                                   state, static_cast<std::size_t>(i), ref))
                             : theme::kHistBarColor;
    painter.fillRect(QRect(x, y, theme::kHistBarWidth, bar_height), color);

    painter.setPen(theme::kHistLabelColor);
    painter.drawText(QRect(x - (theme::kHistBarGap / 2), plot_bottom + 2,
                           theme::kHistBarWidth + theme::kHistBarGap,
                           theme::kHistAxisBottomPad - 2),
                     Qt::AlignHCenter | Qt::AlignTop,
                     KetLabel(static_cast<std::size_t>(i), state.qubit_count));
  }
}

}  // namespace qde::gui
