#ifndef GUI_THEME_HPP_
#define GUI_THEME_HPP_

#include <QColor>

namespace qde::gui::theme {

// Editor colors
inline constexpr QColor kEditorBackground(30, 30, 30);
inline constexpr QColor kEditorText(211, 211, 211);
inline constexpr QColor kLineNumberBackground(40, 40, 40);
inline constexpr QColor kLineNumberText(133, 133, 133);
inline const QColor kErrorUnderline = Qt::red;

// Window colors
inline constexpr QColor kWindowBackground(30, 30, 30);
inline constexpr QColor kWindowText(212, 212, 212);
inline constexpr QColor kBaseBackground(18, 18, 18);
inline constexpr QColor kTextColor(212, 212, 212);
inline constexpr QColor kButtonBackground(45, 45, 45);
inline constexpr QColor kButtonText(212, 212, 212);
inline constexpr QColor kHighlightBackground(86, 156, 214);

// Stylesheet colors
inline constexpr auto kSplitterHandleBackground = "#3a3a3a";
inline constexpr auto kMenuBackground = "#2d2d2d";
inline constexpr auto kMenuText = "#ddd";
inline constexpr auto kMenuSelectedBackground = "#094771";
inline constexpr auto kMenuBarBackground = "#1e1e1e";
inline constexpr auto kMenuBarText = "#ccc";
inline constexpr auto kStatusBarText = "#aaa";
inline constexpr auto kStatusBarBackground = "#1e1e1e";
inline constexpr auto kSuccessText = "#4CAF50";
inline constexpr auto kErrorText = "#F44336";

// Editor font
inline constexpr auto kEditorFontFamily = "Cascadia Code";
inline constexpr auto kEditorFontSize = 11;
inline constexpr auto kEditorTabStop = 4;

inline constexpr auto kTextBlockLeftPadding = 4;

}  // namespace qde::gui::theme

#endif  // GUI_THEME_HPP_
