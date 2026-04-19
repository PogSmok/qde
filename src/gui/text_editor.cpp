#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>

#include "qde/gui/code_editor.hpp"
#include "qde/gui/text_editor.hpp"

namespace qde::gui {

TextEditor::TextEditor(QWidget* parent)
    : QWidget(parent),
      editor_(new CodeEditor(this)),
      document_(new TextDocument(this)),
      undoStack_(new QUndoStack(this)) {
  QPointer<QVBoxLayout> const layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(editor_);

  connect(editor_, &QPlainTextEdit::textChanged, this,
          &TextEditor::onEditorTextChanged);

  connect(document_, &TextDocument::modifiedChanged, this,
          &TextEditor::modifiedChanged);

  connect(document_, &TextDocument::filePathChanged, this,
          &TextEditor::fileChanged);

  // Undo/redo plumbing
  editor_->document()->setUndoRedoEnabled(true);
}

void TextEditor::newFile() {
  document_->setContent({});
  document_->setModified(false);
  syncEditorToDoc();
}

void TextEditor::openFile(const QString& path) {
  if (document_->load(std::filesystem::path(path.toStdString()))) {
    syncEditorToDoc();
  }
}

bool TextEditor::saveFile() {
  if (filePath().isEmpty()) {
    return saveFileAs(QFileDialog::getSaveFileName(
        this, "Save File", {}, "QASM Files (*.qasm);;All Files (*)"));
  }
  return document_->save();
}

bool TextEditor::saveFileAs(const QString& path) {
  if (path.isEmpty()) {
    return false;
  }
  document_->setContent(editor_->toPlainText());
  return document_->saveAs(std::filesystem::path(path.toStdString()));
}

QString TextEditor::plainText() const { return editor_->toPlainText(); }
QString TextEditor::filePath() const { return document_->filePath(); }
bool TextEditor::isModified() const { return document_->modified(); }

void TextEditor::setContent(const QString& content) {
  document_->setContent(content);
  syncEditorToDoc();
}

void TextEditor::onEditorTextChanged() {
  document_->setContent(editor_->toPlainText());
  emit textChanged();
}

void TextEditor::syncEditorToDoc() {
  QSignalBlocker block(editor_);
  editor_->setPlainText(document_->content());
  block.unblock();
}

void TextEditor::setErrors(const std::vector<qde::SyntaxError>& errors) {
  editor_->setErrors(errors);
}

void TextEditor::clearErrors() { editor_->clearErrors(); }

}  // namespace qde::gui