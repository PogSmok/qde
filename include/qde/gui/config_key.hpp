#ifndef GUI_CONFIG_KEY_HPP_
#define GUI_CONFIG_KEY_HPP_

#include <QJsonValue>
#include <QKeySequence>
#include <QVariant>
#include <utility>

namespace qde::gui::config {

// Keys self-register via an intrusive singly-linked list for stack allocation
class ConfigKeyBase {
 public:
  ConfigKeyBase(const char* category, const char* field_name,
                const char* display_name,
                const char* description) noexcept
      : category_(category),
        field_name_(field_name),
        display_name_(display_name),
        description_(description) {
    head_ = this;
  }

  virtual ~ConfigKeyBase() noexcept {
    if (head_ == this) {
      head_ = next_;
      return;
    }
    for (ConfigKeyBase* p = head_; p != nullptr; p = p->next_) {
      if (p->next_ == this) {
        p->next_ = next_;
        return;
      }
    }
  }

  ConfigKeyBase(const ConfigKeyBase&) = delete;
  ConfigKeyBase& operator=(const ConfigKeyBase&) = delete;
  ConfigKeyBase(ConfigKeyBase&&) = delete;
  ConfigKeyBase& operator=(ConfigKeyBase&&) = delete;

  [[nodiscard]] virtual QJsonValue ToJsonValue() const = 0;
  virtual bool FromJsonValue(const QJsonValue& json_value) = 0;

  [[nodiscard]] const char* Category() const noexcept { return category_; }
  [[nodiscard]] const char* FieldName() const noexcept { return field_name_; }
  [[nodiscard]] const char* DisplayName() const noexcept {
    return display_name_;
  }
  [[nodiscard]] const char* Description() const noexcept {
    return description_;
  }

  [[nodiscard]] QString Id() const {
    return QLatin1StringView{category_} + "." + QLatin1StringView{field_name_};
  }

  // ---- Registry (intrusive linked list) ------------------------------------

  [[nodiscard]] static ConfigKeyBase* First() noexcept { return head_; }
  [[nodiscard]] ConfigKeyBase* Next() const noexcept { return next_; }

  [[nodiscard]] static ConfigKeyBase* Find(const QString& id) noexcept {
    for (ConfigKeyBase* p = head_; p != nullptr; p = p->next_) {
      const QLatin1StringView cat{p->category_};
      const QLatin1StringView field{p->field_name_};
      if (id.size() == cat.size() + 1 + field.size() &&
          id.startsWith(cat) && id[cat.size()] == QLatin1Char('.') &&
          id.endsWith(field)) {
        return p;
      }
    }
    return nullptr;
  }

 private:
  inline static ConfigKeyBase* head_ = nullptr;
  ConfigKeyBase* next_{head_};
  const char* category_;
  const char* field_name_;
  const char* display_name_;
  const char* description_;
};

// ---- Serialization helpers ------------------------------------------------

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

// ---- Typed config key -----------------------------------------------------

template <typename T>
class ConfigKey : public ConfigKeyBase {
  // nullptr means "no validation".
  using Validator = bool (*)(const T&);

 public:
  ConfigKey(const char* category, const char* field_name, T default_value,
            const char* display_name, const char* description,
            Validator validator) noexcept
      : ConfigKeyBase(category, field_name, display_name, description),
        value_(std::move(default_value)),
        validator_(validator) {}

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

  [[nodiscard]] T Value() const { return value_; }

  bool SetValue(T value) {
    if (!Validate(value)) {
      return false;
    }
    value_ = std::move(value);
    return true;
  }

  [[nodiscard]] bool Validate(const T& val) const {
    return validator_ == nullptr || validator_(val);
  }

 private:
  T value_;
  Validator validator_;
};

}  // namespace qde::gui::config

#endif  // GUI_CONFIG_KEY_HPP_
