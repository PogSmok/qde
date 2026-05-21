#include <QFileDialog>
#include <QMessageBox>
#include <QTextBlock>
#include <QVBoxLayout>

#include "qde/gui/code_editor.hpp"
#include "qde/gui/config.hpp"
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

void TextEditor::ToggleComment() {
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

void TextEditor::IndentBlock() {
  QTextCursor cursor = editor_->textCursor();
  cursor.beginEditBlock();

  int start = cursor.selectionStart();
  int end = cursor.selectionEnd();

  QTextBlock startBlock = editor_->document()->findBlock(start);
  QTextBlock endBlock = editor_->document()->findBlock(end);

  if (end > start && endBlock.position() == end) {
    endBlock = endBlock.previous();
  }

  const bool useSpaces = config::editor::useSpaces.Value();
  const int tabWidth = config::editor::tabWidth.Value();
  const QString indent = useSpaces ? QString(tabWidth, ' ') : QString('\t');

  for (QTextBlock block = startBlock;
       block.isValid() && block.blockNumber() <= endBlock.blockNumber();
       block = block.next()) {
    cursor.setPosition(block.position());
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.insertText(indent);
  }

  cursor.endEditBlock();
}

void TextEditor::OutdentBlock() {
  QTextCursor cursor = editor_->textCursor();
  cursor.beginEditBlock();

  int start = cursor.selectionStart();
  int end = cursor.selectionEnd();

  QTextBlock startBlock = editor_->document()->findBlock(start);
  QTextBlock endBlock = editor_->document()->findBlock(end);

  if (end > start && endBlock.position() == end) {
    endBlock = endBlock.previous();
  }

  const bool useSpaces = config::editor::useSpaces.Value();
  const int tabWidth = config::editor::tabWidth.Value();

  for (QTextBlock block = startBlock;
       block.isValid() && block.blockNumber() <= endBlock.blockNumber();
       block = block.next()) {
    const QString text = block.text();
    if (text.isEmpty()) {
      continue;
    }

    cursor.setPosition(block.position());
    cursor.movePosition(QTextCursor::StartOfBlock);

    if (!useSpaces) {
      if (text.at(0) == '\t') {
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
        cursor.removeSelectedText();
      }
    } else {
      int spacesToRemove = 0;
      while (spacesToRemove < tabWidth && spacesToRemove < text.size() &&
             text.at(spacesToRemove) == ' ') {
        ++spacesToRemove;
      }
      if (spacesToRemove > 0) {
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor,
                            spacesToRemove);
        cursor.removeSelectedText();
      }
    }
  }

  cursor.endEditBlock();
}

void TextEditor::MoveBlockUp() {
  QTextCursor cursor = editor_->textCursor();
  int start = cursor.selectionStart();
  int end = cursor.selectionEnd();

  QTextBlock startBlock = editor_->document()->findBlock(start);
  QTextBlock endBlock = editor_->document()->findBlock(end);

  QTextBlock prevBlock = startBlock.previous();
  if (!prevBlock.isValid()) {
    return;
  }

  const int startPosition = prevBlock.position();
  const int endPosition = endBlock.position() + endBlock.length() - 1;

  const int newStartPosition = start - prevBlock.length();
  const int newEndPosition = end - prevBlock.length();

  cursor.beginEditBlock();

  // Collect selected block texts
  QStringList lines;
  for (QTextBlock block = startBlock;
       block.isValid() && block.blockNumber() <= endBlock.blockNumber();
       block = block.next()) {
    lines.append(block.text());
  }
  const QString prev_text = prevBlock.text();

  // Replace whole block in single action
  QTextCursor replace_cursor(editor_->document());
  replace_cursor.setPosition(startPosition);
  replace_cursor.setPosition(endPosition, QTextCursor::KeepAnchor);
  replace_cursor.insertText(lines.join('\n') + '\n' + prev_text);

  cursor.endEditBlock();

  // Restore cursor selection
  cursor.setPosition(newStartPosition);
  cursor.setPosition(newEndPosition, QTextCursor::KeepAnchor);
  editor_->setTextCursor(cursor);
}

void TextEditor::MoveBlockDown() {
  QTextCursor cursor = editor_->textCursor();
  int start = cursor.selectionStart();
  int end = cursor.selectionEnd();

  QTextBlock startBlock = editor_->document()->findBlock(start);
  QTextBlock endBlock = editor_->document()->findBlock(end);
  QTextBlock nextBlock = endBlock.next();

  if (!nextBlock.isValid()) return;

  const int startPosition = startBlock.position();
  const int endPosition = nextBlock.position() + nextBlock.length() - 1;
  const int newStartPosition = startBlock.position() + nextBlock.length();
  const int newEndPosition = endPosition;

  cursor.beginEditBlock();

  // Collect selected block texts
  QStringList lines;
  for (QTextBlock block = startBlock;
       block.isValid() && block.blockNumber() <= endBlock.blockNumber();
       block = block.next()) {
    lines.append(block.text());
  }

  const QString nextText = nextBlock.text();

  // Replace whole block in single action
  QTextCursor replaceCursor(editor_->document());
  replaceCursor.setPosition(startPosition);
  replaceCursor.setPosition(endPosition, QTextCursor::KeepAnchor);
  replaceCursor.insertText(nextText + '\n' + lines.join('\n'));

  cursor.endEditBlock();

  // Restore cursor selection
  cursor.setPosition(newStartPosition);
  cursor.setPosition(newEndPosition, QTextCursor::KeepAnchor);
  editor_->setTextCursor(cursor);
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