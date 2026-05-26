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

  int const start = cursor.selectionStart();
  int const end = cursor.selectionEnd();

  QTextBlock start_block = editor_->document()->findBlock(start);
  QTextBlock end_block = editor_->document()->findBlock(end);

  // Common convention: If the cursor is at the very beginning of a block
  // at the end of a selection, don't include that block.
  if (end > start && end_block.position() == end) {
    end_block = end_block.previous();
  }

  // Logic: If any line in selection is NOT commented, we add comments to all
  bool all_commented = true;
  for (QTextBlock block = start_block;
       block.isValid() && block != end_block.next(); block = block.next()) {
    QString const text = block.text().trimmed();
    if (!text.isEmpty() && !text.startsWith("//")) {
      all_commented = false;
      break;
    }
  }

  for (QTextBlock block = start_block;
       block.isValid() && block != end_block.next(); block = block.next()) {
    cursor.setPosition(block.position());
    cursor.movePosition(QTextCursor::StartOfBlock);

    QString const text = block.text();
    if (all_commented) {
      // Remove "//" (and potentially one trailing space)
      int const comment_pos = text.indexOf("//");
      if (comment_pos != -1) {
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor,
                            comment_pos);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
        // Optional: handle the extra space if you use "// " style
        if (text.size() > comment_pos + 2 && text.at(comment_pos + 2) == ' ') {
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

  int const start = cursor.selectionStart();
  int const end = cursor.selectionEnd();

  QTextBlock start_block = editor_->document()->findBlock(start);
  QTextBlock end_block = editor_->document()->findBlock(end);

  if (end > start && end_block.position() == end) {
    end_block = end_block.previous();
  }

  const bool use_spaces = config::editor::use_spaces.Value();
  const int tab_width = config::editor::tab_width.Value();
  const QString indent = use_spaces ? QString(tab_width, ' ') : QString('\t');

  for (QTextBlock block = start_block;
       block.isValid() && block != end_block.next(); block = block.next()) {
    cursor.setPosition(block.position());
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.insertText(indent);
  }

  cursor.endEditBlock();
}

void TextEditor::OutdentBlock() {
  QTextCursor cursor = editor_->textCursor();
  cursor.beginEditBlock();

  int const start = cursor.selectionStart();
  int const end = cursor.selectionEnd();

  QTextBlock start_block = editor_->document()->findBlock(start);
  QTextBlock end_block = editor_->document()->findBlock(end);

  if (end > start && end_block.position() == end) {
    end_block = end_block.previous();
  }

  const bool use_spaces = config::editor::use_spaces.Value();
  const int tab_width = config::editor::tab_width.Value();

  for (QTextBlock block = start_block;
       block.isValid() && block != end_block.next(); block = block.next()) {
    const QString text = block.text();
    if (text.isEmpty()) {
      continue;
    }

    cursor.setPosition(block.position());
    cursor.movePosition(QTextCursor::StartOfBlock);

    if (!use_spaces) {
      if (text.at(0) == '\t') {
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
        cursor.removeSelectedText();
      }
    } else {
      int spaces_to_remove = 0;
      while (spaces_to_remove < tab_width && spaces_to_remove < text.size() &&
             text.at(spaces_to_remove) == ' ') {
        ++spaces_to_remove;
      }
      if (spaces_to_remove > 0) {
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor,
                            spaces_to_remove);
        cursor.removeSelectedText();
      }
    }
  }

  cursor.endEditBlock();
}

void TextEditor::MoveBlockUp() {
  QTextCursor cursor = editor_->textCursor();
  int const start = cursor.selectionStart();
  int const end = cursor.selectionEnd();

  QTextBlock start_block = editor_->document()->findBlock(start);
  QTextBlock end_block = editor_->document()->findBlock(end);

  QTextBlock prev_block = start_block.previous();
  if (!prev_block.isValid()) {
    return;
  }

  const int start_position = prev_block.position();
  const int end_position = end_block.position() + end_block.length() - 1;

  const int new_start_position = start - prev_block.length();
  const int new_end_position = end - prev_block.length();

  cursor.beginEditBlock();

  // Collect selected block texts
  QStringList lines;
  for (QTextBlock block = start_block;
       block.isValid() && block != end_block.next(); block = block.next()) {
    lines.append(block.text());
  }
  const QString prev_text = prev_block.text();

  // Replace whole block in single action
  QTextCursor replace_cursor(editor_->document());
  replace_cursor.setPosition(start_position);
  replace_cursor.setPosition(end_position, QTextCursor::KeepAnchor);
  replace_cursor.insertText(lines.join('\n') + '\n' + prev_text);

  cursor.endEditBlock();

  // Restore cursor selection
  cursor.setPosition(new_start_position);
  cursor.setPosition(new_end_position, QTextCursor::KeepAnchor);
  editor_->setTextCursor(cursor);
}

void TextEditor::MoveBlockDown() {
  QTextCursor cursor = editor_->textCursor();
  int const start = cursor.selectionStart();
  int const end = cursor.selectionEnd();

  QTextBlock start_block = editor_->document()->findBlock(start);
  QTextBlock end_block = editor_->document()->findBlock(end);
  QTextBlock next_block = end_block.next();

  if (!next_block.isValid()) {
    return;
  }

  const int start_position = start_block.position();
  const int end_position = next_block.position() + next_block.length() - 1;
  const int new_start_position = start_block.position() + next_block.length();
  const int new_end_position = end_position;

  cursor.beginEditBlock();

  // Collect selected block texts
  QStringList lines;
  for (QTextBlock block = start_block;
       block.isValid() && block != end_block.next(); block = block.next()) {
    lines.append(block.text());
  }

  const QString next_text = next_block.text();

  // Replace whole block in single action
  QTextCursor replace_cursor(editor_->document());
  replace_cursor.setPosition(start_position);
  replace_cursor.setPosition(end_position, QTextCursor::KeepAnchor);
  replace_cursor.insertText(next_text + '\n' + lines.join('\n'));

  cursor.endEditBlock();

  // Restore cursor selection
  cursor.setPosition(new_start_position);
  cursor.setPosition(new_end_position, QTextCursor::KeepAnchor);
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