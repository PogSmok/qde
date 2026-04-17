#ifndef TEXT_EDITOR_HPP_
#define TEXT_EDITOR_HPP_

#include <QWidget>
#include <QUndoStack>
#include <QString>

#include "qde/qt/text_document.hpp"
#include "qde/qt/code_editor.hpp"

class TextEditor : public QWidget {
  Q_OBJECT
 public:
  explicit TextEditor(QWidget* parent = nullptr);
  virtual ~TextEditor();

  void newFile();
  void openFile(const QString& path);
  bool saveFile();
  bool saveFileAs(const QString& path);
  void syncEditorToDoc();

  [[nodiscard]] QString plainText()  const;
  [[nodiscard]] QString filePath()   const;
  [[nodiscard]] bool    isModified() const;

  [[nodiscard]] TextDocument* document() const { return document_; }

  void setErrors(const std::vector<qde::SyntaxError>& errors);
  void clearErrors();

 signals:
  void textChanged();
  void fileChanged(const QString& path);
  void modifiedChanged(bool modified);

 public slots:
  void onEditorTextChanged();

 private:
  CodeEditor*           editor_;
  TextDocument*         document_;
  // QASMSyntaxHighlighter* highlighter_;
  QUndoStack*           undoStack_;

  bool syncing_ = false;
  
};

#endif // TEXT_EDITOR_HPP_