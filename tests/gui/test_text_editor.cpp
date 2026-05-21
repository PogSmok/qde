#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QTemporaryFile>

#include "qde/gui/config.hpp"
#include "qde/gui/text_editor.hpp"

namespace qde::gui {

class TextEditorTest : public ::testing::Test {
 protected:
  void SetUp() override { editor_ = new TextEditor(); }

  void TearDown() override { delete editor_; }

  void SetSelection(int start, int end) {
    QTextCursor cursor(editor_->Document());
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    editor_->Editor()->setTextCursor(cursor);
  }

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
  EXPECT_EQ(editor_->Document()->toPlainText(), "user typed text");
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

TEST_F(TextEditorTest, CommentSingleLine) {
  editor_->SetContent("line of code");
  SetSelection(0, 0);
  editor_->ToggleComment();
  EXPECT_EQ(editor_->PlainText(), "// line of code");
}

TEST_F(TextEditorTest, UncommentSingleLine) {
  editor_->SetContent("// line of code");
  SetSelection(0, 0);
  editor_->ToggleComment();
  EXPECT_EQ(editor_->PlainText(), "line of code");
}

TEST_F(TextEditorTest, UncommentSingleLineNoSpace) {
  editor_->SetContent("//line of code");
  SetSelection(0, 0);
  editor_->ToggleComment();
  EXPECT_EQ(editor_->PlainText(), "line of code");
}

TEST_F(TextEditorTest, CommentMultipleLines) {
  editor_->SetContent("1. line\n2. line");
  SetSelection(0, 10);
  editor_->ToggleComment();
  EXPECT_EQ(editor_->PlainText(), "// 1. line\n// 2. line");
}

TEST_F(TextEditorTest, UncommentMultipleLines) {
  editor_->SetContent("// 1. line\n// 2. line");
  SetSelection(0, 14);
  editor_->ToggleComment();
  EXPECT_EQ(editor_->PlainText(), "1. line\n2. line");
}

TEST_F(TextEditorTest, IndentSingleLine) {
  editor_->SetContent("line of code");
  SetSelection(0, 0);
  editor_->IndentBlock();
  const bool useSpaces = config::editor::useSpaces.Value();
  const int tabWidth = config::editor::tabWidth.Value();
  const QString indent = useSpaces ? QString(tabWidth, ' ') : QString('\t');
  EXPECT_EQ(editor_->PlainText(), indent + "line of code");
}

TEST_F(TextEditorTest, OutdentSingleLine) {
  const bool useSpaces = config::editor::useSpaces.Value();
  const int tabWidth = config::editor::tabWidth.Value();
  const QString indent = useSpaces ? QString(tabWidth, ' ') : QString('\t');
  editor_->SetContent(indent + "line of code");
  SetSelection(0, 0);
  editor_->OutdentBlock();
  EXPECT_EQ(editor_->PlainText(), "line of code");
}

TEST_F(TextEditorTest, IndentMultipleLines) {
  editor_->SetContent("1. line\n2. line");
  SetSelection(0, 10);
  editor_->IndentBlock();
  const bool useSpaces = config::editor::useSpaces.Value();
  const int tabWidth = config::editor::tabWidth.Value();
  const QString indent = useSpaces ? QString(tabWidth, ' ') : QString('\t');
  EXPECT_EQ(editor_->PlainText(), indent + "1. line\n" + indent + "2. line");
}

TEST_F(TextEditorTest, OutdentMultipleLines) {
  const bool useSpaces = config::editor::useSpaces.Value();
  const int tabWidth = config::editor::tabWidth.Value();
  const QString indent = useSpaces ? QString(tabWidth, ' ') : QString('\t');
  editor_->SetContent(indent + "1. line\n" + indent + "2. line");
  SetSelection(0, 10 + tabWidth);
  editor_->OutdentBlock();
  EXPECT_EQ(editor_->PlainText(), "1. line\n2. line");
}

TEST_F(TextEditorTest, MoveSingleLineDown) {
  editor_->SetContent(
      "first line\n"
      "second line\n"
      "third line");
  SetSelection(0, 0);
  editor_->MoveBlockDown();
  EXPECT_EQ(editor_->PlainText(),
            "second line\n"
            "first line\n"
            "third line");
}

TEST_F(TextEditorTest, MoveMultipleLinesDown) {
  editor_->SetContent(
      "first line\n"
      "second line\n"
      "third line");
  SetSelection(0, 14);
  editor_->MoveBlockDown();
  EXPECT_EQ(editor_->PlainText(),
            "third line\n"
            "first line\n"
            "second line");
}

TEST_F(TextEditorTest, MoveLineDownEndOfFile) {
  editor_->SetContent(
      "first line\n"
      "second line\n"
      "third line");
  SetSelection(30, 31);
  editor_->MoveBlockDown();
  EXPECT_EQ(editor_->PlainText(),
            "first line\n"
            "second line\n"
            "third line");
}

TEST_F(TextEditorTest, MoveLineUpBegginingOfFile) {
  editor_->SetContent(
      "first line\n"
      "second line\n"
      "third line");
  SetSelection(0, 0);
  editor_->MoveBlockUp();
  EXPECT_EQ(editor_->PlainText(),
            "first line\n"
            "second line\n"
            "third line");
}

TEST_F(TextEditorTest, MoveMultipleLinesUp) {
  editor_->SetContent(
      "first line\n"
      "second line\n"
      "third line");
  SetSelection(14, 32);
  editor_->MoveBlockUp();
  EXPECT_EQ(editor_->PlainText(),
            "second line\n"
            "third line\n"
            "first line");
}

TEST_F(TextEditorTest, MoveSingleLineUp) {
  editor_->SetContent(
      "first line\n"
      "second line\n"
      "third line");
  SetSelection(14, 14);
  editor_->MoveBlockUp();
  EXPECT_EQ(editor_->PlainText(),
            "second line\n"
            "first line\n"
            "third line");
}

}  // namespace qde::gui