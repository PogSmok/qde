#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QTemporaryFile>

#include "qde/gui/text_editor.hpp"

namespace qde::gui {

class TextEditorTest : public ::testing::Test {
 protected:
  void SetUp() override { editor_ = new TextEditor(); }

  void TearDown() override { delete editor_; }

  QPointer<TextEditor> editor_;
};

TEST_F(TextEditorTest, NewFileResetsState) {
  editor_->SetContent("some text");

  editor_->NewFile();

  EXPECT_TRUE(editor_->PlainText().isEmpty());
  EXPECT_FALSE(editor_->IsModified());
}

TEST_F(TextEditorTest, TypingUpdatesDocumentAndEmitsSignal) {
  QSignalSpy spy(editor_, &TextEditor::TextChanged);

  // We simulate user typing by calling the slot that handles code editor
  // changes Alternatively, we can find the CodeEditor child and set its text
  auto* inner_editor = editor_->findChild<CodeEditor*>();
  ASSERT_NE(inner_editor, nullptr);

  inner_editor->setPlainText("user typed text");

  EXPECT_EQ(spy.count(), 1);
  EXPECT_EQ(editor_->PlainText(), "user typed text");
  EXPECT_EQ(editor_->Document()->Content(), "user typed text");
  EXPECT_TRUE(editor_->IsModified());
}

TEST_F(TextEditorTest, OpenAndSaveFile) {
  QTemporaryFile temp_file;
  ASSERT_TRUE(temp_file.open());
  QString temp_path = temp_file.fileName();

  QTextStream out(&temp_file);
  out << "file content";
  temp_file.close();

  editor_->OpenFile(temp_path);
  EXPECT_EQ(editor_->PlainText(), "file content");
  EXPECT_FALSE(editor_->IsModified());
  EXPECT_EQ(editor_->FilePath(), temp_path);

  // Now edit and save
  auto* inner_editor = editor_->findChild<CodeEditor*>();
  ASSERT_NE(inner_editor, nullptr);
  inner_editor->setPlainText("modified content");

  EXPECT_TRUE(editor_->IsModified());

  EXPECT_TRUE(editor_->SaveFile());
  EXPECT_FALSE(editor_->IsModified());

  // Verify contents
  QFile file(temp_path);
  ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
  EXPECT_EQ(QString(file.readAll()), "modified content");
}

TEST_F(TextEditorTest, SaveFileAs) {
  QTemporaryFile temp_file;
  ASSERT_TRUE(temp_file.open());
  QString temp_path = temp_file.fileName();
  temp_file.close();

  editor_->SetContent("new content");

  EXPECT_TRUE(editor_->SaveFileAs(temp_path));
  EXPECT_FALSE(editor_->IsModified());
  EXPECT_EQ(editor_->FilePath(), temp_path);

  QFile file(temp_path);
  ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
  EXPECT_EQ(QString(file.readAll()), "new content");
}

TEST_F(TextEditorTest, ErrorHandling) {
  std::vector<qde::SyntaxError> errors;
  errors.push_back({1, 1, "Error 1"});

  editor_->SetContent("code\nmore code");

  editor_->SetErrors(errors);

  auto* inner_editor = editor_->findChild<CodeEditor*>();
  ASSERT_NE(inner_editor, nullptr);
  EXPECT_EQ(inner_editor->extraSelections().size(), 1);

  editor_->ClearErrors();
  EXPECT_EQ(inner_editor->extraSelections().size(), 0);
}

}  // namespace qde::gui