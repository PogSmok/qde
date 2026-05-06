#include <gtest/gtest.h>
#include <QTextDocument>

#include "qde/gui/syntax_highlighter.hpp"

namespace qde::gui {

class SyntaxHighlighterTest : public ::testing::Test {
 protected:
  void SetUp() override { highlighter_ = new SyntaxHighlighter(&doc_); }

  void TearDown() override { delete highlighter_; }

  // Sets text and forces a synchronous rehighlight.
  void Highlight(const QString& text) {
    doc_.setPlainText(text);
    highlighter_->rehighlight();
  }

  [[nodiscard]] QTextBlock LineAt(int n) const {
    return doc_.findBlockByLineNumber(n);
  }

  QTextDocument doc_;
  SyntaxHighlighter* highlighter_ = nullptr;
};

// --- Block comment state -----------------------------------------------------

TEST_F(SyntaxHighlighterTest, NormalCodeHasCodeState) {
  Highlight("gate h q { }");
  EXPECT_EQ(LineAt(0).userState(), 0);
}

TEST_F(SyntaxHighlighterTest, UnclosedBlockCommentSetsCommentedState) {
  Highlight("/* comment");
  EXPECT_EQ(LineAt(0).userState(), 1);
}

TEST_F(SyntaxHighlighterTest, ClosedBlockCommentResetsState) {
  Highlight("/* comment */");
  EXPECT_EQ(LineAt(0).userState(), 0);
}

TEST_F(SyntaxHighlighterTest, BlockCommentSpansMultipleLines) {
  Highlight("/* start\ncontinued\n*/\ncode");
  EXPECT_EQ(LineAt(0).userState(), 1);  // comment opened
  EXPECT_EQ(LineAt(1).userState(), 1);  // still inside
  EXPECT_EQ(LineAt(2).userState(), 0);  // closed by */
  EXPECT_EQ(LineAt(3).userState(), 0);  // normal code after
}

TEST_F(SyntaxHighlighterTest, LineCommentDoesNotAffectBlockState) {
  Highlight("// comment\ncode");
  EXPECT_EQ(LineAt(0).userState(), 0);
  EXPECT_EQ(LineAt(1).userState(), 0);
}

TEST_F(SyntaxHighlighterTest, MultipleBlockCommentsOnOneLine) {
  Highlight("/* a */ code /* b */\nnormal");
  EXPECT_EQ(LineAt(0).userState(), 0);
  EXPECT_EQ(LineAt(1).userState(), 0);
}

TEST_F(SyntaxHighlighterTest, BlockCommentOpenedAndClosedOnNextLine) {
  Highlight("code /*\n*/\nmore code");
  EXPECT_EQ(LineAt(0).userState(), 1);
  EXPECT_EQ(LineAt(1).userState(), 0);
  EXPECT_EQ(LineAt(2).userState(), 0);
}

}  // namespace qde::gui
