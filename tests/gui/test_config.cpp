#include <gtest/gtest.h>

#include "qde/gui/config.hpp"

using namespace qde::gui::config;

TEST(ConfigTest, TabWidthValidator) {
  editor::tabWidth.setValue(4);
  EXPECT_EQ(editor::tabWidth.value(), 4);
  editor::tabWidth.setValue(1);
  EXPECT_EQ(editor::tabWidth.value(), 1);
  editor::tabWidth.setValue(16);
  EXPECT_EQ(editor::tabWidth.value(), 16);
  editor::tabWidth.setValue(0);
  EXPECT_EQ(editor::tabWidth.value(), 16);
  editor::tabWidth.setValue(20);
  EXPECT_EQ(editor::tabWidth.value(), 16);
  editor::tabWidth.setValue(-1);
  EXPECT_EQ(editor::tabWidth.value(), 16);
}

TEST(ConfigTest, FontSizeValidator) {
  editor::fontSize.setValue(11);
  EXPECT_EQ(editor::fontSize.value(), 11);
  editor::fontSize.setValue(6);
  EXPECT_EQ(editor::fontSize.value(), 6);
  editor::fontSize.setValue(72);
  EXPECT_EQ(editor::fontSize.value(), 72);
  editor::fontSize.setValue(0);
  EXPECT_EQ(editor::fontSize.value(), 72);
  editor::fontSize.setValue(100);
  EXPECT_EQ(editor::fontSize.value(), 72);
  editor::fontSize.setValue(-1);
  EXPECT_EQ(editor::fontSize.value(), 72);
}

TEST(ConfigTest, ThemeNameValidator) {
  theme::name.setValue("dark");
  EXPECT_EQ(theme::name.value(), "dark");
  theme::name.setValue("light");
  EXPECT_EQ(theme::name.value(), "light");
  theme::name.setValue("abc");
  EXPECT_EQ(theme::name.value(), "light");
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