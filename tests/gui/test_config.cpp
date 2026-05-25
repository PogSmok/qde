#include <gtest/gtest.h>

#include "qde/gui/config.hpp"

using namespace qde::gui::config;

TEST(ConfigTest, TabWidthValidator) {
  editor::tab_width.SetValue(4);
  EXPECT_EQ(editor::tab_width.Value(), 4);
  editor::tab_width.SetValue(1);
  EXPECT_EQ(editor::tab_width.Value(), 1);
  editor::tab_width.SetValue(16);
  EXPECT_EQ(editor::tab_width.Value(), 16);
  editor::tab_width.SetValue(0);
  EXPECT_EQ(editor::tab_width.Value(), 16);
  editor::tab_width.SetValue(20);
  EXPECT_EQ(editor::tab_width.Value(), 16);
  editor::tab_width.SetValue(-1);
  EXPECT_EQ(editor::tab_width.Value(), 16);
}

TEST(ConfigTest, FontSizeValidator) {
  editor::font_size.SetValue(11);
  EXPECT_EQ(editor::font_size.Value(), 11);
  editor::font_size.SetValue(6);
  EXPECT_EQ(editor::font_size.Value(), 6);
  editor::font_size.SetValue(72);
  EXPECT_EQ(editor::font_size.Value(), 72);
  editor::font_size.SetValue(0);
  EXPECT_EQ(editor::font_size.Value(), 72);
  editor::font_size.SetValue(100);
  EXPECT_EQ(editor::font_size.Value(), 72);
  editor::font_size.SetValue(-1);
  EXPECT_EQ(editor::font_size.Value(), 72);
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
  EXPECT_TRUE(shortcuts::ShortcutValidator(QKeySequence("Tab")));
  EXPECT_TRUE(shortcuts::ShortcutValidator(QKeySequence("Shift+Tab")));
  EXPECT_TRUE(shortcuts::ShortcutValidator(QKeySequence("Alt+Ctrl+Tab")));
  EXPECT_TRUE(shortcuts::ShortcutValidator(QKeySequence("Alt+Up")));
  EXPECT_FALSE(shortcuts::ShortcutValidator(QKeySequence("TabTab")));
  EXPECT_FALSE(shortcuts::ShortcutValidator(QKeySequence("-1")));
  EXPECT_FALSE(shortcuts::ShortcutValidator(QKeySequence("ABC")));
}