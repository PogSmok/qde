#ifndef GUI_CODE_EDITOR_HPP_
#define GUI_CODE_EDITOR_HPP_

#include <QPlainTextEdit>
#include <QPointer>
#include <QWidget>
#include <vector>

#include "qde/syntax_error.hpp"

namespace qde::gui {

class CodeEditor : public QPlainTextEdit {
  Q_OBJECT
 public:
  explicit CodeEditor(QWidget* parent = nullptr);

  void lineNumberAreaPaintEvent(const QPaintEvent* event) const;
  [[nodiscard]] int lineNumberAreaWidth() const;

  void setErrors(const std::vector<qde::SyntaxError>& errors);
  void clearErrors();

 protected:
  void resizeEvent(QResizeEvent* event) override;

 public slots:
  void updateLineNumberAreaWidth(int newBlockCount);
  void updateLineNumberArea(const QRect& rect, int dy);

 private:
  QPointer<QWidget> lineNumberArea_;
};

// ---- LineNumberArea ----
class LineNumberArea : public QWidget {
 public:
  explicit LineNumberArea(CodeEditor* editor)
      : QWidget(editor), editor_(editor) {}
  [[nodiscard]] QSize sizeHint() const override {
    return {editor_->lineNumberAreaWidth(), 0};
  }

 protected:
  void paintEvent(QPaintEvent* ev) override {
    editor_->lineNumberAreaPaintEvent(ev);
  }

 private:
  QPointer<CodeEditor> editor_;
};

}  // namespace qde::gui

#endif  // GUI_CODE_EDITOR_HPP_