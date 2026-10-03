#include <gtest/gtest.h>

#include <cstddef>
#include <utility>
#include <vector>

#include <QComboBox>
#include <QImage>
#include <QScrollArea>
#include <QScrollBar>
#include <QWidget>

#include "qde/gui/simulation_results_view.hpp"
#include "qde/gui/theme.hpp"
#include "qde/simulator/simulation_state.hpp"

namespace qde::gui {

namespace {

qde::SimulationState MakeState(std::size_t qubit_count,
                               std::vector<double> probabilities) {
  qde::SimulationState state;
  state.qubit_count = qubit_count;
  state.basis_probabilities = std::move(probabilities);
  const std::size_t dim = state.basis_probabilities.size();
  state.density_matrix.assign(dim * dim, {0.0, 0.0});
  for (std::size_t i = 0; i < dim; ++i) {
    state.density_matrix[(i * dim) + i] = {state.basis_probabilities[i], 0.0};
  }
  return state;
}

QImage RenderWidget(QWidget* widget) {
  widget->resize(widget->sizeHint());
  QImage image(widget->size(), QImage::Format_ARGB32);
  image.fill(Qt::black);
  widget->render(&image);
  return image;
}

}  // namespace

class SimulationResultsViewTest : public ::testing::Test {
 protected:
  void SetUp() override { view_ = new SimulationResultsView(); }
  void TearDown() override { delete view_; }

  SimulationResultsView* view_{};
};

TEST_F(SimulationResultsViewTest, SizeHintDefault) {
  EXPECT_EQ(view_->sizeHint(),
            QSize(theme::kHistDefaultWidth, theme::kHistDefaultHeight));
}

TEST_F(SimulationResultsViewTest, ShowStateSetsMinimumWidth) {
  view_->ShowState(MakeState(/*qubit_count=*/2, {0.5, 0.0, 0.0, 0.5}));

  EXPECT_EQ(view_->minimumWidth(), view_->sizeHint().width());
  EXPECT_EQ(view_->width(), view_->sizeHint().width());
}

TEST_F(SimulationResultsViewTest, ShowStateWidthScalesWithStateCount) {
  view_->ShowState(MakeState(/*qubit_count=*/1, {0.5, 0.5}));
  const QSize one_qubit_hint = view_->sizeHint();

  view_->ShowState(MakeState(/*qubit_count=*/3, std::vector<double>(8, 0.125)));
  const QSize three_qubit_hint = view_->sizeHint();

  EXPECT_LE(one_qubit_hint.width(), three_qubit_hint.width());
  EXPECT_EQ(one_qubit_hint.height(), three_qubit_hint.height());
}

TEST_F(SimulationResultsViewTest, ClearStateRestoresDefault) {
  view_->ShowState(MakeState(/*qubit_count=*/3, std::vector<double>(8, 0.125)));
  view_->ClearState();

  EXPECT_EQ(view_->sizeHint(),
            QSize(theme::kHistDefaultWidth, theme::kHistDefaultHeight));
  EXPECT_EQ(view_->minimumWidth(), theme::kHistDefaultWidth);
  EXPECT_EQ(view_->width(), theme::kHistDefaultWidth);
}

TEST_F(SimulationResultsViewTest, ShowStateWithZeroQubits) {
  view_->ShowState(MakeState(/*qubit_count=*/0, {1.0}));

  EXPECT_EQ(view_->minimumWidth(), view_->sizeHint().width());
}

TEST(SimulationResultsViewScrollTest, WideHistogramScrollsInsideScrollArea) {
  QScrollArea area;
  area.setWidgetResizable(true);
  auto* view = new SimulationResultsView();
  area.setWidget(view);
  area.resize(theme::kHistDefaultWidth, theme::kHistDefaultHeight * 2);
  area.show();

  view->ShowState(MakeState(
      /*qubit_count=*/6,
      std::vector<double>(theme::kHistMaxStates, 1.0 / theme::kHistMaxStates)));

  EXPECT_GE(view->width(), view->sizeHint().width());
  EXPECT_GT(area.horizontalScrollBar()->maximum(), 0);
}

TEST_F(SimulationResultsViewTest, TooManyStatesFallsBackToDefaultSize) {
  view_->ShowState(MakeState(/*qubit_count=*/7, std::vector<double>(128, 0.0)));

  EXPECT_EQ(view_->sizeHint(),
            QSize(theme::kHistDefaultWidth, theme::kHistDefaultHeight));
}

TEST_F(SimulationResultsViewTest, ComboAndModeStayInSync) {
  auto* combo = view_->findChild<QComboBox*>();
  ASSERT_NE(combo, nullptr);

  combo->setCurrentIndex(1);
  EXPECT_EQ(view_->GetMode(), SimulationResultsView::Mode::kStatevector);

  view_->SetMode(SimulationResultsView::Mode::kProbabilities);
  EXPECT_EQ(combo->currentIndex(), 0);
}

TEST_F(SimulationResultsViewTest, RenderProbabilitiesProducesImage) {
  view_->ShowState(MakeState(/*qubit_count=*/2, {0.5, 0.0, 0.0, 0.5}));
  const QImage image = RenderWidget(view_);

  EXPECT_FALSE(image.isNull());
}

TEST_F(SimulationResultsViewTest, RenderStatevectorProducesImage) {
  view_->ShowState(MakeState(/*qubit_count=*/1, {0.5, 0.5}));
  view_->SetMode(SimulationResultsView::Mode::kStatevector);
  const QImage image = RenderWidget(view_);

  EXPECT_FALSE(image.isNull());
}

TEST_F(SimulationResultsViewTest, RenderTooManyStatesProducesImage) {
  view_->ShowState(MakeState(/*qubit_count=*/7, std::vector<double>(128, 0.0)));
  const QImage image = RenderWidget(view_);

  EXPECT_FALSE(image.isNull());
}

TEST_F(SimulationResultsViewTest, RenderClearedProducesImage) {
  view_->ShowState(MakeState(/*qubit_count=*/1, {1.0, 0.0}));
  view_->ClearState();
  const QImage image = RenderWidget(view_);

  EXPECT_FALSE(image.isNull());
}

TEST_F(SimulationResultsViewTest, DefaultModeIsProbabilities) {
  EXPECT_EQ(view_->GetMode(), SimulationResultsView::Mode::kProbabilities);
}

TEST_F(SimulationResultsViewTest, SetModeSwitchesView) {
  view_->SetMode(SimulationResultsView::Mode::kStatevector);
  EXPECT_EQ(view_->GetMode(), SimulationResultsView::Mode::kStatevector);

  view_->SetMode(SimulationResultsView::Mode::kProbabilities);
  EXPECT_EQ(view_->GetMode(), SimulationResultsView::Mode::kProbabilities);
}

TEST_F(SimulationResultsViewTest, ModeDoesNotAffectSizeHint) {
  view_->ShowState(MakeState(/*qubit_count=*/2, {0.5, 0.0, 0.0, 0.5}));
  const QSize prob_hint = view_->sizeHint();

  view_->SetMode(SimulationResultsView::Mode::kStatevector);

  EXPECT_EQ(view_->sizeHint(), prob_hint);
}

}  // namespace qde::gui
