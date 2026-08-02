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
// other (the theme is a single global set once at startup in production).
class SeverityColorsTest : public testing::Test {
 protected:
  void TearDown() override { SetSeverityTheme(SeverityTheme::kLegacy); }
};

// The default theme is legacy, and it reproduces the exact historical event-row
// colours with the text colour left untouched.
TEST_F(SeverityColorsTest, LegacyIsTheDefaultAndUnchanged) {
  EXPECT_EQ(GetSeverityTheme(), SeverityTheme::kLegacy);

  EXPECT_EQ(EventRowColorsFor(EventBackground::kUnacknowledged).background,
            C(99, 190, 123));
  EXPECT_EQ(EventRowColorsFor(EventBackground::kCritical).background,
            C(248, 105, 107));
  EXPECT_EQ(EventRowColorsFor(EventBackground::kWarning).background,
            C(255, 235, 132));

  // Legacy leaves the row text colour to the widget.
  EXPECT_FALSE(EventRowColorsFor(EventBackground::kCritical).text.has_value());
}

// Switching the theme re-sources every class from the token ramp at once — this
// is the whole point of the single source.
TEST_F(SeverityColorsTest, TokenThemesResolveFromTheRamp) {
  SetSeverityTheme(SeverityTheme::kDark);
  EXPECT_EQ(GetSeverityTheme(), SeverityTheme::kDark);
  // Critical -> severity-critical (dark), with dark text on the bright fill.
  const EventRowColors dark_critical =
      EventRowColorsFor(EventBackground::kCritical);
  EXPECT_EQ(dark_critical.background, C(232, 90, 82));
  ASSERT_TRUE(dark_critical.text.has_value());
  EXPECT_EQ(*dark_critical.text, C(11, 22, 35));

  SetSeverityTheme(SeverityTheme::kLight);
  // The same class now resolves to the light ramp with light text.
  const EventRowColors light_critical =
      EventRowColorsFor(EventBackground::kCritical);
  EXPECT_EQ(light_critical.background, C(143, 36, 31));
  ASSERT_TRUE(light_critical.text.has_value());
  EXPECT_EQ(*light_critical.text, C(255, 255, 255));

  // A token theme differs from legacy, so enabling it is observable.
  SetSeverityTheme(SeverityTheme::kLegacy);
  EXPECT_NE(EventRowColorsFor(EventBackground::kCritical).background,
            light_critical.background);
}

// The solid severity colour (status text / dots) is absent under legacy and for
// kNone, and follows the ramp under the token themes.
TEST_F(SeverityColorsTest, SolidSeverityColourIsTokenOnly) {
  // Legacy never coloured these cues.
  EXPECT_FALSE(SeverityColor(SeverityLevel::kCritical).has_value());
  EXPECT_FALSE(SeverityColor(SeverityLevel::kWarning).has_value());

  SetSeverityTheme(SeverityTheme::kDark);
  EXPECT_FALSE(SeverityColor(SeverityLevel::kNone).has_value());
  ASSERT_TRUE(SeverityColor(SeverityLevel::kCritical).has_value());
  EXPECT_EQ(*SeverityColor(SeverityLevel::kCritical), C(232, 90, 82));
  ASSERT_TRUE(SeverityColor(SeverityLevel::kWarning).has_value());
  EXPECT_EQ(*SeverityColor(SeverityLevel::kWarning), C(230, 178, 75));
}

// The Explorer status-dot quality colour is absent under legacy and follows the
// good/uncertain/bad tokens under the token themes.
TEST_F(SeverityColorsTest, QualityColourIsTokenOnly) {
  EXPECT_FALSE(QualityColor(Quality::kGood).has_value());
  EXPECT_FALSE(QualityColor(Quality::kBad).has_value());

  SetSeverityTheme(SeverityTheme::kDark);
  ASSERT_TRUE(QualityColor(Quality::kGood).has_value());
  EXPECT_EQ(*QualityColor(Quality::kGood), C(68, 192, 145));
  EXPECT_EQ(*QualityColor(Quality::kUncertain), C(230, 178, 75));
  EXPECT_EQ(*QualityColor(Quality::kBad), C(240, 113, 104));

  SetSeverityTheme(SeverityTheme::kLight);
  EXPECT_EQ(*QualityColor(Quality::kBad), C(197, 61, 53));
}

}  // namespace
}  // namespace scada::aui
