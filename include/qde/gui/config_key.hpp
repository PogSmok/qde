#ifndef GUI_CONFIG_KEY_HPP_
#define GUI_CONFIG_KEY_HPP_

#include <QJsonValue>
#include <QKeySequence>
#include <QVariant>
#include <functional>
#include <optional>
#include <utility>

namespace qde::gui::config {

class ConfigKeyBase {
 public:
  explicit ConfigKeyBase(QString id) : registered_id_(std::move(id)) {
    config_registry[registered_id_] = this;
  }

  virtual ~ConfigKeyBase() { config_registry.erase(registered_id_); }
  ConfigKeyBase(const ConfigKeyBase&) = delete;
  virtual ConfigKeyBase& operator=(const ConfigKeyBase&) = delete;
  ConfigKeyBase(ConfigKeyBase&&) = delete;
  virtual ConfigKeyBase& operator=(ConfigKeyBase&&) = delete;

  [[nodiscard]] virtual QJsonValue ToJsonValue() const = 0;
  virtual bool FromJsonValue(const QJsonValue& json_value) = 0;
  [[nodiscard]] virtual QString Category() const = 0;
  [[nodiscard]] virtual QString FieldName() const = 0;
  [[nodiscard]] virtual QString Id() const = 0;
  [[nodiscard]] virtual QString DisplayName() const = 0;
  [[nodiscard]] virtual QString Description() const = 0;

  inline static auto config_registry = std::map<QString, ConfigKeyBase*>();

 private:
  QString registered_id_;
};

template <typename T>
struct ConfigKeySerializer {
  [[nodiscard]] static QJsonValue ToJson(const T& value) {
    return QJsonValue::fromVariant(QVariant::fromValue(value));
  }

  [[nodiscard]] static T FromJson(const QJsonValue& json_value,
                                  const T& fallback) {
    const QVariant var = json_value.toVariant();
    if (!var.canConvert<T>()) {
      return fallback;
    }
    return var.value<T>();
  }
};

template <>
struct ConfigKeySerializer<QKeySequence> {
  [[nodiscard]] static QJsonValue ToJson(const QKeySequence& value) {
    return {value.toString(QKeySequence::PortableText)};
  }

  [[nodiscard]] static QKeySequence FromJson(const QJsonValue& json_value,
                                             const QKeySequence& fallback) {
    if (!json_value.isString()) {
      return fallback;
    }
    return {json_value.toString(), QKeySequence::PortableText};
  }
};

template <typename T>
class ConfigKey : public ConfigKeyBase {
  using Validator = std::function<bool(const T&)>;

 public:
  ConfigKey(QString category, QString field_name, T default_value,
            QString display_name, QString description,
            std::optional<Validator> validator)
      : ConfigKeyBase(category + "." + field_name),
        category_(std::move(category)),
        fieldName_(std::move(field_name)),
        value_(default_value),
        displayName_(std::move(display_name)),
        description_(std::move(description)),
        validator_(std::move(validator)) {
    if (validator_ && !(*validator_)) {
      validator_.reset();
    }
  }

  ~ConfigKey() override = default;
  ConfigKey(const ConfigKey&) = delete;
  ConfigKey& operator=(const ConfigKey&) = delete;
  ConfigKey(ConfigKey&&) = delete;
  ConfigKey& operator=(ConfigKey&&) = delete;

  [[nodiscard]] QJsonValue ToJsonValue() const override {
    return ConfigKeySerializer<T>::ToJson(value_);
  }

  bool FromJsonValue(const QJsonValue& json_value) override {
    T candidate = ConfigKeySerializer<T>::FromJson(json_value, value_);
    return SetValue(std::move(candidate));
  }

  [[nodiscard]] QString Category() const override { return category_; }

  [[nodiscard]] QString FieldName() const override { return fieldName_; }

  [[nodiscard]] QString Id() const override {
    return category_ + "." + fieldName_;
  }

  [[nodiscard]] QString DisplayName() const override { return displayName_; }

  [[nodiscard]] QString Description() const override { return description_; }

  [[nodiscard]] T Value() const { return value_; }

  bool SetValue(T value) {
    if (!Validate(value)) {
      return false;
    }
    value_ = std::move(value);
    return true;
  }

  [[nodiscard]] bool Validate(const T& val) const {
    return !validator_ || !(*validator_) || (*validator_)(val);
  }

 private:
  QString category_;
  QString fieldName_;
  T value_;
  QString displayName_;
  QString description_;
  std::optional<Validator> validator_;
};

}  // namespace qde::gui::config

#endif  // GUI_CONFIG_KEY_HPP_