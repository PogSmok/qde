#include <gtest/gtest.h>

#include <utility>
#include <vector>

#include <QChar>
#include <QString>

#include "qde/gui/view_helpers.hpp"

namespace qde::gui {

namespace {

constexpr double kPi = 3.141592653589793238462643383;

QString PiText() { return {QChar(u'\u03C0')}; }
QString KetCloseText() { return {QChar(u'\u27E9')}; }

}  // namespace

TEST(FormatAngleTest, HalfPiOmitsUnitNumerator) {
  EXPECT_EQ(FormatAngle(kPi / 2), PiText() + "/2");
}

TEST(FormatAngleTest, NegativeQuarterPiOmitsUnitNumerator) {
  EXPECT_EQ(FormatAngle(-kPi / 4), "-" + PiText() + "/4");
}

TEST(FormatAngleTest, ThreeQuartersPiKeepsNumerator) {
  EXPECT_EQ(FormatAngle(3 * kPi / 4), "3" + PiText() + "/4");
}

TEST(FormatAngleTest, WholePi) { EXPECT_EQ(FormatAngle(kPi), PiText()); }

TEST(FormatAngleTest, NegativeWholeMultiple) {
  EXPECT_EQ(FormatAngle(-2 * kPi), "-2" + PiText());
}

TEST(FormatAngleTest, NonFractionUsesDecimal) {
  EXPECT_EQ(FormatAngle(0.1), "0.1");
}

TEST(FormatAngleTest, ZeroUsesDecimal) { EXPECT_EQ(FormatAngle(0.0), "0"); }

TEST(KetLabelTest, MostSignificantQubitFirst) {
  EXPECT_EQ(KetLabel(5, 3), "|101" + KetCloseText());
}

TEST(KetLabelTest, ZeroQubitsShowsZero) {
  EXPECT_EQ(KetLabel(0, 0), "|0" + KetCloseText());
}

TEST(ContiguousRunsTest, GroupsSortsAndDeduplicates) {
  const std::vector<std::pair<int, int>> expected = {{0, 2}, {4, 4}};
  EXPECT_EQ(ContiguousRuns({2, 0, 1, 4, 1}), expected);
}

TEST(ContiguousRunsTest, EmptyInputGivesNoRuns) {
  EXPECT_TRUE(ContiguousRuns({}).empty());
}

}  // namespace qde::gui
