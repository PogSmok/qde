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

  void NewFile();
  void OpenFile(const QString& path);
  bool SaveFile();
  [[nodiscard]] bool SaveFileAs(const QString& path);
  void SyncEditorToDoc();

  [[nodiscard]] QString PlainText() const;
  [[nodiscard]] QString FilePath() const;
  [[nodiscard]] bool IsModified() const;

  [[nodiscard]] QTextDocument* Document() const { return editor_->document(); }
  [[nodiscard]] QPlainTextEdit* Editor() const { return editor_; }
  void SetContent(const QString& content);

  void SetErrors(const std::vector<qde::SyntaxError>& errors);
  void ClearErrors();

 signals:
  void TextChanged();
  void FileChanged(const QString& path);
  void ModifiedChanged(bool modified);

 public slots:
  void OnEditorTextChanged();
  void ToggleComment();
  void IndentBlock();
  void OutdentBlock();
  void MoveBlockUp();
  void MoveBlockDown();

 private:
  QPointer<CodeEditor> editor_;
  QPointer<TextDocument> document_;
  QPointer<QUndoStack> undoStack_;
};

}  // namespace qde::gui

#endif  // GUI_TEXT_EDITOR_HPP_