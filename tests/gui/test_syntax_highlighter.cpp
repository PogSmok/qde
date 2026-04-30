#include <gtest/gtest.h>
#include <QTextDocument>

#include "qde/gui/syntax_highlighter.hpp"

namespace qde::gui {

class SyntaxHighlighterTest : public ::testing::Test {
 protected:
  void SetUp() override { highlighter_ = new SyntaxHighlighter(&doc_); }

  void TearDown() override { delete highlighter_; }

  // Sets text and forces a synchronous rehighlight.
  void highlight(const QString& text) {
    doc_.setPlainText(text);
    highlighter_->rehighlight();
  }

  QTextBlock lineAt(int n) const { return doc_.findBlockByLineNumber(n); }

  QTextDocument doc_;
  SyntaxHighlighter* highlighter_ = nullptr;
};

// --- Block comment state -----------------------------------------------------

TEST_F(SyntaxHighlighterTest, NormalCodeHasCodeState) {
  highlight("gate h q { }");
  EXPECT_EQ(lineAt(0).userState(), 0);
}

TEST_F(SyntaxHighlighterTest, UnclosedBlockCommentSetsCommentedState) {
  highlight("/* comment");
  EXPECT_EQ(lineAt(0).userState(), 1);
}

TEST_F(SyntaxHighlighterTest, ClosedBlockCommentResetsState) {
  highlight("/* comment */");
  EXPECT_EQ(lineAt(0).userState(), 0);
}

TEST_F(SyntaxHighlighterTest, BlockCommentSpansMultipleLines) {
  highlight("/* start\ncontinued\n*/\ncode");
  EXPECT_EQ(lineAt(0).userState(), 1);  // comment opened
  EXPECT_EQ(lineAt(1).userState(), 1);  // still inside
  EXPECT_EQ(lineAt(2).userState(), 0);  // closed by */
  EXPECT_EQ(lineAt(3).userState(), 0);  // normal code after
}

TEST_F(SyntaxHighlighterTest, LineCommentDoesNotAffectBlockState) {
  highlight("// comment\ncode");
  EXPECT_EQ(lineAt(0).userState(), 0);
  EXPECT_EQ(lineAt(1).userState(), 0);
}

TEST_F(SyntaxHighlighterTest, MultipleBlockCommentsOnOneLine) {
  highlight("/* a */ code /* b */\nnormal");
  EXPECT_EQ(lineAt(0).userState(), 0);
  EXPECT_EQ(lineAt(1).userState(), 0);
}

TEST_F(SyntaxHighlighterTest, BlockCommentOpenedAndClosedOnNextLine) {
  highlight("code /*\n*/\nmore code");
  EXPECT_EQ(lineAt(0).userState(), 1);
  EXPECT_EQ(lineAt(1).userState(), 0);
  EXPECT_EQ(lineAt(2).userState(), 0);
}

}  // namespace qde::gui
