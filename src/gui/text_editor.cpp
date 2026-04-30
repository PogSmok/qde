#include <QFileDialog>
#include <QMessageBox>
#include <QTextBlock>
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

void TextEditor::toggleComment() {
  QTextCursor cursor = editor_->textCursor();

  cursor.beginEditBlock();

  int start = cursor.selectionStart();
  int end = cursor.selectionEnd();

  QTextBlock startBlock = editor_->document()->findBlock(start);
  QTextBlock endBlock = editor_->document()->findBlock(end);

  // Common convention: If the cursor is at the very beginning of a block
  // at the end of a selection, don't include that block.
  if (end > start && endBlock.position() == end) {
    endBlock = endBlock.previous();
  }

  // Logic: If any line in selection is NOT commented, we add comments to all
  bool allCommented = true;
  for (QTextBlock block = startBlock;
       block.isValid() && block.blockNumber() <= endBlock.blockNumber();
       block = block.next()) {
    QString text = block.text().trimmed();
    if (!text.isEmpty() && !text.startsWith("//")) {
      allCommented = false;
      break;
    }
  }

  for (QTextBlock block = startBlock;
       block.isValid() && block.blockNumber() <= endBlock.blockNumber();
       block = block.next()) {
    cursor.setPosition(block.position());
    cursor.movePosition(QTextCursor::StartOfBlock);

    QString text = block.text();
    if (allCommented) {
      // Remove "//" (and potentially one trailing space)
      int commentPos = text.indexOf("//");
      if (commentPos != -1) {
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor,
                            commentPos);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
        // Optional: handle the extra space if you use "// " style
        if (text.size() > commentPos + 2 && text.at(commentPos + 2) == ' ') {
          cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
        }
        cursor.removeSelectedText();
      }
    } else {
      // Add "// " at the very beginning of the block
      cursor.insertText("// ");
    }
  }

  cursor.endEditBlock();
}

void TextEditor::indentBlock() {}
void TextEditor::outdentBlock() {}
void TextEditor::moveBlockUp() {}
void TextEditor::moveBlockDown() {}

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
