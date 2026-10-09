#include <gtest/gtest.h>

#include <QSignalSpy>
#include <QTest>

#include "qde/gui/app_controller.hpp"
#include "qde/gui/code_editor.hpp"
#include "qde/gui/quantum_circuit_view.hpp"
#include "qde/gui/text_editor.hpp"

namespace qde::gui {

class AppControllerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    circuit_view = new QuantumCircuitView();
    text_editor = new TextEditor();
    controller = new AppController(circuit_view, text_editor);
  }

  void TearDown() override {
    delete controller;
    delete text_editor;
    delete circuit_view;
  }

 public:
  QPointer<QuantumCircuitView> circuit_view;
  QPointer<TextEditor> text_editor;
  QPointer<AppController> controller;
};

TEST_F(AppControllerTest, ParseNowSuccess) {
  QSignalSpy success_spy(controller, &AppController::ParseSuccess);
  QSignalSpy error_spy(controller, &AppController::ParseError);

  auto* inner_editor = text_editor->findChild<CodeEditor*>();
  ASSERT_NE(inner_editor, nullptr);
  inner_editor->setPlainText("OPENQASM 3.0;\nqreg q[1];");

  controller->ParseNow();

  EXPECT_EQ(success_spy.count(), 1);
  EXPECT_EQ(error_spy.count(), 0);
  EXPECT_NE(controller->GetCircuit(), nullptr);
}

TEST_F(AppControllerTest, ParseNowError) {
  QSignalSpy success_spy(controller, &AppController::ParseSuccess);
  QSignalSpy error_spy(controller, &AppController::ParseError);

  auto* inner_editor = text_editor->findChild<CodeEditor*>();
  ASSERT_NE(inner_editor, nullptr);
  inner_editor->setPlainText("INVALID SYNTAX");

  controller->ParseNow();

  EXPECT_EQ(success_spy.count(), 0);
  EXPECT_EQ(error_spy.count(), 1);
  EXPECT_EQ(controller->GetCircuit(), nullptr);
}

TEST_F(AppControllerTest, ParseErrorClearsCircuitView) {
  auto* inner_editor = text_editor->findChild<CodeEditor*>();
  ASSERT_NE(inner_editor, nullptr);

  inner_editor->setPlainText("OPENQASM 3.0;\nqreg q[2];\nh q[0];\nx q[1];");
  controller->ParseNow();
  ASSERT_NE(controller->GetCircuit(), nullptr);
  ASSERT_NE(circuit_view->sizeHint(), QSize(400, 160));

  inner_editor->setPlainText("INVALID SYNTAX");
  controller->ParseNow();

  EXPECT_EQ(controller->GetCircuit(), nullptr);
  EXPECT_EQ(circuit_view->sizeHint(), QSize(400, 160));
}

TEST_F(AppControllerTest, DebounceTimerWorks) {
  QSignalSpy success_spy(controller, &AppController::ParseSuccess);

  auto* inner_editor = text_editor->findChild<CodeEditor*>();
  ASSERT_NE(inner_editor, nullptr);

  inner_editor->setPlainText("OPENQASM 3.0;\nqreg q[1];");

  EXPECT_EQ(success_spy.count(), 0);
  EXPECT_TRUE(success_spy.wait(2000));
  EXPECT_EQ(success_spy.count(), 1);
}

TEST_F(AppControllerTest, ParseNowEmitsSimulationComplete) {
  QSignalSpy sim_spy(controller, &AppController::SimulationComplete);

  auto* inner_editor = text_editor->findChild<CodeEditor*>();
  ASSERT_NE(inner_editor, nullptr);
  inner_editor->setPlainText("OPENQASM 3.0;\nqreg q[1];\nh q[0];");

  controller->ParseNow();

  EXPECT_EQ(sim_spy.count(), 1);
  ASSERT_NE(controller->GetSimulationState(), nullptr);
  EXPECT_EQ(controller->GetSimulationState()->Dim(), 2U);
}

TEST_F(AppControllerTest, SimulationIgnoresMeasurements) {
  // A measured H qubit would collapse to a single 100% outcome if measurements
  // were simulated. Like IBM Composer, the visualization must ignore the
  // measurement and show the coherent 50/50 superposition.
  auto* inner_editor = text_editor->findChild<CodeEditor*>();
  ASSERT_NE(inner_editor, nullptr);
  inner_editor->setPlainText("qubit q;\nbit c;\nh q;\nmeasure q -> c;");

  controller->ParseNow();

  const auto* state = controller->GetSimulationState();
  ASSERT_NE(state, nullptr);
  ASSERT_EQ(state->Dim(), 2U);
  EXPECT_NEAR(state->Probability(0), 0.5, 1e-9);
  EXPECT_NEAR(state->Probability(1), 0.5, 1e-9);
}

}  // namespace qde::gui
