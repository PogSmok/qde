#ifndef GUI_CONFIG_HPP_
#define GUI_CONFIG_HPP_

#include <QKeySequence>

#include "qde/gui/config_key.hpp"

// ============================================================
// Single source of truth for every setting in the IDE.
// Each key is an `inline const ConfigKey<T>` global — one
// definition, zero runtime cost, no header ordering issues.
//
// Add a new setting by adding one line here.
// The type system enforces correct get()/set() call sites.
//
// ============================================================

namespace qde::gui::config {

namespace editor {

inline const ConfigKey<int> tabWidth("editor", "tabWidth", 4, "Tab Width",
                                     "Number of spaces per tab stop (1–16).",
                               [](const int& v) { return v >= 1 && v <= 16;
                                     });

inline ConfigKey<bool> useSpaces(
    "editor", "useSpaces", true, "Use Spaces",
    "Insert spaces instead of real tab characters.", nullptr);

inline ConfigKey<bool> wordWrap("editor", "wordWrap", false, "Word Wrap",
                                "Wrap long lines at the viewport edge.",
                                nullptr);

inline ConfigKey<int> fontSize("editor", "fontSize", 11, "Font Size",
                               "Editor font size in points (6–72).",
                               [](const int& v) { return v >= 6 && v <= 72; });

inline ConfigKey<QString> fontFamily("editor", "fontFamily",
                                     QStringLiteral("Cascadia Code"),
                                     "Font Family",
                                     "Monospace font used in the editor pane.",
                                     nullptr);

inline ConfigKey<bool> lineNumbers("editor", "lineNumbers", true,
                                   "Line Numbers",
                                   "Display line numbers in the gutter.",
                                   nullptr);

inline ConfigKey<bool> autoSave(
    // Not Implemented
    "editor", "autoSave", false, "Auto Save",
    "Save the file automatically after idle.", nullptr);

inline ConfigKey<int> autoSaveDelayMs(
    // Not Implemented
    "editor", "autoSaveDelayMs", 1000, "Auto Save Delay (ms)",
    "Milliseconds of idle before auto-save fires (100–10 000).",
    [](const int& v) { return v >= 100 && v <= 10'000; });

inline ConfigKey<bool> highlightCurrentLine("editor", "highlightCurrentLine",
                                            true, "Highlight Current Line",
                                            "Tint the line the cursor is on.",
                                            nullptr);

inline ConfigKey<int> scrollOffLines(
    // Not Implemented
    "editor", "scrollOffLines", 5, "Scroll-Off Lines",
    "Minimum lines kept above/below cursor (0–20).",
    [](const int& v) { return v >= 0 && v <= 20; });

}  // namespace editor

namespace theme {

inline ConfigKey<QString> name(
    // Not Implemented
    "theme", "name", QStringLiteral("dark"), "Theme",
    "UI color theme ('dark' | 'light').", [](const QString& v) {
      static const QStringList valid{QStringLiteral("dark"),
                                     QStringLiteral("light")};
      return valid.contains(v);
    });

inline ConfigKey<bool> syntaxHighlighting("theme", "syntaxHighlighting", true,
                                          "Syntax Highlighting",
                                          "Colorize tokens based on grammar.",
                                          nullptr);

inline ConfigKey<bool> matchBrackets(
    // Not Implemented
    "theme", "matchBrackets", true, "Match Brackets",
    "Highlight matching bracket/paren pairs.", nullptr);

inline ConfigKey<bool> renderWhitespace(
    // Not Implemented
    "theme", "renderWhitespace", false, "Render Whitespace",
    "Show spaces and tabs as visible glyphs.", nullptr);

}  // namespace theme

namespace shortcuts {

inline bool shortcutValidator(const QKeySequence& v) {
  for (int i = 0; i < v.count(); i++) {
    if (v[i].key() == Qt::Key_unknown) {
      return false;
    }
  }
  return true;
}

inline ConfigKey<QKeySequence> fileNew("shortcut", "fileNew",
                                       QKeySequence("Ctrl+n"),
                                       "Create new file",
                                       "Key shortcut for creating new file.",
                                       shortcutValidator);

inline ConfigKey<QKeySequence> fileOpen(
    "shortcut", "fileOpen", QKeySequence("Ctrl+o"), "Open file",
    "Key shortcut for opening file via file system.", shortcutValidator);

inline ConfigKey<QKeySequence> fileSave("shortcut", "fileSave",
                                        QKeySequence("Ctrl+s"), "Save file",
                                        "Key shortcut for saving current file.",
                                        shortcutValidator);

inline ConfigKey<QKeySequence> fileSaveAs(
    "shortcut", "fileSaveAs", QKeySequence("Ctrl+Shift+s"), "Save file as",
    "Key shortcut for saving current buffer as new file.", shortcutValidator);

inline ConfigKey<QKeySequence> editIndent(
    "shortcut", "editIndent", QKeySequence("Tab"), "Indent block",
    "Key shortcut for indenting selected block of code.", shortcutValidator);

inline ConfigKey<QKeySequence> editOutdent(
    "shortcut", "editOutdent", QKeySequence("Shift+Tab"), "Outdent block",
    "Key shortcut for outdenting selected block of code.", shortcutValidator);

inline ConfigKey<QKeySequence> editComment(
    "shortcut", "editComment", QKeySequence("Ctrl+/"), "Toggle block comment",
    "Key shortcut for toggling comment for selected block of code.",
    shortcutValidator);

inline ConfigKey<QKeySequence> editMoveBlockUp(
    "shortcut", "editMoveBlockUp", QKeySequence("Alt+Up"), "Move block up",
    "Key shortcut for moving selected block of code up a line.",
    shortcutValidator);

inline ConfigKey<QKeySequence> editMoveBlockDown(
    "shortcut", "editMoveBlockDown", QKeySequence("Alt+Down"),
    "Move block down",
    "Key shortcut for moving selected block of code down a line.",
    shortcutValidator);

}  // namespace shortcuts

}  // namespace qde::gui::config

#endif  // GUI_CONFIG_HPP_