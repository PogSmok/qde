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
    mainWindow_ = new MainWindow();
    mainWindow_->show();
    // Allow event loop to process show events
    QApplication::processEvents();
  }

  void TearDown() override { delete mainWindow_; }

  QPointer<MainWindow> mainWindow_;
};

TEST_F(MainWindowTest, Initialization) {
  EXPECT_EQ(mainWindow_->windowTitle(), QString("QDE"));

  auto* splitter = mainWindow_->findChild<QSplitter*>();
  ASSERT_NE(splitter, nullptr);
  EXPECT_EQ(splitter->count(), 2);

  auto* editor = mainWindow_->findChild<TextEditor*>();
  ASSERT_NE(editor, nullptr);

  auto* circuit_view = mainWindow_->findChild<QuantumCircuitView*>();
  ASSERT_NE(circuit_view, nullptr);
}

TEST_F(MainWindowTest, UpdateTitleOnModifiedChanged) {
  auto* editor = mainWindow_->findChild<TextEditor*>();
  ASSERT_NE(editor, nullptr);

  QString unmodified_title = mainWindow_->windowTitle();
  EXPECT_FALSE(unmodified_title.endsWith("*"));

  // Trigger modified
  editor->setContent("Modified");

  QString modified_title = mainWindow_->windowTitle();
  EXPECT_TRUE(modified_title.endsWith("*"));
}

TEST_F(MainWindowTest, OnParseSuccessUpdatesStatus) {
  mainWindow_->onParseSuccess();
  auto* status_label = mainWindow_->findChild<QLabel*>();
  ASSERT_NE(status_label, nullptr);

  EXPECT_EQ(status_label->text(), "Parsed successfully");
  EXPECT_TRUE(status_label->styleSheet().contains(theme::kSuccessText));
}

TEST_F(MainWindowTest, OnParseFailUpdatesStatus) {
  mainWindow_->onParseFail({"Error 1", "Error 2"});
  auto* status_label = mainWindow_->findChild<QLabel*>();
  ASSERT_NE(status_label, nullptr);

  EXPECT_EQ(status_label->text(), "Syntax Error: 2 errors");
  EXPECT_TRUE(status_label->styleSheet().contains(theme::kErrorText));
}

TEST_F(MainWindowTest, NewFileWhenUnmodified) {
  auto* editor = mainWindow_->findChild<TextEditor*>();
  ASSERT_NE(editor, nullptr);

  // newFile should clear the editor if unmodified without blocking prompt
  mainWindow_->newFile();
  EXPECT_TRUE(editor->plainText().isEmpty());
}

TEST_F(MainWindowTest, CloseEventWhenUnmodified) {
  auto* editor = mainWindow_->findChild<TextEditor*>();
  ASSERT_NE(editor, nullptr);

  // If we close when unmodified, it shouldn't block on QMessageBox
  // So we just simulate a close event
  QCloseEvent event;
  QApplication::sendEvent(mainWindow_, &event);

  EXPECT_TRUE(event.isAccepted());
}

}  // namespace qde::gui