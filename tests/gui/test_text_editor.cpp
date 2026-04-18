#include <gtest/gtest.h>
#include <QSignalSpy>
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

}  // namespace qde::gui