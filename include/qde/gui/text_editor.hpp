#ifndef GUI_TEXT_EDITOR_HPP_
#define GUI_TEXT_EDITOR_HPP_

#include <QString>
#include <QPointer>
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
  bool saveFileAs(const QString& path) const;
  void syncEditorToDoc();

  [[nodiscard]] QString plainText() const;
  [[nodiscard]] QString filePath() const;
  [[nodiscard]] bool isModified() const;

  [[nodiscard]] QPointer<TextDocument> document() const { return document_; }

  void setErrors(const std::vector<qde::SyntaxError>& errors) const;
  void clearErrors() const;

 signals:
  void textChanged();
  void fileChanged(const QString& path);
  void modifiedChanged(bool modified);

 public slots:
  void onEditorTextChanged();

 private:
  QPointer<CodeEditor> editor_;
  QPointer<TextDocument> document_;
  QPointer<QUndoStack> undoStack_;

  bool syncing_ = false;
};

}  // namespace qde::gui

#endif  // GUI_TEXT_EDITOR_HPP_