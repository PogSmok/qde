#include <QFile>
#include <QTextStream>

#include "qde/gui/text_document.hpp"

namespace qde::gui {

TextDocument::TextDocument(QObject* parent) : QObject(parent) {}

QString TextDocument::filePath() const {
  return QString::fromStdString(path_.string());
}

bool TextDocument::load(const std::filesystem::path& path) {
  QFile file(QString::fromStdString(path.string()));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }
  QTextStream in(&file);
  content_ = in.readAll();
  path_ = path;
  setModified(false);
  emit contentChanged();
  emit filePathChanged(filePath());
  return true;
}

bool TextDocument::save() {
  if (path_.empty()) {
    return false;
  }
  return saveAs(path_);
}

bool TextDocument::saveAs(const std::filesystem::path& path) {
  QFile file(QString::fromStdString(path.string()));
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    return false;
  }
  QTextStream out(&file);
  out << content_;
  path_ = path;
  setModified(false);
  emit filePathChanged(filePath());
  return true;
}

void TextDocument::setModified(bool v) {
  if (modified_ == v) {
    return;
  }
  modified_ = v;
  emit modifiedChanged(v);
}

void TextDocument::setContent(const QString& text) {
  if (content_ == text) {
    return;
  }
  content_ = text;
  setModified(true);
  emit contentChanged();
}

}  // namespace qde::gui