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

}  // namespace
}  // namespace scada::aui
