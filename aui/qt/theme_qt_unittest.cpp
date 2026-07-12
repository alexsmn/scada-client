#include "aui/aui_ns_compat.h"

#include "aui/qt/theme_qt.h"

#include "aui/test/app_environment.h"

#include <QApplication>
#include <QPalette>
#include <QStyle>
#include <gtest/gtest.h>

namespace scada::aui {
namespace {

// The persisted-string form round-trips for every theme, and unknown/empty
// input falls back to the requested default rather than throwing.
TEST(ThemeQtTest, StringRoundTrip) {
  for (Theme theme : {Theme::kDark, Theme::kLight, Theme::kHighContrast}) {
    EXPECT_EQ(ThemeFromString(ThemeToString(theme)), theme);
  }
  EXPECT_EQ(ThemeFromString(QStringLiteral("nonsense"), Theme::kLight),
            Theme::kLight);
  EXPECT_EQ(ThemeFromString(QString(), Theme::kHighContrast),
            Theme::kHighContrast);
  // Accepted aliases for high contrast.
  EXPECT_EQ(ThemeFromString(QStringLiteral("HC")), Theme::kHighContrast);
  EXPECT_EQ(ThemeFromString(QStringLiteral("high-contrast")),
            Theme::kHighContrast);
}

// The three themes are genuinely different surfaces/accents, so switching has a
// visible effect (guards against a copy-paste table that never diverges).
TEST(ThemeQtTest, ThemesAreDistinct) {
  const ThemeTokens& dark = GetThemeTokens(Theme::kDark);
  const ThemeTokens& light = GetThemeTokens(Theme::kLight);
  const ThemeTokens& hc = GetThemeTokens(Theme::kHighContrast);

  EXPECT_NE(dark.bg, light.bg);
  EXPECT_NE(dark.bg, hc.bg);
  EXPECT_NE(light.bg, hc.bg);
  EXPECT_NE(dark.accent, light.accent);

  // The rail stays charcoal (dark) in both the dark and light themes — the
  // shared "deep charcoal rail" signature. It must not track the light bg.
  EXPECT_NE(light.rail_bg, light.bg);
  EXPECT_TRUE(light.rail_bg.lightness() < light.bg.lightness());
}

// The generated palette exposes the accent as the selection highlight, so
// selection colour follows the token rather than Fusion's default blue.
TEST(ThemeQtTest, PaletteUsesAccentForHighlight) {
  const ThemeTokens& dark = GetThemeTokens(Theme::kDark);
  const QPalette palette = BuildThemePalette(dark);
  EXPECT_EQ(palette.color(QPalette::Highlight), dark.accent);
  EXPECT_EQ(palette.color(QPalette::HighlightedText), dark.accent_fg);
  EXPECT_EQ(palette.color(QPalette::Window), dark.bg);
}

// The stylesheet is generated from the tokens: it is non-empty and actually
// contains the theme's accent colour (i.e. it is not a static string).
TEST(ThemeQtTest, StyleSheetIsTokenDriven) {
  const ThemeTokens& dark = GetThemeTokens(Theme::kDark);
  const QString sheet = BuildThemeStyleSheet(dark);
  EXPECT_FALSE(sheet.isEmpty());
  EXPECT_TRUE(sheet.contains(dark.accent.name()));
  EXPECT_TRUE(sheet.contains(QStringLiteral("QHeaderView")));
}

// ApplyTheme installs the Fusion style plus the token palette and stylesheet on
// the running application, and can switch themes live.
TEST(ThemeQtTest, ApplyThemeInstallsPaletteAndStyle) {
  AppEnvironment app_env;

  ApplyTheme(Theme::kLight);
  ASSERT_NE(qApp->style(), nullptr);
  EXPECT_EQ(qApp->palette().color(QPalette::Highlight),
            GetThemeTokens(Theme::kLight).accent);
  EXPECT_FALSE(qApp->styleSheet().isEmpty());

  // Switching to another theme takes effect on the live application.
  ApplyTheme(Theme::kDark);
  EXPECT_EQ(qApp->palette().color(QPalette::Window),
            GetThemeTokens(Theme::kDark).bg);

  // While a global stylesheet is installed (kFull), qApp->style() is Qt's
  // QStyleSheetStyle wrapper whose objectName() is empty, so Fusion cannot be
  // identified through it. Switching to kPaletteOnly clears the stylesheet,
  // Qt unwraps back to the base style, and the Fusion install made by the
  // same ApplyTheme code path becomes directly observable.
  ApplyTheme(Theme::kDark, ThemeScope::kPaletteOnly);
  EXPECT_TRUE(
      qApp->style()->objectName().toLower().contains(QStringLiteral("fusion")));
}

// Palette-first: kPaletteOnly recolours through the palette but installs no
// global stylesheet (safest for ActiveX/embedded and custom-painted widgets).
TEST(ThemeQtTest, ApplyThemePaletteOnlyInstallsNoStyleSheet) {
  AppEnvironment app_env;

  ApplyTheme(Theme::kDark, ThemeScope::kPaletteOnly);
  EXPECT_EQ(qApp->palette().color(QPalette::Window),
            GetThemeTokens(Theme::kDark).bg);
  EXPECT_TRUE(qApp->styleSheet().isEmpty());
}

}  // namespace
}  // namespace scada::aui
