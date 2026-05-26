#include <gtest/gtest.h>
#include "qde/gui/config_key.hpp"

using namespace qde::gui::config;

TEST(ConfigKey, KeyInitialization) {
  ConfigKey<int> g("test", "testField", 4, "Display Name", "Description",
                   [](const int& v) { return v >= 1 && v <= 16; });
  EXPECT_STREQ(g.Category(), "test");
  EXPECT_STREQ(g.FieldName(), "testField");
  EXPECT_EQ(g.Id(), "test.testField");
  EXPECT_STREQ(g.DisplayName(), "Display Name");
  EXPECT_STREQ(g.Description(), "Description");
  EXPECT_EQ(g.Value(), 4);
}

TEST(ConfigKey, KeyContraints) {
  ConfigKey<int> g("test", "testField", 4, "Display Name", "Description",
                   [](const int& v) { return v >= 1 && v <= 16; });

  auto correct_value = g.SetValue(2);
  EXPECT_EQ(correct_value, true);
  EXPECT_EQ(g.Value(), 2);

  auto wrong_value = g.SetValue(-1);
  EXPECT_EQ(wrong_value, false);
  EXPECT_EQ(g.Value(), 2);
}

TEST(ConfigKey, KeyToJsonValue) {
  ConfigKey<int> g("test", "testField", 4, "Display Name", "Description",
                   [](const int& v) { return v >= 1 && v <= 16; });

  auto json_value = g.ToJsonValue();
  EXPECT_EQ(json_value, 4);
}

TEST(ConfigKey, KeyFromJsonValue) {
  ConfigKey<int> g("test", "testField", 4, "Display Name", "Description",
                   [](const int& v) { return v >= 1 && v <= 16; });

  auto correct_value = g.FromJsonValue(QJsonValue(2));
  EXPECT_EQ(correct_value, true);
  EXPECT_EQ(g.Value(), 2);

  auto wrong_value = g.FromJsonValue(QJsonValue(-1));
  EXPECT_EQ(wrong_value, false);
  EXPECT_EQ(g.Value(), 2);
}
