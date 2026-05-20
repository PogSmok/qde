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
          &TextEditor::OnEditorTextChanged);

  connect(document_, &TextDocument::ModifiedChanged, this,
          &TextEditor::ModifiedChanged);

  connect(document_, &TextDocument::FilePathChanged, this,
          &TextEditor::FileChanged);

  // Undo/redo plumbing
  editor_->document()->setUndoRedoEnabled(true);
}

void TextEditor::NewFile() {
  document_->SetContent({});
  document_->SetModified(false);
  SyncEditorToDoc();
}

void TextEditor::OpenFile(const QString& path) {
  if (document_->Load(std::filesystem::path(path.toStdString()))) {
    SyncEditorToDoc();
  }
}

bool TextEditor::SaveFile() {
  if (FilePath().isEmpty()) {
    return SaveFileAs(QFileDialog::getSaveFileName(
        this, "Save File", {}, "QASM Files (*.qasm);;All Files (*)"));
  }
  return document_->Save();
}

bool TextEditor::SaveFileAs(const QString& path) {
  if (path.isEmpty()) {
    return false;
  }
  document_->SetContent(editor_->toPlainText());
  return document_->SaveAs(std::filesystem::path(path.toStdString()));
}

QString TextEditor::PlainText() const { return editor_->toPlainText(); }
QString TextEditor::FilePath() const { return document_->FilePath(); }
bool TextEditor::IsModified() const { return document_->Modified(); }

void TextEditor::SetContent(const QString& content) {
  document_->SetContent(content);
  SyncEditorToDoc();
}

void TextEditor::OnEditorTextChanged() {
  document_->SetContent(editor_->toPlainText());
  emit TextChanged();
}

void TextEditor::SyncEditorToDoc() {
  QSignalBlocker block(editor_);
  editor_->setPlainText(document_->Content());
  block.unblock();
}

void TextEditor::SetErrors(const std::vector<qde::SyntaxError>& errors) {
  editor_->SetErrors(errors);
}

void TextEditor::ClearErrors() { editor_->ClearErrors(); }

}  // namespace qde::gui