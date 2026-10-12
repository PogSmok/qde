#ifndef GUI_THEME_HPP_
#define GUI_THEME_HPP_

#include <QColor>

namespace qde::gui::theme {

// Capped qubits/bits
inline constexpr int kMaxDisplayedQubits = 16;
inline constexpr int kMaxDisplayedBits = 16;

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
inline constexpr auto kCircuitViewBackground = "#19191f";
inline constexpr auto kSplitterHandleBackground = "#3a3a3a";
inline constexpr auto kControlBorder = "#3a3a3a";
inline constexpr auto kMenuBackground = "#2d2d2d";
inline constexpr auto kMenuText = "#ddd";
inline constexpr auto kMenuSelectedBackground = "#094771";
inline constexpr auto kMenuBarBackground = "#1e1e1e";
inline constexpr auto kMenuBarText = "#ccc";
inline constexpr auto kStatusBarText = "#aaa";
inline constexpr auto kStatusBarBackground = "#1e1e1e";
inline constexpr auto kSuccessText = "#4caf50";
inline constexpr auto kErrorText = "#f44336";

inline constexpr auto kTextBlockLeftPadding = 4;

// Editor syntax colours
inline constexpr auto kKeywordColor = "#2252ff";
inline constexpr auto kTypeColor = "#e288c0";
inline constexpr auto kVariableColor = "#9cdcfe";
inline constexpr auto kGateColor = "#e2c790";
inline constexpr auto kFunctionColor = "#c586c0";
inline constexpr auto kConstantColor = "#4fc1ff";
inline constexpr auto kNumberColor = "#73ca44";
inline constexpr auto kStringColor = "#b5e39c";
inline constexpr auto kOperatorColor = "#c8c8c8";
inline constexpr auto kCommentColor = "#90988c";

// Circuit view layout
inline constexpr int kLabelWidth = 80;
inline constexpr int kCellWidth = 64;
inline constexpr int kCellHeight = 52;
inline constexpr int kGateSize = 36;
inline constexpr int kDotRadius = 6;
inline constexpr int kTargetRadius = 14;
inline constexpr int kClassGap = 3;
inline constexpr int kCornerRadius = 4;
inline constexpr int kXMarkArm = 8;
inline constexpr int kDefaultMargin = 12;
inline constexpr int kDefaultWidth = 400;
inline constexpr int kDefaultHeight = 160;
inline constexpr int kLabelPadding = 1;
inline constexpr int kBarrierHalfWidth = 4;
inline constexpr int kGateShadowOffset = 2;
inline constexpr int kMeasureArcInset = 4;
inline constexpr int kGateBorderLighter = 160;
inline constexpr double kPenWidth = 1.5;
inline constexpr int kPlaceholderFontSize = 10;
inline constexpr int kWireLabelFontSize = 9;
inline constexpr auto kWireLabelFontFamily = "Consolas, Monospace";
inline constexpr auto kMessageLabelFontFamily = "Consolas, Monospace";

// Circuit view colors
inline constexpr QColor kCircuitBackground{25, 25, 35};
inline constexpr QColor kClassWireColor{120, 120, 120};
inline constexpr QColor kWireColor{180, 180, 180};
inline constexpr QColor kPlaceholderText{120, 120, 120};
inline constexpr QColor kBarrierFill{180, 180, 180, 30};
inline constexpr QColor kBarrierLine{180, 180, 180, 160};
inline constexpr QColor kGateShadow{0, 0, 0, 60};
inline constexpr QColor kGateMeasurement{255, 202, 40};
inline constexpr QColor kGateDefault{80, 80, 90};
inline const QColor kGateLabelText = Qt::white;
inline const QColor kMeasureArcColor = Qt::black;
// Standard gates colors
// Pauli: X red, Y green, Z blue. Controlled variants reuse the target gate's
// color.
inline constexpr QColor kGateX{198, 70, 70};
inline constexpr QColor kGateY{62, 150, 84};
inline constexpr QColor kGateZ{66, 106, 196};
inline constexpr QColor kGateCX{198, 70, 70};
inline constexpr QColor kGateCY{62, 150, 84};
inline constexpr QColor kGateCZ{66, 106, 196};
inline constexpr QColor kGateCCX{198, 70, 70};

// Hadamard: purple
inline constexpr QColor kGateH{142, 88, 186};
inline constexpr QColor kGateCH{142, 88, 186};

// sqrt(X): orange
inline constexpr QColor kGateSX{190, 108, 52};

// Phase family: teal/cyan shades
inline constexpr QColor kGateP{38, 150, 140};
inline constexpr QColor kGatePhase{38, 150, 140};
inline constexpr QColor kGateCP{38, 150, 140};
inline constexpr QColor kGateCPhase{38, 150, 140};
inline constexpr QColor kGateU1{38, 150, 140};
inline constexpr QColor kGateS{30, 130, 120};
inline constexpr QColor kGateSDG{22, 105, 98};
inline constexpr QColor kGateT{25, 118, 150};
inline constexpr QColor kGateTDG{20, 92, 118};

// Rotations: muted versions of the matching Pauli color
inline constexpr QColor kGateRX{160, 62, 62};
inline constexpr QColor kGateRY{54, 125, 72};
inline constexpr QColor kGateRZ{56, 88, 166};
inline constexpr QColor kGateCRX{160, 62, 62};
inline constexpr QColor kGateCRY{54, 125, 72};
inline constexpr QColor kGateCRZ{56, 88, 166};

// Swap family: pink
inline constexpr QColor kGateSwap{200, 88, 146};
inline constexpr QColor kGateCSwap{200, 88, 146};

// Generic unitaries: indigo, darker with more parameters
inline constexpr QColor kGateU{112, 102, 204};
inline constexpr QColor kGateCU{112, 102, 204};
inline constexpr QColor kGateU2{98, 90, 180};
inline constexpr QColor kGateU3{84, 78, 156};

// Identity: neutral gray, slightly lighter than kGateDefault
inline constexpr QColor kGateID{100, 100, 110};

// Simulation results (probabilities histogram) layout
inline constexpr int kHistDefaultWidth = 320;
inline constexpr int kHistDefaultHeight = 160;
inline constexpr int kHistMargin = 16;
inline constexpr int kHistAxisBottomPad = 28;  // room for basis labels
inline constexpr int kHistAxisLeftPad = 36;    // room for percentage labels
inline constexpr int kHistBarWidth = 28;
inline constexpr int kHistBarGap = 8;
inline constexpr int kMaxSimQubits = 6;
inline constexpr int kHistMaxStates = 1 << kMaxSimQubits;  // <= 6 qubits
inline constexpr int kHistTitleFontSize = 10;
inline constexpr int kHistLabelFontSize = 8;
inline constexpr int kHistTickCount = 4;  // 25/50/75/100 % gridlines
inline constexpr int kHistSelectorHeight = 24;
inline constexpr int kHistAxisLabelHeight = 24;
inline constexpr int kHistSelectorWidth = 150;
inline constexpr int kHistSelectorTop = 4;
inline constexpr double kHistPhaseSaturation = 0.65;
inline constexpr double kHistPhaseValue = 0.95;

// Simulation results colors
inline constexpr QColor kHistBackground{25, 25, 35};
inline constexpr QColor kHistBarColor{86, 156, 214};
inline constexpr QColor kHistAxisColor{120, 120, 130};
inline constexpr QColor kHistGridColor{60, 60, 70};
inline constexpr QColor kHistLabelColor{180, 180, 180};
inline constexpr QColor kHistPlaceholderText{120, 120, 120};
inline constexpr QColor kHistErrorText{220, 20, 10};

}  // namespace qde::gui::theme

#endif  // GUI_THEME_HPP_
