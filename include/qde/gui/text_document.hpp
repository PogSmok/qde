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

  [[nodiscard]] QString filePath() const;
  [[nodiscard]] const QString& content() const noexcept { return content_; }
  [[nodiscard]] bool modified() const noexcept { return modified_; }

  bool load(const std::filesystem::path& path);
  bool save();
  bool saveAs(const std::filesystem::path& path);
  void setModified(bool v);
  void setContent(const QString& text);

 signals:
  void modifiedChanged(bool modified);
  void contentChanged();
  void filePathChanged(const QString& path);

 private:
  QString content_;
  bool modified_ = false;
  std::filesystem::path path_;
};

}  // namespace qde::gui

#endif  // GUI_TEXT_DOCUMENT_HPP_