#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QTest>

#include "qde/gui/app_controller.hpp"
#include "qde/gui/main_window.hpp"
#include "qde/gui/quantum_circuit_view.hpp"
#include "qde/gui/text_editor.hpp"
#include "qde/gui/theme.hpp"

namespace qde::gui {

class MainWindowTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mainWindow = new MainWindow();
    mainWindow->show();
    // Allow event loop to process show events
    QApplication::processEvents();
  }

  void TearDown() override { delete mainWindow; }

  QPointer<MainWindow> mainWindow;
};

TEST_F(MainWindowTest, Initialization) {
  EXPECT_EQ(mainWindow->windowTitle(), QString("QDE"));

  auto* splitter = mainWindow->findChild<QSplitter*>();
  ASSERT_NE(splitter, nullptr);
  EXPECT_EQ(splitter->count(), 2);

  auto* editor = mainWindow->findChild<TextEditor*>();
  ASSERT_NE(editor, nullptr);

  auto* circuitView = mainWindow->findChild<QuantumCircuitView*>();
  ASSERT_NE(circuitView, nullptr);
}

TEST_F(MainWindowTest, UpdateTitleOnModifiedChanged) {
  auto* editor = mainWindow->findChild<TextEditor*>();
  ASSERT_NE(editor, nullptr);

  QString unmodifiedTitle = mainWindow->windowTitle();
  EXPECT_FALSE(unmodifiedTitle.endsWith("*"));

  // Trigger modified
  editor->setContent("Modified");

  QString modifiedTitle = mainWindow->windowTitle();
  EXPECT_TRUE(modifiedTitle.endsWith("*"));
}

TEST_F(MainWindowTest, OnParseSuccessUpdatesStatus) {
  mainWindow->onParseSuccess();
  auto* statusLabel = mainWindow->findChild<QLabel*>();
  ASSERT_NE(statusLabel, nullptr);

  EXPECT_EQ(statusLabel->text(), "Parsed successfully");
  EXPECT_TRUE(statusLabel->styleSheet().contains(theme::kSuccessText));
}

TEST_F(MainWindowTest, OnParseFailUpdatesStatus) {
  mainWindow->onParseFail({"Error 1", "Error 2"});
  auto* statusLabel = mainWindow->findChild<QLabel*>();
  ASSERT_NE(statusLabel, nullptr);

  EXPECT_EQ(statusLabel->text(), "Syntax Error: 2 errors");
  EXPECT_TRUE(statusLabel->styleSheet().contains(theme::kErrorText));
}

TEST_F(MainWindowTest, NewFileWhenUnmodified) {
  auto* editor = mainWindow->findChild<TextEditor*>();
  ASSERT_NE(editor, nullptr);

  // newFile should clear the editor if unmodified without blocking prompt
  mainWindow->newFile();
  EXPECT_TRUE(editor->plainText().isEmpty());
}

TEST_F(MainWindowTest, CloseEventWhenUnmodified) {
  auto* editor = mainWindow->findChild<TextEditor*>();
  ASSERT_NE(editor, nullptr);

  // If we close when unmodified, it shouldn't block on QMessageBox
  // So we just simulate a close event
  QCloseEvent event;
  QApplication::sendEvent(mainWindow, &event);

  EXPECT_TRUE(event.isAccepted());
}

}  // namespace qde::gui