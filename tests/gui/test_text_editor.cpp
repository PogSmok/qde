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
  editor_->setContent("some text");

  editor_->newFile();

  EXPECT_TRUE(editor_->plainText().isEmpty());
  EXPECT_FALSE(editor_->isModified());
}

TEST_F(TextEditorTest, TypingUpdatesDocumentAndEmitsSignal) {
  QSignalSpy spy(editor_, &TextEditor::textChanged);

  // We simulate user typing by calling the slot that handles code editor
  // changes Alternatively, we can find the CodeEditor child and set its text
  auto* inner_editor = editor_->findChild<CodeEditor*>();
  ASSERT_NE(inner_editor, nullptr);

  inner_editor->setPlainText("user typed text");

  EXPECT_EQ(spy.count(), 1);
  EXPECT_EQ(editor_->plainText(), "user typed text");
  EXPECT_EQ(editor_->document()->content(), "user typed text");
  EXPECT_TRUE(editor_->isModified());
}

TEST_F(TextEditorTest, OpenAndSaveFile) {
  QTemporaryFile temp_file;
  ASSERT_TRUE(temp_file.open());
  QString temp_path = temp_file.fileName();

  QTextStream out(&temp_file);
  out << "file content";
  temp_file.close();

  editor_->openFile(temp_path);
  EXPECT_EQ(editor_->plainText(), "file content");
  EXPECT_FALSE(editor_->isModified());
  EXPECT_EQ(editor_->filePath(), temp_path);

  // Now edit and save
  auto* inner_editor = editor_->findChild<CodeEditor*>();
  ASSERT_NE(inner_editor, nullptr);
  inner_editor->setPlainText("modified content");

  EXPECT_TRUE(editor_->isModified());

  EXPECT_TRUE(editor_->saveFile());
  EXPECT_FALSE(editor_->isModified());

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

  editor_->setContent("new content");

  EXPECT_TRUE(editor_->saveFileAs(temp_path));
  EXPECT_FALSE(editor_->isModified());
  EXPECT_EQ(editor_->filePath(), temp_path);

  QFile file(temp_path);
  ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
  EXPECT_EQ(QString(file.readAll()), "new content");
}

TEST_F(TextEditorTest, ErrorHandling) {
  std::vector<qde::SyntaxError> errors;
  errors.push_back({1, 1, "Error 1"});

  editor_->setContent("code\nmore code");

  editor_->setErrors(errors);

  auto* inner_editor = editor_->findChild<CodeEditor*>();
  ASSERT_NE(inner_editor, nullptr);
  EXPECT_EQ(inner_editor->extraSelections().size(), 1);

  editor_->clearErrors();
  EXPECT_EQ(inner_editor->extraSelections().size(), 0);
}

}  // namespace qde::gui