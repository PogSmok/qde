#include <gtest/gtest.h>
#include "qde/gui/code_editor.hpp"

namespace qde::gui {

class CodeEditorTest : public ::testing::Test {
 protected:
  void SetUp() override { editor = new CodeEditor(); }

  void TearDown() override { delete editor; }

 public:
  QPointer<CodeEditor> editor;
};

TEST_F(CodeEditorTest, SetClearErrors) {
  std::vector<qde::SyntaxError> errors;
  errors.push_back({1, 1, "Unexpected token"});
  errors.push_back({2, 5, "Missing semicolon"});

  // Need to have some text to select
  editor->setPlainText("line 1\nline 2");

  editor->SetErrors(errors);

  QList<QTextEdit::ExtraSelection> selections = editor->extraSelections();
  EXPECT_EQ(selections.size(), 2);

  if (selections.size() == 2) {
    EXPECT_EQ(selections[0].format.underlineStyle(),
              QTextCharFormat::SpellCheckUnderline);
    EXPECT_EQ(selections[0].format.underlineColor(), Qt::red);
    EXPECT_EQ(selections[0].format.toolTip(), "Unexpected token");
  }

  editor->ClearErrors();
  selections = editor->extraSelections();
  EXPECT_TRUE(selections.isEmpty());
}

TEST_F(CodeEditorTest, GeometryAndLineNumbers) {
  // Show editor to ensure geometries are updated
  editor->show();

  // Set multi-line text to ensure block count goes above 10 for width
  // adjustment
  QString text;
  for (int i = 0; i < 20; ++i) {
    text += "line\n";
  }
  editor->setPlainText(text);

  // Force a resize event
  editor->resize(400, 400);

  // Width should increase with digits
  EXPECT_GT(editor->LineNumberAreaWidth(), 0);
}

}  // namespace qde::gui