#ifndef GUI_VIEW_HELPERS_HPP_
#define GUI_VIEW_HELPERS_HPP_

#include <cstddef>

#include <QString>

namespace qde::gui {

// Formats an angle in radians for gate labels. Multiples of pi/4 are shown as
// fractions of pi (e.g. "π/2", "-3π/4"); other values use 3 significant digits.
QString FormatAngle(double radians);

// Ket label for basis state `index` over `qubit_count` qubits, most significant
// qubit first (q[n-1] ... q[0]), e.g. KetLabel(5, 3) == "|101⟩".
QString KetLabel(std::size_t index, std::size_t qubit_count);

}  // namespace qde::gui

#endif  // GUI_VIEW_HELPERS_HPP_
