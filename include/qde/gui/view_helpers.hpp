#ifndef GUI_VIEW_HELPERS_HPP_
#define GUI_VIEW_HELPERS_HPP_

#include <cstddef>
#include <utility>
#include <vector>

#include <QString>

namespace qde::gui {

// Formats an angle in radians for gate labels. Multiples of pi/4 are shown as
// fractions of pi (e.g. "π/2", "-3π/4"); other values use 3 significant digits.
QString FormatAngle(double radians);

// Ket label for basis state `index` over `qubit_count` qubits, most significant
// qubit first (q[n-1] ... q[0]), e.g. KetLabel(5, 3) == "|101⟩".
QString KetLabel(std::size_t index, std::size_t qubit_count);

// Groups wire rows into maximal runs of consecutive rows, returned as
// inclusive (first, last) pairs in ascending order. Duplicates are ignored.
std::vector<std::pair<int, int>> ContiguousRuns(std::vector<int> rows);

}  // namespace qde::gui

#endif  // GUI_VIEW_HELPERS_HPP_
