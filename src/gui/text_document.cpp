#include <QFile>
#include <QTextStream>

#include "qde/gui/text_document.hpp"

namespace qde::gui {

TextDocument::TextDocument(QObject* parent) : QObject(parent) {}

QString TextDocument::FilePath() const {
  return QString::fromStdString(path_.string());
}

bool TextDocument::Load(const std::filesystem::path& path) {
  QFile file(QString::fromStdString(path.string()));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }
  QTextStream in(&file);
  content_ = in.readAll();
  path_ = path;
  SetModified(false);
  emit ContentChanged();
  emit FilePathChanged(FilePath());
  return true;
}

bool TextDocument::Save() {
  if (path_.empty()) {
    return false;
  }
  return SaveAs(path_);
}

bool TextDocument::SaveAs(const std::filesystem::path& path) {
  QFile file(QString::fromStdString(path.string()));
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    return false;
  }
  QTextStream out(&file);
  out << content_;
  path_ = path;
  SetModified(false);
  emit FilePathChanged(FilePath());
  return true;
}

void TextDocument::SetModified(bool v) {
  if (modified_ == v) {
    return;
  }
  modified_ = v;
  emit ModifiedChanged(v);
}

void TextDocument::SetContent(const QString& text) {
  if (content_ == text) {
    return;
  }
  content_ = text;
  SetModified(true);
  emit ContentChanged();
}

}  // namespace qde::gui