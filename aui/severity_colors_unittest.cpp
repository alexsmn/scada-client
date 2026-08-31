#include "aui/severity_colors.h"

#include <gtest/gtest.h>

namespace scada::aui {
namespace {

// Builds a colour without exposing commas to the EXPECT_* macros.
Color C(int r, int g, int b) {
  return Color{Rgba{static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g),
                    static_cast<std::uint8_t>(b)}};
}

// Restores the process-wide severity theme so tests don't leak state into each
// other (the theme is a single global, set by ApplyTheme in production).
class SeverityColorsTest : public testing::Test {
 protected:
  void TearDown() override { SetSeverityTheme(SeverityTheme::kDark); }
};

// Dark is what the ramp answers before anything sets it — the appearance the
// client resolves to when the host reports no light/dark preference.
TEST_F(SeverityColorsTest, DarkIsTheDefault) {
  EXPECT_EQ(GetSeverityTheme(), SeverityTheme::kDark);
  EXPECT_EQ(EventRowColorsFor(EventBackground::kCritical).background,
            C(232, 90, 82));
}

// Switching the theme re-sources every class from the token ramp at once — this
// is the whole point of the single source.
TEST_F(SeverityColorsTest, ThemesResolveFromTheRamp) {
  SetSeverityTheme(SeverityTheme::kDark);
  // Critical -> severity-critical (dark), with dark text on the bright fill.
  const EventRowColors dark_critical =
      EventRowColorsFor(EventBackground::kCritical);
  EXPECT_EQ(dark_critical.background, C(232, 90, 82));
  EXPECT_EQ(dark_critical.text, C(11, 22, 35));

  SetSeverityTheme(SeverityTheme::kLight);
  // The same class now resolves to the light ramp with light text.
  const EventRowColors light_critical =
      EventRowColorsFor(EventBackground::kCritical);
  EXPECT_EQ(light_critical.background, C(143, 36, 31));
  EXPECT_EQ(light_critical.text, C(255, 255, 255));

  SetSeverityTheme(SeverityTheme::kHighContrast);
  const EventRowColors hc_critical =
      EventRowColorsFor(EventBackground::kCritical);
  EXPECT_EQ(hc_critical.background, C(255, 107, 107));
  EXPECT_EQ(hc_critical.text, C(0, 0, 0));
}

// Warning and critical resolve to different fills in every theme, so the row
// colour carries the severity rather than merely "something is wrong".
TEST_F(SeverityColorsTest, WarningAndCriticalNeverCollide) {
  for (const SeverityTheme theme : {SeverityTheme::kDark, SeverityTheme::kLight,
                                    SeverityTheme::kHighContrast}) {
    SetSeverityTheme(theme);
    EXPECT_NE(EventRowColorsFor(EventBackground::kCritical).background,
              EventRowColorsFor(EventBackground::kWarning).background);
  }
}

// The solid severity colour (status text / dots) is absent for kNone — the
// absence of a severity — and follows the ramp otherwise.
TEST_F(SeverityColorsTest, SolidSeverityColourIsAbsentOnlyForNone) {
  SetSeverityTheme(SeverityTheme::kDark);
  EXPECT_FALSE(SeverityColor(SeverityLevel::kNone).has_value());
  ASSERT_TRUE(SeverityColor(SeverityLevel::kCritical).has_value());
  EXPECT_EQ(*SeverityColor(SeverityLevel::kCritical), C(232, 90, 82));
  ASSERT_TRUE(SeverityColor(SeverityLevel::kWarning).has_value());
  EXPECT_EQ(*SeverityColor(SeverityLevel::kWarning), C(230, 178, 75));

  SetSeverityTheme(SeverityTheme::kLight);
  EXPECT_EQ(*SeverityColor(SeverityLevel::kCritical), C(143, 36, 31));
}

// The Explorer status-dot quality colour follows the good/uncertain/bad tokens.
// It is unconditional: every quality has a colour in every theme.
TEST_F(SeverityColorsTest, QualityColourFollowsTheTheme) {
  SetSeverityTheme(SeverityTheme::kDark);
  EXPECT_EQ(QualityColor(Quality::kGood), C(68, 192, 145));
  EXPECT_EQ(QualityColor(Quality::kUncertain), C(230, 178, 75));
  EXPECT_EQ(QualityColor(Quality::kBad), C(240, 113, 104));

  SetSeverityTheme(SeverityTheme::kLight);
  EXPECT_EQ(QualityColor(Quality::kBad), C(197, 61, 53));

  SetSeverityTheme(SeverityTheme::kHighContrast);
  EXPECT_EQ(QualityColor(Quality::kBad), C(255, 107, 107));
}

}  // namespace
}  // namespace scada::aui
