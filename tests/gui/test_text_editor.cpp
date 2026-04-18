#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QTemporaryFile>

#include "qde/gui/text_editor.hpp"

namespace qde::gui {

class TextEditorTest : public ::testing::Test {
 protected:
  void SetUp() override { editor = new TextEditor(); }

  void TearDown() override { delete editor; }

  QPointer<TextEditor> editor;
};

TEST_F(TextEditorTest, NewFileResetsState) {
  editor->document()->setContent("some text");
  editor->document()->setModified(true);

  editor->newFile();

  EXPECT_TRUE(editor->plainText().isEmpty());
  EXPECT_FALSE(editor->isModified());
}

TEST_F(TextEditorTest, TypingUpdatesDocumentAndEmitsSignal) {
  QSignalSpy spy(editor, &TextEditor::textChanged);

  // We simulate user typing by calling the slot that handles code editor
  // changes Alternatively, we can find the CodeEditor child and set its text
  CodeEditor* innerEditor = editor->findChild<CodeEditor*>();
  ASSERT_NE(innerEditor, nullptr);

  innerEditor->setPlainText("user typed text");

  EXPECT_EQ(spy.count(), 1);
  EXPECT_EQ(editor->plainText(), "user typed text");
  EXPECT_EQ(editor->document()->content(), "user typed text");
  EXPECT_TRUE(editor->isModified());
}

TEST_F(TextEditorTest, OpenAndSaveFile) {
  QTemporaryFile tempFile;
  ASSERT_TRUE(tempFile.open());
  QString tempPath = tempFile.fileName();

  QTextStream out(&tempFile);
  out << "file content";
  tempFile.close();

  editor->openFile(tempPath);
  EXPECT_EQ(editor->plainText(), "file content");
  EXPECT_FALSE(editor->isModified());
  EXPECT_EQ(editor->filePath(), tempPath);

  // Now edit and save
  CodeEditor* innerEditor = editor->findChild<CodeEditor*>();
  ASSERT_NE(innerEditor, nullptr);
  innerEditor->setPlainText("modified content");

  EXPECT_TRUE(editor->isModified());

  EXPECT_TRUE(editor->saveFile());
  EXPECT_FALSE(editor->isModified());

  // Verify contents
  QFile file(tempPath);
  ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
  EXPECT_EQ(QString(file.readAll()), "modified content");
}

TEST_F(TextEditorTest, SaveFileAs) {
  QTemporaryFile tempFile;
  ASSERT_TRUE(tempFile.open());
  QString tempPath = tempFile.fileName();
  tempFile.close();

  editor->document()->setContent("new content");
  editor->syncEditorToDoc();

  EXPECT_TRUE(editor->saveFileAs(tempPath));
  EXPECT_FALSE(editor->isModified());
  EXPECT_EQ(editor->filePath(), tempPath);

  QFile file(tempPath);
  ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
  EXPECT_EQ(QString(file.readAll()), "new content");
}

TEST_F(TextEditorTest, ErrorHandling) {
  std::vector<qde::SyntaxError> errors;
  errors.push_back({1, 1, "Error 1"});

  editor->document()->setContent("code\nmore code");
  editor->syncEditorToDoc();

  editor->setErrors(errors);

  CodeEditor* innerEditor = editor->findChild<CodeEditor*>();
  ASSERT_NE(innerEditor, nullptr);
  EXPECT_EQ(innerEditor->extraSelections().size(), 1);

  editor->clearErrors();
  EXPECT_EQ(innerEditor->extraSelections().size(), 0);
}

}  // namespace qde::gui