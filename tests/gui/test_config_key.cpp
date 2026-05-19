#include <gtest/gtest.h>
#include "qde/gui/config_key.hpp"

using namespace qde::gui::config;

TEST(ConfigKey, KeyInitialization) {
  ConfigKey<int> g("test", "testField", 4, "Display Name", "Description",
                   [](const int& v) { return v >= 1 && v <= 16; });
  EXPECT_EQ(g.category(), "test");
  EXPECT_EQ(g.fieldName(), "testField");
  EXPECT_EQ(g.id(), "test.testField");
  EXPECT_EQ(g.displayName(), "Display Name");
  EXPECT_EQ(g.description(), "Description");
  EXPECT_EQ(g.value(), 4);
}

TEST(ConfigKey, KeyContraints) {
  ConfigKey<int> g("test", "testField", 4, "Display Name", "Description",
                   [](const int& v) { return v >= 1 && v <= 16; });

  auto correct_value = g.setValue(2);
  EXPECT_EQ(correct_value, true);
  EXPECT_EQ(g.value(), 2);

  auto wrong_value = g.setValue(-1);
  EXPECT_EQ(wrong_value, false);
  EXPECT_EQ(g.value(), 2);
}

TEST(ConfigKey, KeyToJsonValue) {
  ConfigKey<int> g("test", "testField", 4, "Display Name", "Description",
                   [](const int& v) { return v >= 1 && v <= 16; });

  auto json_value = g.toJsonValue();
  EXPECT_EQ(json_value, 4);
}

TEST(ConfigKey, KeyFromJsonValue) {
  ConfigKey<int> g("test", "testField", 4, "Display Name", "Description",
                   [](const int& v) { return v >= 1 && v <= 16; });

  auto correct_value = g.fromJsonValue(QJsonValue(2));
  EXPECT_EQ(correct_value, true);
  EXPECT_EQ(g.value(), 2);

  auto wrong_value = g.fromJsonValue(QJsonValue(-1));
  EXPECT_EQ(wrong_value, false);
  EXPECT_EQ(g.value(), 2);
}
