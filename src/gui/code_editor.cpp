#include <QPainter>
#include <QTextBlock>

#include "qde/gui/code_editor.hpp"
#include "qde/gui/config.hpp"
#include "qde/gui/syntax_highlighter.hpp"
#include "qde/gui/theme.hpp"

namespace qde::gui {

// ---- LineNumberArea ----
class LineNumberArea : public QWidget {
 public:
  explicit LineNumberArea(CodeEditor* editor)
      : QWidget(editor), editor_(editor) {}
  [[nodiscard]] QSize sizeHint() const override {
    return {editor_->LineNumberAreaWidth(), 0};
  }

 protected:
  void paintEvent(QPaintEvent* ev) override {
    editor_->LineNumberAreaPaintEvent(ev);
  }

 private:
  QPointer<CodeEditor> editor_;
};

CodeEditor::CodeEditor(QWidget* parent)
    : QPlainTextEdit(parent), lineNumberArea_(new LineNumberArea(this)) {
  QPalette p = palette();
  p.setColor(QPalette::Base, theme::kEditorBackground);
  p.setColor(QPalette::Text, theme::kEditorText);
  setPalette(p);

  QFont font(qde::gui::config::editor::fontFamily.Value(),
             qde::gui::config::editor::fontSize.Value());
  font.setFixedPitch(true);
  setFont(font);
  setTabStopDistance(QFontMetrics(font).horizontalAdvance(' ') *
                     qde::gui::config::editor::tabWidth.Value());

  connect(this, &QPlainTextEdit::blockCountChanged, this,
          &CodeEditor::UpdateLineNumberAreaWidth);
  connect(this, &QPlainTextEdit::updateRequest, this,
          &CodeEditor::UpdateLineNumberArea);

  UpdateLineNumberAreaWidth(0);

  new SyntaxHighlighter(document());
}

int CodeEditor::LineNumberAreaWidth() const {
  int digits = 1;
  int max = qMax(1, blockCount());
  while (max >= 10) {
    max /= 10;
    ++digits;
  }
  return 8 + (fontMetrics().horizontalAdvance('9') * digits);
}

void CodeEditor::UpdateLineNumberAreaWidth(int /*unused*/) {
  setViewportMargins(LineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::UpdateLineNumberArea(const QRect& rect, int dy) {
  if (dy != 0) {
    lineNumberArea_->scroll(0, dy);
  } else {
    lineNumberArea_->update(0, rect.y(), lineNumberArea_->width(),
                            rect.height());
  }

  if (rect.contains(viewport()->rect())) {
    UpdateLineNumberAreaWidth(0);
  }
}

void CodeEditor::resizeEvent(QResizeEvent* event) {
  QPlainTextEdit::resizeEvent(event);
  QRect cr = contentsRect();
  lineNumberArea_->setGeometry(
      {cr.left(), cr.top(), LineNumberAreaWidth(), cr.height()});
}

void CodeEditor::LineNumberAreaPaintEvent(const QPaintEvent* event) const {
  QPainter painter(lineNumberArea_);
  painter.fillRect(event->rect(), theme::kLineNumberBackground);

  QTextBlock block = firstVisibleBlock();
  int block_num = block.blockNumber();
  int top =
      qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
  int bottom = top + qRound(blockBoundingRect(block).height());

  while (block.isValid() && top <= event->rect().bottom()) {
    if (block.isVisible() && bottom >= event->rect().top()) {
      painter.setPen(theme::kLineNumberText);
      painter.drawText(0, top,
                       lineNumberArea_->width() - theme::kTextBlockLeftPadding,
                       fontMetrics().height(), Qt::AlignRight,
                       QString::number(block_num + 1));
    }
    block = block.next();
    top = bottom;
    bottom = top + qRound(blockBoundingRect(block).height());
    ++block_num;
  }
}

void CodeEditor::SetErrors(const std::vector<qde::SyntaxError>& errors) {
  QList<QTextEdit::ExtraSelection> selections;

  for (const auto& [line, column, message] : errors) {
    QTextEdit::ExtraSelection selection;

    QTextCharFormat format;
    format.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    format.setUnderlineColor(Qt::red);
    format.setToolTip(QString::fromStdString(message));

    QTextCursor cursor = textCursor();
    cursor.setPosition(0);
    // QTextBlock is 0-indexed, SyntaxError line is 1-indexed.
    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor,
                        static_cast<int>(line) - 1);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor,
                        static_cast<int>(column) - 1);

    // Select the word or character at the error position
    cursor.select(QTextCursor::WordUnderCursor);

    selection.format = format;
    selection.cursor = cursor;
    selections.append(selection);
  }

  setExtraSelections(selections);
}

void CodeEditor::ClearErrors() {
  setExtraSelections(QList<QTextEdit::ExtraSelection>());
}

}  // namespace qde::gui