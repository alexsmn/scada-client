
#include "aui/qt/theme_qt.h"

#include "aui/severity_colors.h"
#include "aui/test/app_environment.h"

#include <QApplication>
#include <QCheckBox>
#include <QImage>
#include <QPalette>
#include <QStyle>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <optional>
#include <utility>

namespace scada::aui {
namespace {

// The persisted-string form round-trips for every theme, and unknown/empty
// input falls back to the requested default rather than throwing.
TEST(ThemeQtTest, StringRoundTrip) {
  for (Theme theme :
       {Theme::kSystem, Theme::kDark, Theme::kLight, Theme::kHighContrast}) {
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
  // "auto" is accepted alongside "system" for following the OS appearance.
  EXPECT_EQ(ThemeFromString(QStringLiteral("auto")), Theme::kSystem);
  // Following the OS is the default when nothing is persisted.
  EXPECT_EQ(ThemeFromString(QString()), Theme::kSystem);
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
}

// The activity bar and status strip are ordinary chrome: they take the window
// colour, in every theme. `rail_bg` used to be a charcoal-in-every-theme value
// from the browser-styled reshell — a near-black strip down a light window,
// which is exactly what made the client read as a web page. It survives only as
// a vestigial alias until backlog P6.3 removes it, and `fg_on_dark` with it:
// with no dark rail left, rail text is just ordinary text.
TEST(ThemeQtTest, RailIsOrdinaryChromeInEveryTheme) {
  for (const Theme theme :
       {Theme::kDark, Theme::kLight, Theme::kHighContrast}) {
    const ThemeTokens& t = GetThemeTokens(theme);
    EXPECT_EQ(t.rail_bg, t.bg) << "theme " << static_cast<int>(theme);
    EXPECT_EQ(t.fg_on_dark, t.fg) << "theme " << static_cast<int>(theme);
  }
}

// The chrome carries no hue of its own — colour is reserved for process
// semantics (severity, quality, equipment state), which is what lets an
// operator read an alarm tint as meaning something. A tinted window competes
// with that, and it is also the clearest tell that an application is painting
// its own idea of light/dark instead of the desktop's.
//
// The bar is *imperceptible* hue, not arithmetically zero: the shipped greys
// are the platform ones and several carry a 2-5/255 channel spread (`#1d1d1f`,
// `#6e6e73`). The retired blue-tinted values were 20-32 apart (`#07111b` 20,
// `#111827` 22, `#0d1a27` 26, `#152635` 32), so a threshold of 8 separates the
// two cases with room on both sides rather than pinning exact hexes here.
TEST(ThemeQtTest, ChromeSurfacesAndTextAreNeutral) {
  constexpr int kMaxChannelSpread = 8;
  for (const Theme theme :
       {Theme::kDark, Theme::kLight, Theme::kHighContrast}) {
    const ThemeTokens& t = GetThemeTokens(theme);
    for (const QColor& c :
         {t.bg, t.bg_elevated, t.surface, t.surface_muted, t.rail_bg,
          t.topbar_bg, t.fg, t.fg_muted, t.fg_subtle, t.fg_on_dark}) {
      const int spread = std::max({c.red(), c.green(), c.blue()}) -
                         std::min({c.red(), c.green(), c.blue()});
      EXPECT_LE(spread, kMaxChannelSpread)
          << c.name().toStdString() << " in theme " << static_cast<int>(theme);
    }
  }
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

// The stylesheet is generated from the tokens rather than being a static
// string: it carries the theme's own destructive-action colour.
TEST(ThemeQtTest, StyleSheetIsTokenDriven) {
  const ThemeTokens& dark = GetThemeTokens(Theme::kDark);
  const ThemeTokens& light = GetThemeTokens(Theme::kLight);
  const QString sheet = BuildThemeStyleSheet(dark);
  EXPECT_FALSE(sheet.isEmpty());
  EXPECT_TRUE(sheet.contains(dark.bad.name()));
  EXPECT_NE(BuildThemeStyleSheet(light), sheet);
}

// Regression guard for the native look and feel (backlog P6.2): the generated
// sheet must not restyle widgets the platform style already draws. Each of
// these selectors was removed for a reason recorded in theme_qt.cpp — a
// re-added rule would silently make the client look identical, and equally
// foreign, on every OS again.
TEST(ThemeQtTest, StyleSheetDoesNotOverpaintNativeWidgets) {
  for (Theme theme : {Theme::kDark, Theme::kLight, Theme::kHighContrast}) {
    const QString sheet = BuildThemeStyleSheet(GetThemeTokens(theme));
    for (const char* selector :
         {"QMenuBar", "QMenu", "QToolBar", "QToolButton", "QDockWidget",
          "QTreeView", "QTableView", "QListView", "QHeaderView", "QTabWidget",
          "QTabBar", "QStatusBar", "QLineEdit", "QComboBox", "QScrollBar",
          "QMainWindow"}) {
      // `QAbstractItemView::indicator:unchecked` is allowed below; these are
      // the concrete view classes, whose bare selectors would repaint the
      // whole widget.
      EXPECT_FALSE(sheet.contains(QLatin1String(selector)))
          << "theme " << ThemeToString(theme).toStdString()
          << " re-introduced a rule for " << selector;
    }
    // Bare QPushButton styling is gone too; only the role-qualified form
    // survives, because Qt has no palette role for a destructive action.
    EXPECT_FALSE(sheet.contains(QStringLiteral("QPushButton{")));
    EXPECT_TRUE(sheet.contains(QStringLiteral("QPushButton[role=\"danger\"]")));

    // The one sanctioned sub-control exception: the unchecked checkbox
    // indicator, which no palette role reaches (see theme_qt.cpp). It must
    // stay scoped to `::indicator:unchecked` — a bare QCheckBox rule, or one
    // that also claims `:checked`, would take the platform's tick and accent
    // fill with it.
    EXPECT_TRUE(sheet.contains(QStringLiteral("::indicator:unchecked")));
    EXPECT_FALSE(sheet.contains(QStringLiteral("QCheckBox{")));
    EXPECT_FALSE(sheet.contains(QStringLiteral("indicator:checked")));
  }
}

// The monospace value font is part of the opt-in token themes: empty under
// the legacy severity theme, and a fixed-pitch monospace-hinted font — sized
// like the application font — under a token theme.
TEST(ThemeQtTest, MonoValueFontIsTokenThemeGated) {
  AppEnvironment app_env;

  SetSeverityTheme(SeverityTheme::kLegacy);
  EXPECT_FALSE(MonoValueFont().has_value());

  SetSeverityTheme(SeverityTheme::kDark);
  const std::optional<QFont> font = MonoValueFont();
  ASSERT_TRUE(font.has_value());
  EXPECT_TRUE(font->fixedPitch());
  EXPECT_EQ(font->styleHint(), QFont::Monospace);
  EXPECT_EQ(font->pointSize(), QApplication::font().pointSize());

  SetSeverityTheme(SeverityTheme::kLegacy);
}

// ApplyTheme installs the token palette and stylesheet on the running
// application, and can switch themes live.
TEST(ThemeQtTest, ApplyThemeInstallsPaletteAndStyleSheet) {
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
}

// Regression guard for the native look and feel (docs/client/ux/principles.md
// §9): ApplyTheme must recolour without touching the widget style. It used to
// force Fusion unconditionally, which overrode both the platform style — making
// the client look equally foreign on every OS — and any explicit operator
// choice restored by InstalledStyle.
TEST(ThemeQtTest, ApplyThemeDoesNotChangeTheWidgetStyle) {
  AppEnvironment app_env;

  // Stand in for InstalledStyle having settled the style before theming runs.
  // Compare against the base style: while a global stylesheet is installed,
  // qApp->style() is Qt's QStyleSheetStyle wrapper, so read the style back in
  // the kPaletteOnly state where Qt has unwrapped it again.
  ApplyTheme(Theme::kDark, ThemeScope::kPaletteOnly);
  const QString style_before = qApp->style()->objectName();
  ASSERT_FALSE(style_before.isEmpty());

  ApplyTheme(Theme::kLight, ThemeScope::kPaletteOnly);
  EXPECT_EQ(qApp->style()->objectName(), style_before);

  // ...and the palette still followed the theme, so this is not passing merely
  // because ApplyTheme did nothing at all.
  EXPECT_EQ(qApp->palette().color(QPalette::Window),
            GetThemeTokens(Theme::kLight).bg);
}

// `kSystem` resolves to the concrete appearance the host OS asks for, for the
// benefit of callers (like the severity ramp) that need a real light/dark
// answer rather than "whatever the desktop is".
TEST(ThemeQtTest, SystemThemeResolvesToAConcreteAppearance) {
  AppEnvironment app_env;

  const Theme resolved = ResolveSystemTheme();
  EXPECT_TRUE(resolved == Theme::kLight || resolved == Theme::kDark);
  EXPECT_EQ(ResolveTheme(Theme::kSystem), resolved);

  // Explicit choices are returned unchanged — following the OS is the default,
  // not an override of what the operator picked.
  for (Theme theme : {Theme::kDark, Theme::kLight, Theme::kHighContrast}) {
    EXPECT_EQ(ResolveTheme(theme), theme);
  }
}

// The whole point of `kSystem`: chrome tokens are the host desktop's actual
// colours, not a look-alike table. Everything a widget paints its background,
// text, hairlines or selection with must come straight off the live palette.
TEST(ThemeQtTest, SystemTokensComeFromTheLivePalette) {
  AppEnvironment app_env;

  QPalette palette;
  palette.setColor(QPalette::Window, QColor(0x2b, 0x34, 0x41));
  palette.setColor(QPalette::Base, QColor(0x1e, 0x25, 0x30));
  palette.setColor(QPalette::AlternateBase, QColor(0x26, 0x2e, 0x3a));
  palette.setColor(QPalette::WindowText, QColor(0xe8, 0xed, 0xf2));
  palette.setColor(QPalette::Highlight, QColor(0xd0, 0x50, 0x10));
  palette.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
  QApplication::setPalette(palette);

  const ThemeTokens& t = GetThemeTokens(Theme::kSystem);
  EXPECT_EQ(t.bg, palette.color(QPalette::Window));
  EXPECT_EQ(t.surface, palette.color(QPalette::Base));
  EXPECT_EQ(t.surface_muted, palette.color(QPalette::AlternateBase));
  EXPECT_EQ(t.fg, palette.color(QPalette::WindowText));
  // The OS accent, including a user-chosen one that matches no theme of ours.
  EXPECT_EQ(t.accent, palette.color(QPalette::Highlight));
  EXPECT_EQ(t.accent_fg, palette.color(QPalette::HighlightedText));
  // The charcoal rail is retired: chrome takes the window colour like the rest.
  EXPECT_EQ(t.rail_bg, palette.color(QPalette::Window));
  EXPECT_EQ(t.topbar_bg, palette.color(QPalette::Window));

  // Process semantics do NOT follow the desktop (principles.md §9): alarm,
  // quality and single-line colours are safety signals with fixed values.
  const ThemeTokens& dark = GetThemeTokens(Theme::kDark);
  EXPECT_EQ(t.severity_critical, dark.severity_critical);
  EXPECT_EQ(t.bad, dark.bad);
  EXPECT_EQ(t.good, dark.good);
  EXPECT_EQ(t.sl_live, dark.sl_live);
}

// AlternateBase is supposed to be a barely-there stripe beside Base, but the
// macOS style reports a mid grey (#8e8e8e) for it. Taken literally that lights
// up every table header and inset field on a dark window, so an out-of-range
// AlternateBase must be ignored in favour of a shade derived from Base.
TEST(ThemeQtTest, SystemTokensIgnoreAnOutOfRangeAlternateBase) {
  AppEnvironment app_env;

  QPalette hostile;
  hostile.setColor(QPalette::Window, QColor(0x1e, 0x1e, 0x1e));
  hostile.setColor(QPalette::Base, QColor(0x17, 0x17, 0x17));
  hostile.setColor(QPalette::WindowText, QColor(0xff, 0xff, 0xff));
  hostile.setColor(QPalette::AlternateBase, QColor(0x8e, 0x8e, 0x8e));
  QApplication::setPalette(hostile);

  const ThemeTokens& t = GetThemeTokens(Theme::kSystem);
  EXPECT_NE(t.surface_muted, hostile.color(QPalette::AlternateBase));
  // It stays an inset shade: near Base, and on the same side as the text.
  EXPECT_LT(std::abs(t.surface_muted.lightness() -
                     hostile.color(QPalette::Base).lightness()),
            40);

  // A sane AlternateBase is still used verbatim — the guard must not override
  // platforms that report a usable value.
  QPalette sane = hostile;
  sane.setColor(QPalette::AlternateBase, QColor(0x1f, 0x1f, 0x1f));
  QApplication::setPalette(sane);
  EXPECT_EQ(GetThemeTokens(Theme::kSystem).surface_muted,
            sane.color(QPalette::AlternateBase));
}

// An OS appearance change must be picked up without anyone invalidating a
// cache, and must swing the semantic ramp to the legible variant.
TEST(ThemeQtTest, SystemTokensFollowAPaletteChange) {
  AppEnvironment app_env;

  QPalette light;
  light.setColor(QPalette::Window, QColor(0xf4, 0xf6, 0xf8));
  light.setColor(QPalette::WindowText, QColor(0x10, 0x14, 0x18));
  QApplication::setPalette(light);
  EXPECT_EQ(GetThemeTokens(Theme::kSystem).bg, light.color(QPalette::Window));
  EXPECT_EQ(GetThemeTokens(Theme::kSystem).severity_critical,
            GetThemeTokens(Theme::kLight).severity_critical);

  QPalette dark;
  dark.setColor(QPalette::Window, QColor(0x12, 0x16, 0x1b));
  dark.setColor(QPalette::WindowText, QColor(0xf0, 0xf3, 0xf6));
  QApplication::setPalette(dark);
  EXPECT_EQ(GetThemeTokens(Theme::kSystem).bg, dark.color(QPalette::Window));
  EXPECT_EQ(GetThemeTokens(Theme::kSystem).severity_critical,
            GetThemeTokens(Theme::kDark).severity_critical);
}

// Following the OS means leaving its palette alone. Installing a look-alike
// palette of our own is what made the client merely resemble the desktop.
TEST(ThemeQtTest, SystemThemeDoesNotOverwriteThePlatformPalette) {
  AppEnvironment app_env;
  // ActiveThemeTokens() hands out the dark table under the legacy severity
  // theme (reshell off); ApplyTheme moves the ramp off legacy for us, so the
  // token seam under test is the live one.
  ApplyTheme(Theme::kDark, ThemeScope::kPaletteOnly);
  const QColor themed_bg = qApp->palette().color(QPalette::Window);
  EXPECT_EQ(themed_bg, GetThemeTokens(Theme::kDark).bg);

  // Switching to system restores the style's own palette...
  ApplyTheme(Theme::kSystem, ThemeScope::kPaletteOnly);
  ASSERT_NE(qApp->style(), nullptr);
  const QPalette standard = qApp->style()->standardPalette();
  EXPECT_EQ(qApp->palette().color(QPalette::Window),
            standard.color(QPalette::Window));

  // ...and the tokens the widgets read agree with it, so token-styled surfaces
  // match the window they sit in rather than the previous explicit theme.
  EXPECT_EQ(ActiveThemeTokens().bg, qApp->palette().color(QPalette::Window));

  ClearTheme();
}

// ClearTheme is ApplyTheme's inverse: the "Classic" row of Settings →
// Appearance has to leave the application indistinguishable from one that never
// enabled the reshell, or switching off would strand the operator with a
// half-themed client until restart.
TEST(ThemeQtTest, ClearThemeRestoresThePlatformLook) {
  AppEnvironment app_env;
  ASSERT_NE(qApp->style(), nullptr);
  const QPalette pristine = qApp->palette();
  ASSERT_TRUE(qApp->styleSheet().isEmpty());
  ASSERT_FALSE(IsThemeInstalled());

  ApplyTheme(Theme::kHighContrast, ThemeScope::kFull);
  ASSERT_TRUE(IsThemeInstalled());
  ASSERT_NE(qApp->palette().color(QPalette::Window),
            pristine.color(QPalette::Window));
  ASSERT_FALSE(qApp->styleSheet().isEmpty());
  ASSERT_NE(GetSeverityTheme(), SeverityTheme::kLegacy);

  ClearTheme();

  EXPECT_FALSE(IsThemeInstalled());
  EXPECT_EQ(qApp->palette().color(QPalette::Window),
            pristine.color(QPalette::Window));
  EXPECT_TRUE(qApp->styleSheet().isEmpty());
  // The ramp is what makes the quality dots, severity marks and monospace
  // numerals opt-in; leaving it set would keep half the reshell visible.
  EXPECT_EQ(GetSeverityTheme(), SeverityTheme::kLegacy);
  EXPECT_EQ(MonoValueFont(), std::nullopt);
}

// Switching off must not be one-way, and must not degrade: the Appearance menu
// lets an operator flip between Classic and a theme as often as they like.
TEST(ThemeQtTest, ThemeCanBeReappliedAfterClearing) {
  AppEnvironment app_env;

  ApplyTheme(Theme::kDark, ThemeScope::kPaletteOnly);
  const QColor dark_bg = qApp->palette().color(QPalette::Window);
  ClearTheme();
  ApplyTheme(Theme::kDark, ThemeScope::kPaletteOnly);

  EXPECT_TRUE(IsThemeInstalled());
  EXPECT_EQ(ActiveTheme(), Theme::kDark);
  EXPECT_EQ(qApp->palette().color(QPalette::Window), dark_bg);

  ClearTheme();
}

// ClearTheme with nothing installed must not touch the palette: with the
// reshell off the application palette belongs to the platform style, and
// overwriting it with standardPalette() would discard, for instance, the user's
// accent colour.
TEST(ThemeQtTest, ClearThemeIsANoOpWhenNoThemeIsInstalled) {
  AppEnvironment app_env;

  QPalette customized = qApp->palette();
  customized.setColor(QPalette::Window, QColor(0x12, 0x34, 0x56));
  QApplication::setPalette(customized);
  ASSERT_FALSE(IsThemeInstalled());

  ClearTheme();

  EXPECT_EQ(qApp->palette().color(QPalette::Window), QColor(0x12, 0x34, 0x56));
  QApplication::setPalette(qApp->style()->standardPalette());
}

// ApplyTheme owns the severity/quality ramp, so the process-semantic colours
// can never disagree with the chrome they sit on. Three call sites used to
// repeat this mapping by hand.
TEST(ThemeQtTest, ApplyThemeSettlesTheSeverityRamp) {
  AppEnvironment app_env;

  const std::pair<Theme, SeverityTheme> kExpected[] = {
      {Theme::kDark, SeverityTheme::kDark},
      {Theme::kLight, SeverityTheme::kLight},
      {Theme::kHighContrast, SeverityTheme::kHighContrast},
  };
  for (const auto& [theme, severity] : kExpected) {
    ApplyTheme(theme, ThemeScope::kPaletteOnly);
    EXPECT_EQ(GetSeverityTheme(), severity)
        << "theme " << ThemeToString(theme).toStdString();
  }

  // kSystem is resolved first: the ramps name concrete light/dark tables, so
  // "whatever the desktop is" has to become one of them.
  ApplyTheme(Theme::kSystem, ThemeScope::kPaletteOnly);
  EXPECT_EQ(GetSeverityTheme(), ResolveSystemTheme() == Theme::kLight
                                    ? SeverityTheme::kLight
                                    : SeverityTheme::kDark);

  ClearTheme();
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

// WCAG 2.2 SC 1.4.11: a control frame needs 3:1 against the surface behind it.
double RelativeLuminanceOf(const QColor& c) {
  const auto channel = [](double v) {
    return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
  };
  return 0.2126 * channel(c.redF()) + 0.7152 * channel(c.greenF()) +
         0.0722 * channel(c.blueF());
}

double ContrastOf(const QColor& a, const QColor& b) {
  const double la = RelativeLuminanceOf(a);
  const double lb = RelativeLuminanceOf(b);
  return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

// The roles a QStyle draws control frames from. An unchecked checkbox is the
// worst case — the frame is its entire affordance — and the dark theme used to
// render it at 1.3:1, which is present but not perceivable.
TEST(ThemeQtTest, ControlFrameRolesMeetNonTextContrast) {
  for (Theme theme : {Theme::kDark, Theme::kLight, Theme::kHighContrast}) {
    const QPalette p = BuildThemePalette(GetThemeTokens(theme));
    const QColor window = p.color(QPalette::Window);
    for (QPalette::ColorRole role : {QPalette::Mid, QPalette::Dark}) {
      EXPECT_GE(ContrastOf(p.color(role), window), 3.0)
          << "theme " << static_cast<int>(theme) << ", role "
          << static_cast<int>(role) << ": a control frame an operator cannot "
             "see is not a control";
    }
  }
}

// Text on a process-semantic fill (the alarm-flood pill, a quality-coloured
// cell) cannot take its colour from the palette, because the fill does not
// either. It must still be legible on every severity token in every theme — the
// flood pill used to bake in #ffffff, which the light theme's amber and the
// dark theme's medium band do not carry.
TEST(ThemeQtTest, ReadableTextOnSeverityFillsIsLegible) {
  for (Theme theme : {Theme::kDark, Theme::kLight, Theme::kHighContrast}) {
    const ThemeTokens& t = GetThemeTokens(theme);
    for (const QColor& fill :
         {t.severity_critical, t.severity_high, t.severity_medium,
          t.severity_low, t.good, t.uncertain, t.bad}) {
      // WCAG 2.2 SC 1.4.3 at large/bold text: 3:1. The pill is bold and short.
      EXPECT_GE(ContrastOf(ReadableTextOn(fill), fill), 3.0)
          << "theme " << ThemeToString(theme).toStdString() << ", fill "
          << fill.name().toStdString();
    }
  }
}

// A palette entry has to be opaque: the border tokens are translucent whites,
// and a style handed a semi-transparent brush composites it over whatever
// backdrop it happens to have.
TEST(ThemeQtTest, PaletteEntriesAreOpaque) {
  for (Theme theme : {Theme::kDark, Theme::kLight, Theme::kHighContrast}) {
    const QPalette p = BuildThemePalette(GetThemeTokens(theme));
    for (int role = 0; role < QPalette::NColorRoles; ++role) {
      const auto color_role = static_cast<QPalette::ColorRole>(role);
      EXPECT_EQ(p.color(color_role).alpha(), 255)
          << "theme " << static_cast<int>(theme) << ", role " << role;
    }
  }
}


// The rendered check: palette arithmetic is not enough, because the thing that
// was broken — Fusion deriving the indicator outline from Window.darker(140) —
// is invisible to it. Grab a real QCheckBox and measure the pixels.
//
// The unchecked box is the worst case: its frame is the entire affordance, so
// it has to clear WCAG 2.2 SC 1.4.11's 3:1. The checked box is measured too,
// to catch a rule that fixes the empty state by swallowing the platform's tick.
double RenderedContrast(bool checked) {
  QCheckBox box;
  box.setChecked(checked);
  box.resize(60, 24);
  const QImage image =
      box.grab().toImage().convertToFormat(QImage::Format_RGB32);
  std::map<QRgb, int> histogram;
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x)
      ++histogram[image.pixel(x, y)];
  }
  QRgb background = 0;
  int most = 0;
  for (const auto& [color, count] : histogram) {
    if (count > most) {
      most = count;
      background = color;
    }
  }
  double best = 1.0;
  for (const auto& [color, count] : histogram) {
    // Ignore stray antialiasing pixels; a frame covers more than a handful.
    if (color != background && count > 4)
      best = std::max(best, ContrastOf(QColor{color}, QColor{background}));
  }
  return best;
}

TEST(ThemeQtTest, RenderedCheckBoxIndicatorIsVisible) {
  AppEnvironment app_env;
  for (Theme theme : {Theme::kDark, Theme::kLight, Theme::kHighContrast}) {
    ApplyTheme(theme, ThemeScope::kFull);
    EXPECT_GE(RenderedContrast(/*checked=*/false), 3.0)
        << "theme " << ThemeToString(theme).toStdString()
        << ": an unchecked box an operator cannot see is not a control";
    EXPECT_GE(RenderedContrast(/*checked=*/true), 3.0)
        << "theme " << ThemeToString(theme).toStdString()
        << ": the checked state lost its tick";
  }
  ClearTheme();
}

}  // namespace scada::aui
