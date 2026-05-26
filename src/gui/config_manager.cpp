#include "qde/gui/config_manager.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QString>
#include <map>
#include <vector>

#include "qde/gui/config_key.hpp"

namespace qde::gui::config {

ConfigManager& ConfigManager::Instance() {
  static ConfigManager instance;
  return instance;
}

QString ConfigManager::DefaultConfigPath() {
  // e.g. ~/.config/<AppName>  on Linux
  //       ~/Library/Preferences/<AppName>  on macOS
  //       %APPDATA%\<AppName>  on Windows
  return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

bool ConfigManager::Save(const QString& dir_path) {
  // Partition the flat registry into per-category buckets.
  std::map<QString, std::vector<const ConfigKeyBase*>> by_category;
  for (const ConfigKeyBase* key = ConfigKeyBase::First(); key != nullptr;
       key = key->Next()) {
    by_category[QLatin1StringView{key->Category()}].push_back(key);
  }

  bool all_ok = true;
  for (const auto& [category, keys] : by_category) {
    all_ok = all_ok && SaveCategory(category, keys, dir_path);
  }
  return all_ok;
}

bool ConfigManager::SaveCategory(const QString& category,
                                 const std::vector<const ConfigKeyBase*>& keys,
                                 const QString& dir_path) {
  const QDir dir(dir_path);
  if (!dir.exists() && !QDir().mkpath(dir_path)) {
    qWarning("ConfigManager::Save - cannot create directory: %s",
             qPrintable(dir_path));
    return false;
  }

  QJsonObject obj;
  for (const ConfigKeyBase* key : keys) {
    obj.insert(QLatin1StringView{key->FieldName()}, key->ToJsonValue());
  }

  const QString file_path = dir.filePath(category + ".json");
  QFile file(file_path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate |
                 QIODevice::Text)) {
    qWarning("ConfigManager::Save - cannot open for writing: %s - %s",
             qPrintable(file_path), qPrintable(file.errorString()));
    return false;
  }

  const qint64 written =
      file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
  if (written < 0) {
    qWarning("ConfigManager::Save - write error on: %s - %s",
             qPrintable(file_path), qPrintable(file.errorString()));
    return false;
  }

  return true;
}

bool ConfigManager::Load(const QString& dir_path) {
  const QDir dir(dir_path);
  if (!dir.exists()) {
    // No config directory yet; silently keep all defaults.
    return true;
  }

  const QStringList json_files = dir.entryList(
      QStringList{QStringLiteral("*.json")}, QDir::Files | QDir::Readable);

  bool all_ok = true;
  for (const QString& filename : json_files) {
    all_ok &= LoadFile(dir.filePath(filename));
  }
  return all_ok;
}

bool ConfigManager::LoadFile(const QString& file_path) {
  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning("ConfigManager::Load - cannot open: %s - %s",
             qPrintable(file_path), qPrintable(file.errorString()));
    return false;
  }

  QJsonParseError parse_error;
  const QJsonDocument doc =
      QJsonDocument::fromJson(file.readAll(), &parse_error);

  if (parse_error.error != QJsonParseError::NoError) {
    qWarning("ConfigManager::Load - JSON parse error in %s at offset %d: %s",
             qPrintable(file_path), parse_error.offset,
             qPrintable(parse_error.errorString()));
    return false;
  }

  if (!doc.isObject()) {
    qWarning("ConfigManager::Load - root is not a JSON object in: %s",
             qPrintable(file_path));
    return false;
  }

  const QString category = QFileInfo(file_path).completeBaseName();

  bool all_ok = true;
  const QJsonObject obj = doc.object();

  for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
    const QString key_id = category + "." + it.key();

    ConfigKeyBase* found = ConfigKeyBase::Find(key_id);
    if (found == nullptr) {
      qWarning("ConfigManager::Load - unknown key '%s' in %s - skipped",
               qPrintable(key_id), qPrintable(file_path));
      continue;
    }

    if (!found->FromJsonValue(it.value())) {
      qWarning(
          "ConfigManager::Load - value for '%s' in %s failed validation - "
          "keeping default",
          qPrintable(key_id), qPrintable(file_path));
      all_ok = false;
    }
  }

  return all_ok;
}

}  // namespace qde::gui::config