#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QTest>
#include "qde/gui/app_controller.hpp"
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
  QSignalSpy success_spy(controller, &AppController::parseSuccess);
  QSignalSpy error_spy(controller, &AppController::parseError);

  auto* inner_editor = text_editor->findChild<CodeEditor*>();
  ASSERT_NE(inner_editor, nullptr);
  inner_editor->setPlainText("OPENQASM 3.0;\nqreg q[1];");

  controller->parseNow();

  EXPECT_EQ(success_spy.count(), 1);
  EXPECT_EQ(error_spy.count(), 0);
  EXPECT_NE(controller->circuit(), nullptr);
}

TEST_F(AppControllerTest, ParseNowError) {
  QSignalSpy success_spy(controller, &AppController::parseSuccess);
  QSignalSpy error_spy(controller, &AppController::parseError);

  auto* inner_editor = text_editor->findChild<CodeEditor*>();
  ASSERT_NE(inner_editor, nullptr);
  inner_editor->setPlainText("INVALID SYNTAX");

  controller->parseNow();

  EXPECT_EQ(success_spy.count(), 0);
  EXPECT_EQ(error_spy.count(), 1);
  EXPECT_EQ(controller->circuit(), nullptr);
}

TEST_F(AppControllerTest, DebounceTimerWorks) {
  QSignalSpy success_spy(controller, &AppController::parseSuccess);

  auto* inner_editor = text_editor->findChild<CodeEditor*>();
  ASSERT_NE(inner_editor, nullptr);

  // Type something
  inner_editor->setPlainText("OPENQASM 3.0;\nqreg q[1];");

  // The signal textChanged triggers debounce timer (400ms).
  EXPECT_EQ(success_spy.count(), 0);  // It should not parse instantly.
  EXPECT_TRUE(success_spy.wait(2000));
  EXPECT_EQ(success_spy.count(), 1);  // It should be parsed.
}

}  // namespace qde::gui