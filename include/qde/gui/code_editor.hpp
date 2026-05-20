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

  void LineNumberAreaPaintEvent(const QPaintEvent* event) const;
  [[nodiscard]] int LineNumberAreaWidth() const;

  void SetErrors(const std::vector<qde::SyntaxError>& errors);
  void ClearErrors();

 protected:
  void resizeEvent(QResizeEvent* event) override;

 public slots:
  void UpdateLineNumberAreaWidth(int new_block_count);
  void UpdateLineNumberArea(const QRect& rect, int dy);

 private:
  QPointer<QWidget> lineNumberArea_;
};

}  // namespace qde::gui

#endif  // GUI_CODE_EDITOR_HPP_