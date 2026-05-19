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

ConfigManager& ConfigManager::instance() {
  static ConfigManager instance;
  return instance;
}

QString ConfigManager::defaultConfigPath() {
  // e.g. ~/.config/<AppName>  on Linux
  //       ~/Library/Preferences/<AppName>  on macOS
  //       %APPDATA%\<AppName>  on Windows
  return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

bool ConfigManager::save(const QString& dir_path) {
  // Partition the flat registry into per-category buckets.
  std::map<QString, std::vector<ConfigKeyBase*>> by_category;
  for (auto& [id, key] : ConfigKeyBase::config_registry) {
    by_category[key->category()].push_back(key);
  }

  bool all_ok = true;
  for (const auto& [category, keys] : by_category) {
    all_ok &= saveCategory(category, keys, dir_path);
  }
  return all_ok;
}

bool ConfigManager::saveCategory(const QString& category,
                                 const std::vector<ConfigKeyBase*>& keys,
                                 const QString& dir_path) const {
  // Ensure destination directory exists.
  const QDir dir(dir_path);
  if (!dir.exists() && !QDir().mkpath(dir_path)) {
    qWarning("ConfigManager::save - cannot create directory: %s",
             qPrintable(dir_path));
    return false;
  }

  // Build JSON object: { fieldName: value, … }
  QJsonObject obj;
  for (const ConfigKeyBase* key : keys) {
    obj.insert(key->fieldName(), key->toJsonValue());
  }

  const QString file_path = dir.filePath(category + ".json");
  QFile file(file_path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate |
                 QIODevice::Text)) {
    qWarning("ConfigManager::save - cannot open for writing: %s - %s",
             qPrintable(file_path), qPrintable(file.errorString()));
    return false;
  }

  const qint64 written =
      file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
  if (written < 0) {
    qWarning("ConfigManager::save - write error on: %s - %s",
             qPrintable(file_path), qPrintable(file.errorString()));
    return false;
  }

  return true;
}

bool ConfigManager::load(const QString& dir_path) {
  const QDir dir(dir_path);
  if (!dir.exists()) {
    // No config directory yet; silently keep all defaults.
    return true;
  }

  const QStringList json_files = dir.entryList(
      QStringList{QStringLiteral("*.json")}, QDir::Files | QDir::Readable);

  bool all_ok = true;
  for (const QString& filename : json_files) {
    all_ok &= loadFile(dir.filePath(filename));
  }
  return all_ok;
}

bool ConfigManager::loadFile(const QString& file_path) const {
  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning("ConfigManager::load - cannot open: %s - %s",
             qPrintable(file_path), qPrintable(file.errorString()));
    return false;
  }

  QJsonParseError parse_error;
  const QJsonDocument doc =
      QJsonDocument::fromJson(file.readAll(), &parse_error);

  if (parse_error.error != QJsonParseError::NoError) {
    qWarning("ConfigManager::load - JSON parse error in %s at offset %d: %s",
             qPrintable(file_path), parse_error.offset,
             qPrintable(parse_error.errorString()));
    return false;
  }

  if (!doc.isObject()) {
    qWarning("ConfigManager::load - root is not a JSON object in: %s",
             qPrintable(file_path));
    return false;
  }

  // The category is the file stem (e.g. "editor" from "editor.json").
  const QString category = QFileInfo(file_path).completeBaseName();

  bool all_ok = true;
  const QJsonObject obj = doc.object();

  for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
    const QString key_id = category + "." + it.key();

    const auto registry_it = ConfigKeyBase::config_registry.find(key_id);
    if (registry_it == ConfigKeyBase::config_registry.cend()) {
      qWarning("ConfigManager::load - unknown key '%s' in %s - skipped",
               qPrintable(key_id), qPrintable(file_path));
      continue;
    }

    if (!registry_it->second->fromJsonValue(it.value())) {
      qWarning(
          "ConfigManager::load - value for '%s' in %s failed validation - "
          "keeping default",
          qPrintable(key_id), qPrintable(file_path));
      all_ok = false;
    }
  }

  return all_ok;
}

}  // namespace qde::gui::config