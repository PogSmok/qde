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
    circuitView = new QuantumCircuitView();
    textEditor = new TextEditor();
    controller = new AppController(circuitView, textEditor);
  }

  void TearDown() override {
    delete controller;
    delete textEditor;
    delete circuitView;
  }

  QuantumCircuitView* circuitView;
  TextEditor* textEditor;
  AppController* controller;
};

TEST_F(AppControllerTest, ParseNowSuccess) {
  QSignalSpy successSpy(controller, &AppController::parseSuccess);
  QSignalSpy errorSpy(controller, &AppController::parseError);

  CodeEditor* innerEditor = textEditor->findChild<CodeEditor*>();
  ASSERT_NE(innerEditor, nullptr);
  innerEditor->setPlainText("OPENQASM 3.0;\nqreg q[1];");

  controller->parseNow();

  EXPECT_EQ(successSpy.count(), 1);
  EXPECT_EQ(errorSpy.count(), 0);
  EXPECT_NE(controller->circuit(), nullptr);
}

TEST_F(AppControllerTest, ParseNowError) {
  QSignalSpy successSpy(controller, &AppController::parseSuccess);
  QSignalSpy errorSpy(controller, &AppController::parseError);

  CodeEditor* innerEditor = textEditor->findChild<CodeEditor*>();
  ASSERT_NE(innerEditor, nullptr);
  innerEditor->setPlainText("INVALID SYNTAX");

  controller->parseNow();

  EXPECT_EQ(successSpy.count(), 0);
  EXPECT_EQ(errorSpy.count(), 1);
  EXPECT_EQ(controller->circuit(), nullptr);
}

TEST_F(AppControllerTest, DebounceTimerWorks) {
  QSignalSpy successSpy(controller, &AppController::parseSuccess);

  CodeEditor* innerEditor = textEditor->findChild<CodeEditor*>();
  ASSERT_NE(innerEditor, nullptr);

  // Type something
  innerEditor->setPlainText("OPENQASM 3.0;\nqreg q[1];");

  // The signal textChanged triggers debounce timer (400ms).
  // It should not parse instantly.
  EXPECT_EQ(successSpy.count(), 0);

  // Wait slightly more than 400ms
  QTest::qWait(450);

  // Now it should have parsed
  EXPECT_EQ(successSpy.count(), 1);
}

}  // namespace qde::gui