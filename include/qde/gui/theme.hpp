#ifndef GUI_THEME_HPP_
#define GUI_THEME_HPP_

#include <QColor>

namespace qde::gui::theme {

// Editor colors
inline const QColor editorBackground(30, 30, 30);
inline const QColor editorText(212, 212, 212);
inline const QColor lineNumberBackground(40, 40, 40);
inline const QColor lineNumberText(133, 133, 133);
inline const QColor errorUnderline = Qt::red;

// Window colors
inline const QColor windowBackground(30, 30, 30);
inline const QColor windowText(212, 212, 212);
inline const QColor baseBackground(18, 18, 18);
inline const QColor textColor(212, 212, 212);
inline const QColor buttonBackground(45, 45, 45);
inline const QColor buttonText(212, 212, 212);
inline const QColor highlightBackground(86, 156, 214);

// Stylesheet colors
inline constexpr auto splitterHandleBackground = "#3a3a3a";
inline constexpr auto menuBackground = "#2d2d2d";
inline constexpr auto menuText = "#ddd";
inline constexpr auto menuSelectedBackground = "#094771";
inline constexpr auto menuBarBackground = "#1e1e1e";
inline constexpr auto menuBarText = "#ccc";
inline constexpr auto statusBarText = "#aaa";
inline constexpr auto statusBarBackground = "#1e1e1e";
inline constexpr auto successText = "#4CAF50";
inline constexpr auto errorText = "#F44336";

// Editor font
inline constexpr auto editorFontFamily = "Cascadia Code";
inline constexpr auto editorFontSize = 11;
inline constexpr auto editorTabStop = 4;

inline constexpr auto textBlockLeftPadding = 4;

}  // namespace qde::gui::theme

#endif  // GUI_THEME_HPP_
