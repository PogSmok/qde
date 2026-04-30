#ifndef GUI_SYNTAX_HIGHLIGHTER_HPP_
#define GUI_SYNTAX_HIGHLIGHTER_HPP_

#include <QRegularExpression>
#include <QString>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QVector>

namespace qde::gui {

class SyntaxHighlighter : public QSyntaxHighlighter {
  Q_OBJECT
 public:
  explicit SyntaxHighlighter(QTextDocument* parent);

 protected:
  void highlightBlock(const QString& text) override;

 private:
  struct Rule {
    QRegularExpression pattern;
    QTextCharFormat format;
  };
  QVector<Rule> rules_;

  // comments are handled separately
  QString block_comment_start_;
  QString block_comment_end_;
  QTextCharFormat comment_format_;
};

}  // namespace qde::gui

#endif  // GUI_SYNTAX_HIGHLIGHTER_HPP_
