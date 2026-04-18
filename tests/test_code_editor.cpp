#include <gtest/gtest.h>
#include "qde/qt/code_editor.hpp"

class CodeEditorTest : public ::testing::Test {
protected:
 void SetUp() override {
  editor = new CodeEditor();
 }

 void TearDown() override {
  delete editor;
 }

 CodeEditor* editor;
};

TEST_F(CodeEditorTest, SetClearErrors) {
 std::vector<qde::SyntaxError> errors;
 errors.push_back({1, 1, "Unexpected token"});
 errors.push_back({2, 5, "Missing semicolon"});

 // Need to have some text to select
 editor->setPlainText("line 1\nline 2");

 editor->setErrors(errors);
 
 QList<QTextEdit::ExtraSelection> selections = editor->extraSelections();
 EXPECT_EQ(selections.size(), 2);
 
 if (selections.size() == 2) {
  EXPECT_EQ(selections[0].format.underlineStyle(), QTextCharFormat::SpellCheckUnderline);
  EXPECT_EQ(selections[0].format.underlineColor(), Qt::red);
  EXPECT_EQ(selections[0].format.toolTip(), "Unexpected token");
 }

 editor->clearErrors();
 selections = editor->extraSelections();
 EXPECT_TRUE(selections.isEmpty());
}