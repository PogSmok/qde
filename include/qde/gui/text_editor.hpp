#ifndef GUI_TEXT_EDITOR_HPP_
#define GUI_TEXT_EDITOR_HPP_

#include <QPointer>
#include <QString>
#include <QUndoStack>
#include <QWidget>

#include "qde/gui/code_editor.hpp"
#include "qde/gui/text_document.hpp"

namespace qde::gui {

class TextEditor : public QWidget {
  Q_OBJECT
 public:
  explicit TextEditor(QWidget* parent = nullptr);

  void newFile();
  void openFile(const QString& path);
  bool saveFile();
  [[nodiscard]] bool saveFileAs(const QString& path);
  void syncEditorToDoc();

  [[nodiscard]] QString plainText() const;
  [[nodiscard]] QString filePath() const;
  [[nodiscard]] bool isModified() const;

  [[nodiscard]] const TextDocument* document() const { return document_; }
  void setContent(const QString& content);

  void setErrors(const std::vector<qde::SyntaxError>& errors);
  void clearErrors();

 signals:
  void textChanged();
  void fileChanged(const QString& path);
  void modifiedChanged(bool modified);

 public slots:
  void onEditorTextChanged();
  void toggleComment();
  void indentBlock();
  void outdentBlock();
  void moveBlockUp();
  void moveBlockDown();

 private:
  QPointer<CodeEditor> editor_;
  QPointer<TextDocument> document_;
  QPointer<QUndoStack> undoStack_;
};

}  // namespace qde::gui

#endif  // GUI_TEXT_EDITOR_HPP_