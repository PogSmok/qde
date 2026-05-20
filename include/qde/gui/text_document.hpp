#ifndef GUI_TEXT_DOCUMENT_HPP_
#define GUI_TEXT_DOCUMENT_HPP_

#include <QObject>
#include <QString>
#include <filesystem>

namespace qde::gui {

class TextDocument : public QObject {
  Q_OBJECT
 public:
  explicit TextDocument(QObject* parent = nullptr);

  [[nodiscard]] QString FilePath() const;
  [[nodiscard]] const QString& Content() const noexcept { return content_; }
  [[nodiscard]] bool Modified() const noexcept { return modified_; }

  bool Load(const std::filesystem::path& path);
  bool Save();
  bool SaveAs(const std::filesystem::path& path);
  void SetModified(bool v);
  void SetContent(const QString& text);

 signals:
  void ModifiedChanged(bool modified);
  void ContentChanged();
  void FilePathChanged(const QString& path);

 private:
  QString content_;
  bool modified_ = false;
  std::filesystem::path path_;
};

}  // namespace qde::gui

#endif  // GUI_TEXT_DOCUMENT_HPP_