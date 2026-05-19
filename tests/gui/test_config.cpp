#include <gtest/gtest.h>

#include "qde/gui/config.hpp"

using namespace qde::gui::config;

TEST(ConfigTest, TabWidthValidator) {
  editor::tabWidth.SetValue(4);
  EXPECT_EQ(editor::tabWidth.Value(), 4);
  editor::tabWidth.SetValue(1);
  EXPECT_EQ(editor::tabWidth.Value(), 1);
  editor::tabWidth.SetValue(16);
  EXPECT_EQ(editor::tabWidth.Value(), 16);
  editor::tabWidth.SetValue(0);
  EXPECT_EQ(editor::tabWidth.Value(), 16);
  editor::tabWidth.SetValue(20);
  EXPECT_EQ(editor::tabWidth.Value(), 16);
  editor::tabWidth.SetValue(-1);
  EXPECT_EQ(editor::tabWidth.Value(), 16);
}

TEST(ConfigTest, FontSizeValidator) {
  editor::fontSize.SetValue(11);
  EXPECT_EQ(editor::fontSize.Value(), 11);
  editor::fontSize.SetValue(6);
  EXPECT_EQ(editor::fontSize.Value(), 6);
  editor::fontSize.SetValue(72);
  EXPECT_EQ(editor::fontSize.Value(), 72);
  editor::fontSize.SetValue(0);
  EXPECT_EQ(editor::fontSize.Value(), 72);
  editor::fontSize.SetValue(100);
  EXPECT_EQ(editor::fontSize.Value(), 72);
  editor::fontSize.SetValue(-1);
  EXPECT_EQ(editor::fontSize.Value(), 72);
}

TEST(ConfigTest, ThemeNameValidator) {
  theme::name.SetValue("dark");
  EXPECT_EQ(theme::name.Value(), "dark");
  theme::name.SetValue("light");
  EXPECT_EQ(theme::name.Value(), "light");
  theme::name.SetValue("abc");
  EXPECT_EQ(theme::name.Value(), "light");
}

TEST(ConfigTest, ShortcutValidator) {
  EXPECT_TRUE(shortcuts::shortcutValidator(QKeySequence("Tab")));
  EXPECT_TRUE(shortcuts::shortcutValidator(QKeySequence("Shift+Tab")));
  EXPECT_TRUE(shortcuts::shortcutValidator(QKeySequence("Alt+Ctrl+Tab")));
  EXPECT_TRUE(shortcuts::shortcutValidator(QKeySequence("Alt+Up")));
  EXPECT_FALSE(shortcuts::shortcutValidator(QKeySequence("TabTab")));
  EXPECT_FALSE(shortcuts::shortcutValidator(QKeySequence("-1")));
  EXPECT_FALSE(shortcuts::shortcutValidator(QKeySequence("ABC")));
}