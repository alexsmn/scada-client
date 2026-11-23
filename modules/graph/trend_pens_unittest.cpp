#include "graph/trend_pens.h"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

namespace {

const QColor kDarkCanvas{0x17, 0x17, 0x17};
const QColor kLightCanvas{0xff, 0xff, 0xff};
const QColor kHighContrastCanvas{0x00, 0x00, 0x00};

double Luminance(const QColor& color) {
  auto channel = [](int value) {
    const double c = value / 255.0;
    return c <= 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
  };
  return 0.2126 * channel(color.red()) + 0.7152 * channel(color.green()) +
         0.0722 * channel(color.blue());
}

double Contrast(const QColor& a, const QColor& b) {
  const double la = Luminance(a);
  const double lb = Luminance(b);
  return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

// Angular distance between two hues, in degrees.
int HueDistance(int a, int b) {
  const int d = std::abs(a - b) % 360;
  return std::min(d, 360 - d);
}

// No default pen may sit in a hue the client reserves for a process signal:
// red through yellow (alarm severity, docs/client/ux/principles.md §1), the
// low-severity blue (~210°) or the quality green (~158°). A grey has no hue
// and is exempt.
TEST(TrendPensTest, SequenceAvoidsTheReservedHues) {
  for (const QColor& pen : TrendPenSequence()) {
    if (pen.hslSaturationF() < 0.15)
      continue;
    const int hue = pen.hslHue();
    EXPECT_FALSE(hue >= 330 || hue <= 70) << pen.name().toStdString();
    EXPECT_GE(HueDistance(hue, 158), 15) << pen.name().toStdString();
    EXPECT_GE(HueDistance(hue, 210), 15) << pen.name().toStdString();
  }
}

// A pen is persisted with the series and survives an appearance switch, so it
// must read on every canvas: WCAG 1.4.11's 3:1 non-text minimum on each.
TEST(TrendPensTest, EveryPenReadsOnEveryCanvas) {
  for (const QColor& pen : TrendPenSequence()) {
    for (const QColor& canvas :
         {kDarkCanvas, kLightCanvas, kHighContrastCanvas}) {
      EXPECT_GE(Contrast(pen, canvas), 3.0)
          << pen.name().toStdString() << " on " << canvas.name().toStdString();
    }
  }
}

TEST(TrendPensTest, PensAreDistinct) {
  const auto pens = TrendPenSequence();
  for (size_t i = 0; i < pens.size(); ++i) {
    for (size_t j = i + 1; j < pens.size(); ++j)
      EXPECT_NE(pens[i], pens[j]);
  }
}

TEST(TrendPensTest, FirstSeriesGetsTheFirstPenOnEitherCanvas) {
  EXPECT_EQ(ChooseNewPenColor(QColor{}, kDarkCanvas, {}),
            TrendPenSequence()[0]);
  EXPECT_EQ(ChooseNewPenColor(QColor{}, kLightCanvas, {}),
            TrendPenSequence()[0]);
}

TEST(TrendPensTest, SkipsPensAlreadyInUse) {
  const std::vector<QColor> used = {TrendPenSequence()[0],
                                    TrendPenSequence()[2]};
  EXPECT_EQ(ChooseNewPenColor(QColor{}, kDarkCanvas, used),
            TrendPenSequence()[1]);
}

TEST(TrendPensTest, RepeatsTheSequenceOnceEveryPenIsTaken) {
  const auto pens = TrendPenSequence();
  std::vector<QColor> used{pens.begin(), pens.end()};
  EXPECT_EQ(ChooseNewPenColor(QColor{}, kDarkCanvas, used), pens[0]);
  used.push_back(pens[0]);
  EXPECT_EQ(ChooseNewPenColor(QColor{}, kDarkCanvas, used), pens[1]);
}

// The operator's own default pen is honoured whatever its hue...
TEST(TrendPensTest, HonoursAPreferredPen) {
  EXPECT_EQ(ChooseNewPenColor(QColor{Qt::red}, kDarkCanvas, {}),
            QColor{Qt::red});
}

// ...but not twice, and not where it cannot be seen.
TEST(TrendPensTest, PreferredPenGivesWayWhenTakenOrUnreadable) {
  const std::vector<QColor> used = {QColor{Qt::red}};
  EXPECT_EQ(ChooseNewPenColor(QColor{Qt::red}, kDarkCanvas, used),
            TrendPenSequence()[0]);
  EXPECT_EQ(ChooseNewPenColor(QColor{Qt::black}, kDarkCanvas, {}),
            TrendPenSequence()[0]);
}

}  // namespace
