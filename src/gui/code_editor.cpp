#include <QPainter>
#include <QTextBlock>

#include "qde/gui/code_editor.hpp"

#include <iostream>
#include <ostream>

namespace qde::gui {

CodeEditor::CodeEditor(QWidget* parent) : QPlainTextEdit(parent) {
  lineNumberArea_ = new LineNumberArea(this);

  // Dark background
  QPalette p = palette();
  p.setColor(QPalette::Base, QColor(30, 30, 30));
  p.setColor(QPalette::Text, QColor(212, 212, 212));
  setPalette(p);

  QFont font("Cascadia Code", 11);
  font.setFixedPitch(true);
  setFont(font);
  setTabStopDistance(QFontMetrics(font).horizontalAdvance(' ') * 4);

  connect(this, &QPlainTextEdit::blockCountChanged, this,
          &CodeEditor::updateLineNumberAreaWidth);
  connect(this, &QPlainTextEdit::updateRequest, this,
          &CodeEditor::updateLineNumberArea);

  updateLineNumberAreaWidth(0);
}

int CodeEditor::lineNumberAreaWidth() const {
  int digits = 1;
  int max = qMax(1, blockCount());
  while (max >= 10) {
    max /= 10;
    ++digits;
  }
  return 8 + fontMetrics().horizontalAdvance('9') * digits;
}

void CodeEditor::updateLineNumberAreaWidth(int) {
  setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect& rect, int dy) {
  if (dy) {
    lineNumberArea_->scroll(0, dy);
  } else {
    lineNumberArea_->update(0, rect.y(), lineNumberArea_->width(),
                            rect.height());
  }

  if (rect.contains(viewport()->rect())) updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent* event) {
  QPlainTextEdit::resizeEvent(event);
  QRect cr = contentsRect();
  lineNumberArea_->setGeometry(
      {cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()});
  std::cout << lineNumberAreaWidth() << std::endl;
}

void CodeEditor::lineNumberAreaPaintEvent(const QPaintEvent* event) const {
  QPainter painter(lineNumberArea_);
  painter.fillRect(event->rect(), QColor(40, 40, 40));

  QTextBlock block = firstVisibleBlock();
  int blockNum = block.blockNumber();
  int top =
      qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
  int bottom = top + qRound(blockBoundingRect(block).height());

  while (block.isValid() && top <= event->rect().bottom()) {
    if (block.isVisible() && bottom >= event->rect().top()) {
      painter.setPen(QColor(133, 133, 133));
      painter.drawText(0, top, lineNumberArea_->width() - 4,
                       fontMetrics().height(), Qt::AlignRight,
                       QString::number(blockNum + 1));
    }
    block = block.next();
    top = bottom;
    bottom = top + qRound(blockBoundingRect(block).height());
    ++blockNum;
  }
}

void CodeEditor::setErrors(const std::vector<qde::SyntaxError>& errors) {
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
                        line - 1);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor,
                        column - 1);

    // Select the word or character at the error position
    cursor.select(QTextCursor::WordUnderCursor);

    selection.format = format;
    selection.cursor = cursor;
    selections.append(selection);
  }

  setExtraSelections(selections);
}

void CodeEditor::clearErrors() {
  setExtraSelections(QList<QTextEdit::ExtraSelection>());
}

}  // namespace qde::gui