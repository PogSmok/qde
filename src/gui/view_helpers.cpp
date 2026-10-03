#include "qde/gui/view_helpers.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <utility>
#include <vector>

#include <QChar>
#include <QLatin1Char>
#include <QString>

namespace qde::gui {

namespace {

constexpr double kPi = 3.141592653589793238462643383;
constexpr char16_t kPiSymbol = u'\u03C0';
constexpr char16_t kKetClose = u'\u27E9';

}  // namespace

QString FormatAngle(const double radians) {
  constexpr int max_den = 4;
  constexpr double tolerance = 1e-6;
  const double frac = radians / kPi;
  const double rounded = std::round(frac * max_den) / max_den;
  if (rounded == 0.0 || std::abs(frac - rounded) >= tolerance) {
    return QString::number(radians, 'g', 3);
  }

  const int quarters = static_cast<int>(std::round(rounded * max_den));
  const int g = std::gcd(quarters, max_den);
  const int num = quarters / g;
  const int den = max_den / g;

  QString text;
  if (num == -1) {
    text = QStringLiteral("-");
  } else if (num != 1) {
    text = QString::number(num);
  }
  text += QChar(kPiSymbol);
  if (den != 1) {
    text += QLatin1Char('/') + QString::number(den);
  }
  return text;
}

QString KetLabel(const std::size_t index, const std::size_t qubit_count) {
  QString bits;
  for (std::size_t b = qubit_count; b-- > 0;) {
    bits += ((index >> b) & 1U) != 0U ? QLatin1Char('1') : QLatin1Char('0');
  }
  if (bits.isEmpty()) {
    bits = QStringLiteral("0");
  }
  return QLatin1Char('|') + bits + QChar(kKetClose);
}

std::vector<std::pair<int, int>> ContiguousRuns(std::vector<int> rows) {
  std::sort(rows.begin(), rows.end());
  rows.erase(std::unique(rows.begin(), rows.end()), rows.end());

  std::vector<std::pair<int, int>> runs;
  for (const int row : rows) {
    if (!runs.empty() && runs.back().second + 1 == row) {
      runs.back().second = row;
    } else {
      runs.emplace_back(row, row);
    }
  }
  return runs;
}

}  // namespace qde::gui
