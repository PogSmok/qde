#ifndef GUI_CONFIG_MANAGER_HPP_
#define GUI_CONFIG_MANAGER_HPP_

#include <QString>
#include <vector>

// Forward-declare to avoid pulling all of config_key.hpp
// when only need to call save()/load().
namespace qde::gui::config {
class ConfigKeyBase;
}

namespace qde::gui::config {

// -----------------------------------------------------------------------
// ConfigManager
//
// Persistence layer for the config registry defined in config.hpp.
//
// Layout on disk:
//   <dir>/editor.json
//   <dir>/theme.json
//   <dir>/shortcut.json
//   …one file per category, file-stem == category name…
//
// Typical usage:
//   auto& mgr = ConfigManager::instance();
//   mgr.load();          // at startup — populates all ConfigKey values
//   ...user changes settings...
//   mgr.save();          // at shutdown / apply
//
// Both save() and load() are non-throwing; errors are reported via
// qWarning() and reflected in the bool return value.
// -----------------------------------------------------------------------
class ConfigManager {
 public:
  // Meyer's singleton — construction is deferred until first call.
  static ConfigManager& Instance();

  // Saves every category to <dir_path>/<category>.json.
  // Creates dir_path (and parents) if absent.
  // Returns true iff every file was written successfully.
  bool Save(const QString& dir_path = DefaultConfigPath());

  // Loads every *.json file found under dir_path.
  // Missing dir -> no-op (returns true; defaults remain in effect).
  // Unknown keys in a file are skipped with a warning.
  // Invalid values (validator rejection) are skipped; default is kept.
  // Returns true iff every file was parsed and every value applied.
  bool Load(const QString& dir_path = DefaultConfigPath());

  // QStandardPaths::AppConfigLocation for the running application.
  // Requires QCoreApplication::setApplicationName() to be called first
  // for a meaningful path.
  [[nodiscard]] static QString DefaultConfigPath();

  // Non-copyable, non-movable (singleton).
  ConfigManager(const ConfigManager&) = delete;
  ConfigManager& operator=(const ConfigManager&) = delete;
  ConfigManager(ConfigManager&&) = delete;
  ConfigManager& operator=(ConfigManager&&) = delete;

 private:
  ConfigManager() = default;
  ~ConfigManager() = default;

  // Serializes one category bucket to <dir_path>/<category>.json.
  [[nodiscard]] static bool SaveCategory(
      const QString& category, const std::vector<ConfigKeyBase*>& keys,
      const QString& dir_path);

  // Deserializes one JSON file and applies values to the registry.
  [[nodiscard]] static bool LoadFile(const QString& file_path);
};

}  // namespace qde::gui::config

#endif  // GUI_CONFIG_MANAGER_HPP_