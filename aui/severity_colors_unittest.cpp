#include "aui/severity_colors.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

namespace scada::aui {
namespace {

// Builds a colour without exposing commas to the EXPECT_* macros.
Color C(int r, int g, int b) {
  return Color{Rgba{static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g),
                    static_cast<std::uint8_t>(b)}};
}

// WCAG 2.x relative luminance of an sRGB colour,
// https://www.w3.org/TR/WCAG22/#dfn-relative-luminance
double RelativeLuminance(Color color) {
  const Rgba rgba = color.rgba();
  auto linear = [](std::uint8_t channel) {
    const double c = channel / 255.0;
    return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
  };
  return 0.2126 * linear(rgba.r) + 0.7152 * linear(rgba.g) +
         0.0722 * linear(rgba.b);
}

// WCAG 2.x contrast ratio, https://www.w3.org/TR/WCAG22/#dfn-contrast-ratio
double ContrastRatio(Color a, Color b) {
  const double la = RelativeLuminance(a);
  const double lb = RelativeLuminance(b);
  return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
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

// Row text must stay legible on its severity fill in every theme: WCAG 2.x
// SC 1.4.3 Contrast (Minimum) asks 4.5:1 for normal text,
// https://www.w3.org/TR/WCAG22/#contrast-minimum. The light warning row once
// carried white text on its amber fill at 3.0:1 (backlog 1123).
TEST_F(SeverityColorsTest, RowTextMeetsWcagAaOnItsFill) {
  for (const SeverityTheme theme : {SeverityTheme::kDark, SeverityTheme::kLight,
                                    SeverityTheme::kHighContrast}) {
    SetSeverityTheme(theme);
    for (const EventBackground background :
         {EventBackground::kCritical, EventBackground::kWarning}) {
      const EventRowColors colors = EventRowColorsFor(background);
      EXPECT_GE(ContrastRatio(colors.text, colors.background), 4.5)
          << "theme " << static_cast<int>(theme) << ", background "
          << static_cast<int>(background);
    }
  }
}

// Backlog 1144: severity *text* on the window must clear WCAG 2.2 SC 1.4.3's
// 4.5:1 (https://www.w3.org/TR/WCAG22/#contrast-minimum) in every scheme. The
// light solid amber (#c18a24) is 3.0:1 on white, so text takes its own token.
// The backgrounds are each scheme's `--bg` and `--surface`
// (docs/client/ux/design-language.md §2, `aui/qt/theme_qt.cpp`).
TEST_F(SeverityColorsTest, SeverityTextMeetsWcagAaOnTheWindow) {
  struct Scheme {
    SeverityTheme theme;
    Color window;
    Color surface;
  };
  const Scheme schemes[] = {
      {SeverityTheme::kDark, C(0x1e, 0x1e, 0x1e), C(0x17, 0x17, 0x17)},
      {SeverityTheme::kLight, C(0xec, 0xec, 0xec), C(0xff, 0xff, 0xff)},
      {SeverityTheme::kHighContrast, C(0, 0, 0), C(0, 0, 0)},
  };
  for (const Scheme& scheme : schemes) {
    SetSeverityTheme(scheme.theme);
    for (const SeverityLevel level :
         {SeverityLevel::kWarning, SeverityLevel::kCritical}) {
      const std::optional<Color> text = SeverityTextColor(level);
      ASSERT_TRUE(text.has_value());
      EXPECT_GE(ContrastRatio(*text, scheme.window), 4.5)
          << "theme " << static_cast<int>(scheme.theme) << ", level "
          << static_cast<int>(level);
      EXPECT_GE(ContrastRatio(*text, scheme.surface), 4.5)
          << "theme " << static_cast<int>(scheme.theme) << ", level "
          << static_cast<int>(level);
    }
  }
  EXPECT_EQ(SeverityTextColor(SeverityLevel::kNone), std::nullopt);
}

// The text token departs from the fill only where it must: the light amber
// fill stays #c18a24 (its row text is near-black, 6.2:1), while its text form
// is #8a5f0f.
TEST_F(SeverityColorsTest, LightWarningTextIsDarkerThanItsFill) {
  SetSeverityTheme(SeverityTheme::kLight);
  EXPECT_EQ(SeverityColor(SeverityLevel::kWarning), C(193, 138, 36));
  EXPECT_EQ(SeverityTextColor(SeverityLevel::kWarning), C(138, 95, 15));
  SetSeverityTheme(SeverityTheme::kDark);
  EXPECT_EQ(SeverityTextColor(SeverityLevel::kWarning),
            SeverityColor(SeverityLevel::kWarning));
}

}  // namespace
}  // namespace scada::aui
