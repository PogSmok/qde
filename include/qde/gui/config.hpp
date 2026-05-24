#ifndef GUI_CONFIG_HPP_
#define GUI_CONFIG_HPP_

#include <QKeySequence>

#include "qde/gui/config_key.hpp"

namespace qde::gui::config {

namespace editor {

inline ConfigKey<int> tab_width("editor", "tabWidth", 4, "Tab Width",
                                "Number of spaces per tab stop (1–16).",
                                [](const int& v) { return v >= 1 && v <= 16; });

inline ConfigKey<bool> use_spaces(
    "editor", "useSpaces", true, "Use Spaces",
    "Insert spaces instead of real tab characters.", nullptr);

inline ConfigKey<bool> word_wrap("editor", "wordWrap", false, "Word Wrap",
                                 "Wrap long lines at the viewport edge.",
                                 nullptr);

inline ConfigKey<int> font_size("editor", "fontSize", 11, "Font Size",
                                "Editor font size in points (6–72).",
                                [](const int& v) { return v >= 6 && v <= 72; });

inline ConfigKey<QString> font_family("editor", "fontFamily",
                                      QStringLiteral("Cascadia Code"),
                                      "Font Family",
                                      "Monospace font used in the editor pane.",
                                      nullptr);

inline ConfigKey<bool> line_numbers("editor", "lineNumbers", true,
                                    "Line Numbers",
                                    "Display line numbers in the gutter.",
                                    nullptr);

// Not Implemented
inline ConfigKey<bool> auto_save("editor", "autoSave", false, "Auto Save",
                                 "Save the file automatically after idle.",
                                 nullptr);

// Not Implemented
inline ConfigKey<int> auto_save_delay_ms(
    "editor", "autoSaveDelayMs", 1000, "Auto Save Delay (ms)",
    "Milliseconds of idle before auto-save fires (100–10 000).",
    [](const int& v) { return v >= 100 && v <= 10'000; });

inline ConfigKey<bool> highlight_current_line("editor", "highlightCurrentLine",
                                              true, "Highlight Current Line",
                                              "Tint the line the cursor is on.",
                                              nullptr);

// Not Implemented
inline ConfigKey<int> scroll_off_lines(
    "editor", "scrollOffLines", 5, "Scroll-Off Lines",
    "Minimum lines kept above/below cursor (0–20).",
    [](const int& v) { return v >= 0 && v <= 20; });

}  // namespace editor

namespace theme {

// Not Implemented
inline ConfigKey<QString> name("theme", "name", QStringLiteral("dark"), "Theme",
                               "UI color theme ('dark' | 'light').",
                               [](const QString& v) {
                                 static const QStringList kValid{
                                     QStringLiteral("dark"),
                                     QStringLiteral("light")};
                                 return kValid.contains(v);
                               });

inline ConfigKey<bool> syntax_highlighting("theme", "syntaxHighlighting", true,
                                           "Syntax Highlighting",
                                           "Colorize tokens based on grammar.",
                                           nullptr);

// Not Implemented
inline ConfigKey<bool> match_brackets("theme", "matchBrackets", true,
                                      "Match Brackets",
                                      "Highlight matching bracket/paren pairs.",
                                      nullptr);

// Not Implemented
inline ConfigKey<bool> render_whitespace(
    "theme", "renderWhitespace", false, "Render Whitespace",
    "Show spaces and tabs as visible glyphs.", nullptr);

}  // namespace theme

namespace shortcuts {

inline bool ShortcutValidator(const QKeySequence& v) {
  for (int i = 0; i < v.count(); i++) {
    if (v[i].key() == Qt::Key_unknown) {
      return false;
    }
  }
  return true;
}

inline ConfigKey<QKeySequence> file_new("shortcut", "fileNew",
                                        QKeySequence("Ctrl+N"),
                                        "Create new file",
                                        "Key shortcut for creating new file.",
                                        ShortcutValidator);

inline ConfigKey<QKeySequence> file_open(
    "shortcut", "fileOpen", QKeySequence("Ctrl+O"), "Open file",
    "Key shortcut for opening file via file system.", ShortcutValidator);

inline ConfigKey<QKeySequence> file_save(
    "shortcut", "fileSave", QKeySequence("Ctrl+S"), "Save file",
    "Key shortcut for saving current file.", ShortcutValidator);

inline ConfigKey<QKeySequence> file_save_as(
    "shortcut", "fileSaveAs", QKeySequence("Ctrl+Shift+S"), "Save file as",
    "Key shortcut for saving current buffer as new file.", ShortcutValidator);

inline ConfigKey<QKeySequence> edit_indent(
    "shortcut", "editIndent", QKeySequence("Tab"), "Indent block",
    "Key shortcut for indenting selected block of code.", ShortcutValidator);

inline ConfigKey<QKeySequence> edit_outdent(
    "shortcut", "editOutdent", QKeySequence("Shift+Tab"), "Outdent block",
    "Key shortcut for outdenting selected block of code.", ShortcutValidator);

inline ConfigKey<QKeySequence> edit_comment(
    "shortcut", "editComment", QKeySequence("Ctrl+/"), "Toggle block comment",
    "Key shortcut for toggling comment for selected block of code.",
    ShortcutValidator);

inline ConfigKey<QKeySequence> edit_move_block_up(
    "shortcut", "editMoveBlockUp", QKeySequence("Alt+Up"), "Move block up",
    "Key shortcut for moving selected block of code up a line.",
    ShortcutValidator);

inline ConfigKey<QKeySequence> edit_move_block_down(
    "shortcut", "editMoveBlockDown", QKeySequence("Alt+Down"),
    "Move block down",
    "Key shortcut for moving selected block of code down a line.",
    ShortcutValidator);

}  // namespace shortcuts

}  // namespace qde::gui::config

#endif  // GUI_CONFIG_HPP_