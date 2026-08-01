
#include "aui/qt/theme_qt.h"

#include "aui/severity_colors.h"

#include "base/no_destructor.h"

#include <QApplication>
#include <QGuiApplication>
#include <QObject>
#include <QString>
#include <QStyle>
#include <QStyleHints>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace scada::aui {

namespace {

// Builds the three token tables from the exact values in
// docs/client/ux/design-language.md. Semi-transparent tokens (borders, soft
// tints) carry an alpha derived from the documented opacity percentage.
ThemeTokens MakeDarkTokens() {
  ThemeTokens t;
  // Surfaces. Neutral greys, not the blue-tinted charcoals this theme started
  // with: under the native direction the chrome must not carry a hue of its
  // own, because hue is reserved for process semantics (severity, quality,
  // equipment state) — the tokens further down, which stay fixed. The values
  // match the desktop dark appearances the client sits inside and the mockups
  // in docs/product/ui-mockups/screens/.
  t.bg = QColor(0x1e, 0x1e, 0x1e);
  t.bg_elevated = QColor(0x1e, 0x1e, 0x1e);
  t.surface = QColor(0x17, 0x17, 0x17);
  t.surface_muted = QColor(0x29, 0x29, 0x29);
  // The charcoal rail is retired: the activity bar and status strip are
  // ordinary chrome and take the window colour like everything else.
  t.rail_bg = QColor(0x1e, 0x1e, 0x1e);
  t.topbar_bg = QColor(0x1e, 0x1e, 0x1e);
  // Text.
  t.fg = QColor(0xe6, 0xe6, 0xe6);
  t.fg_muted = QColor(0xb4, 0xb4, 0xb4);
  t.fg_subtle = QColor(0x8c, 0x8c, 0x8c);
  t.fg_on_dark = QColor(0xe6, 0xe6, 0xe6);
  // Lines: rgba(255,255,255,.10) / .18.
  t.border = QColor(255, 255, 255, 26);
  t.border_strong = QColor(255, 255, 255, 46);
  // Accent and quality.
  t.accent = QColor(0x77, 0xb4, 0xf3);
  // Text on the accent fill. The accent is a pale blue, so this stays dark —
  // neutral now, matching the window rather than the retired navy.
  t.accent_fg = QColor(0x1e, 0x1e, 0x1e);
  t.accent_soft = QColor(119, 180, 243, 38);  // rgba(...,.15)
  t.good = QColor(0x44, 0xc0, 0x91);
  t.uncertain = QColor(0xe6, 0xb2, 0x4b);
  t.bad = QColor(0xf0, 0x71, 0x68);
  // Severity ramp.
  t.severity_critical = QColor(0xe8, 0x5a, 0x52);
  t.severity_high = QColor(0xf0, 0x71, 0x68);
  t.severity_medium = QColor(0xe6, 0xb2, 0x4b);
  t.severity_low = QColor(0x77, 0xb4, 0xf3);
  // Single-line diagram.
  t.sl_live = QColor(0xe6, 0xb2, 0x4b);
  t.sl_energized = QColor(0x8f, 0xa3, 0xb4);
  t.sl_closed = QColor(0x44, 0xc0, 0x91);
  t.sl_open = QColor(0x8f, 0xa3, 0xb4);
  return t;
}

ThemeTokens MakeLightTokens() {
  ThemeTokens t;
  // Surfaces. Neutral greys for the same reason as the dark table above: the
  // chrome carries no hue of its own, so hue means process semantics and
  // nothing else. These replace the faintly blue whites this theme started
  // with.
  t.bg = QColor(0xec, 0xec, 0xec);
  t.bg_elevated = QColor(0xec, 0xec, 0xec);
  t.surface = QColor(0xff, 0xff, 0xff);
  t.surface_muted = QColor(0xf2, 0xf2, 0xf2);
  // The charcoal rail is retired here too — this used to be a near-black
  // #0d1a27 sitting in a light window, the single most conspicuously
  // non-native thing the client drew.
  t.rail_bg = QColor(0xec, 0xec, 0xec);
  t.topbar_bg = QColor(0xec, 0xec, 0xec);
  // Text.
  t.fg = QColor(0x1d, 0x1d, 0x1f);
  t.fg_muted = QColor(0x4a, 0x4a, 0x4d);
  t.fg_subtle = QColor(0x6e, 0x6e, 0x73);
  // With the rail no longer charcoal, "text on the rail" is just the ordinary
  // foreground; a light value here would now be invisible.
  t.fg_on_dark = QColor(0x1d, 0x1d, 0x1f);
  // Lines: rgba(0,0,0,.12) / .22.
  t.border = QColor(0, 0, 0, 31);
  t.border_strong = QColor(0, 0, 0, 56);
  t.accent = QColor(0x0f, 0x6b, 0xff);
  t.accent_fg = QColor(0xff, 0xff, 0xff);
  t.accent_soft = QColor(15, 107, 255, 26);  // rgba(...,.1)
  t.good = QColor(0x14, 0x82, 0x5f);
  t.uncertain = QColor(0xb6, 0x7a, 0x17);
  t.bad = QColor(0xc5, 0x3d, 0x35);
  t.severity_critical = QColor(0x8f, 0x24, 0x1f);
  t.severity_high = QColor(0xb7, 0x31, 0x2b);
  t.severity_medium = QColor(0xc1, 0x8a, 0x24);
  t.severity_low = QColor(0x23, 0x5f, 0x98);
  t.sl_live = QColor(0xb6, 0x7a, 0x17);
  t.sl_energized = QColor(0x6b, 0x7b, 0x8d);
  t.sl_closed = QColor(0x14, 0x82, 0x5f);
  t.sl_open = QColor(0x8a, 0x99, 0xa8);
  return t;
}

ThemeTokens MakeHighContrastTokens() {
  ThemeTokens t;
  t.bg = QColor(0x00, 0x00, 0x00);
  t.bg_elevated = QColor(0x00, 0x00, 0x00);
  t.surface = QColor(0x00, 0x00, 0x00);
  t.surface_muted = QColor(0x11, 0x11, 0x11);
  t.rail_bg = QColor(0x00, 0x00, 0x00);
  t.topbar_bg = QColor(0x00, 0x00, 0x00);
  t.fg = QColor(0xff, 0xff, 0xff);
  t.fg_muted = QColor(0xff, 0xff, 0xff);
  t.fg_subtle = QColor(0xff, 0xff, 0xff);
  t.fg_on_dark = QColor(0xff, 0xff, 0xff);
  t.border = QColor(0xff, 0xff, 0xff);
  t.border_strong = QColor(0xff, 0xff, 0xff);
  t.accent = QColor(0xff, 0xff, 0x00);
  t.accent_fg = QColor(0x00, 0x00, 0x00);
  t.accent_soft = QColor(0x11, 0x11, 0x11);
  t.good = QColor(0x00, 0xff, 0x7a);
  t.uncertain = QColor(0xff, 0xff, 0x00);
  t.bad = QColor(0xff, 0x6b, 0x6b);
  t.severity_critical = QColor(0xff, 0x6b, 0x6b);
  t.severity_high = QColor(0xff, 0x9f, 0x43);
  t.severity_medium = QColor(0xff, 0xff, 0x00);
  t.severity_low = QColor(0x00, 0xd4, 0xff);
  t.sl_live = QColor(0xff, 0xff, 0x00);
  t.sl_energized = QColor(0xff, 0xff, 0xff);
  t.sl_closed = QColor(0x00, 0xff, 0x7a);
  t.sl_open = QColor(0xff, 0xff, 0xff);
  return t;
}

// Formats a colour for a Qt style sheet. Opaque colours use `#RRGGBB`;
// translucent colours use Qt's `#AARRGGBB` form, which QSS parses natively (its
// rgba() function's alpha handling is version-dependent, so we avoid it).
// The appearance ApplyTheme last installed. Kept so ActiveThemeTokens() can
// hand out the system-derived table when the client is following the OS —
// GetSeverityTheme() cannot express that, since its light/dark values name
// concrete ramps rather than "whatever the desktop is". Trivially destructible,
// so a function-local static is safe here.
Theme& MutableActiveTheme() {
  static Theme theme = Theme::kDark;
  return theme;
}

// Whether ApplyTheme() currently owns the application palette. Distinct from
// MutableActiveTheme(), which keeps its last value so ActiveThemeTokens() has a
// table to hand out either way; this is the flag ClearTheme() clears and the
// Appearance menu reads. Trivially destructible, like the theme above.
bool& MutableThemeInstalled() {
  static bool installed = false;
  return installed;
}

// The scope of the last ApplyTheme(), so a live OS light/dark switch can
// re-apply exactly what the operator asked for rather than assuming kFull.
ThemeScope& MutableActiveScope() {
  static ThemeScope scope = ThemeScope::kFull;
  return scope;
}

// The severity/quality ramp matching a resolved (never kSystem) appearance.
// The ramps name concrete light/dark tables, so this must be handed a resolved
// theme.
SeverityTheme SeverityThemeFor(Theme resolved) {
  switch (resolved) {
    case Theme::kLight:
      return SeverityTheme::kLight;
    case Theme::kHighContrast:
      return SeverityTheme::kHighContrast;
    case Theme::kSystem:
    case Theme::kDark:
      break;
  }
  return SeverityTheme::kDark;
}

// The palette the application is actually painting with right now. Falls back
// to a default-constructed palette before QApplication exists so the token
// accessors stay callable from static initialisation and from tests.
QPalette CurrentPalette() {
  return QApplication::instance() ? QApplication::palette() : QPalette{};
}

// Whether a palette reads as a dark appearance. Used to pick which semantic
// (severity / quality) ramp stays legible against it.
bool IsDarkPalette(const QPalette& palette) {
  return palette.color(QPalette::Window).lightness() < 128;
}

// Linear blend, `t` = 0 gives `a`, 1 gives `b`. Alpha follows the same ramp.
QColor Mix(const QColor& a, const QColor& b, qreal t) {
  return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t,
                          a.greenF() + (b.greenF() - a.greenF()) * t,
                          a.blueF() + (b.blueF() - a.blueF()) * t,
                          a.alphaF() + (b.alphaF() - a.alphaF()) * t);
}

QColor WithAlpha(QColor c, int alpha) {
  c.setAlpha(alpha);
  return c;
}

// Derives the token table from the palette the OS/platform style handed us, so
// the client's chrome is literally the host's colours rather than a look-alike.
//
// Only the *chrome* tokens are derived. The semantic tokens — quality,
// alarm severity, single-line equipment state — are process signals fixed by
// ISA-101 / ISA-18.2 (docs/client/ux/principles.md §9) and must not follow the
// desktop accent colour. They are taken wholesale from the light or dark table,
// chosen by the palette's own lightness so they stay legible against it.
ThemeTokens MakeSystemTokens(const QPalette& p) {
  const bool dark = IsDarkPalette(p);
  // Start from the matching semantic ramp, then overwrite everything the
  // platform owns.
  ThemeTokens t = dark ? MakeDarkTokens() : MakeLightTokens();

  const QColor window = p.color(QPalette::Window);
  const QColor text = p.color(QPalette::WindowText);

  const QColor base = p.color(QPalette::Base);

  t.bg = window;
  t.bg_elevated = window;
  t.surface = base;
  // AlternateBase is meant to be a barely-there stripe next to Base, but not
  // every platform treats it that way — the macOS style reports a mid grey
  // (#8e8e8e) that would light up every table header and inset field on a dark
  // window. Trust it only when it actually sits near Base; otherwise derive the
  // inset shade ourselves by nudging Base toward the text colour.
  const QColor alternate = p.color(QPalette::AlternateBase);
  constexpr int kMaxInsetDistance = 40;
  t.surface_muted =
      std::abs(alternate.lightness() - base.lightness()) <= kMaxInsetDistance
          ? alternate
          : Mix(base, text, 0.08);
  // The charcoal rail/status strip is retired: native chrome takes the window
  // colour like everything else (docs/client/ux/design-language.md).
  t.rail_bg = window;
  t.topbar_bg = window;

  t.fg = text;
  t.fg_muted = Mix(text, window, 0.30);
  t.fg_subtle = Mix(text, window, 0.45);
  t.fg_on_dark = text;

  // Hairlines: the platform's Mid/Dark roles, softened so they read as rules
  // rather than borders on styles that set them strongly.
  t.border = WithAlpha(p.color(QPalette::Mid), 110);
  t.border_strong = p.color(QPalette::Mid);

  // The OS accent — this is the single most recognisable "matches my desktop"
  // cue, and Qt already exposes it as the highlight roles.
  t.accent = p.color(QPalette::Highlight);
  t.accent_fg = p.color(QPalette::HighlightedText);
  t.accent_soft = WithAlpha(p.color(QPalette::Highlight), dark ? 64 : 40);

  return t;
}

// The system table, recomputed whenever the application palette changes. Keyed
// on QPalette::cacheKey() so an OS light/dark switch is picked up on the next
// read without anyone having to invalidate anything.
const ThemeTokens& SystemTokens() {
  static base::NoDestructor<ThemeTokens> tokens;
  static qint64 cached_key = 0;
  const QPalette palette = CurrentPalette();
  if (palette.cacheKey() != cached_key) {
    cached_key = palette.cacheKey();
    *tokens = MakeSystemTokens(palette);
  }
  return *tokens;
}

QString Css(const QColor& c) {
  if (c.alpha() == 255) {
    return QString::asprintf("#%02x%02x%02x", c.red(), c.green(), c.blue());
  }
  return QString::asprintf("#%02x%02x%02x%02x", c.alpha(), c.red(), c.green(),
                           c.blue());
}

}  // namespace

Theme ResolveSystemTheme() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  if (const QStyleHints* hints = QGuiApplication::styleHints()) {
    switch (hints->colorScheme()) {
      case Qt::ColorScheme::Light:
        return Theme::kLight;
      case Qt::ColorScheme::Dark:
        return Theme::kDark;
      case Qt::ColorScheme::Unknown:
        break;
    }
  }
#endif
  // No explicit OS preference (or a Qt build without the API): read it off the
  // palette the platform style actually gave us, which is the thing we are
  // going to match anyway.
  return IsDarkPalette(CurrentPalette()) ? Theme::kDark : Theme::kLight;
}

Theme ResolveTheme(Theme theme) {
  return theme == Theme::kSystem ? ResolveSystemTheme() : theme;
}

const ThemeTokens& GetThemeTokens(Theme theme) {
  static const ThemeTokens kDark = MakeDarkTokens();
  static const ThemeTokens kLight = MakeLightTokens();
  static const ThemeTokens kHighContrast = MakeHighContrastTokens();
  switch (theme) {
    case Theme::kSystem:
      return SystemTokens();
    case Theme::kLight:
      return kLight;
    case Theme::kHighContrast:
      return kHighContrast;
    case Theme::kDark:
      break;
  }
  return kDark;
}

Theme ThemeFromString(const QString& name, Theme fallback) {
  const QString key = name.trimmed().toLower();
  if (key == QStringLiteral("system") || key == QStringLiteral("auto")) {
    return Theme::kSystem;
  }
  if (key == QStringLiteral("dark")) {
    return Theme::kDark;
  }
  if (key == QStringLiteral("light")) {
    return Theme::kLight;
  }
  if (key == QStringLiteral("hc") || key == QStringLiteral("high-contrast") ||
      key == QStringLiteral("highcontrast")) {
    return Theme::kHighContrast;
  }
  return fallback;
}

QString ThemeToString(Theme theme) {
  switch (theme) {
    case Theme::kSystem:
      return QStringLiteral("system");
    case Theme::kLight:
      return QStringLiteral("light");
    case Theme::kHighContrast:
      return QStringLiteral("hc");
    case Theme::kDark:
      break;
  }
  return QStringLiteral("dark");
}

Theme ActiveTheme() {
  return MutableActiveTheme();
}

bool IsThemeInstalled() {
  return MutableThemeInstalled();
}

const ThemeTokens& ActiveThemeTokens() {
  // The reshell is off (legacy severity theme): keep the historical behaviour
  // of handing standalone chrome the dark tokens regardless.
  if (GetSeverityTheme() == SeverityTheme::kLegacy) {
    return GetThemeTokens(Theme::kDark);
  }
  // Otherwise follow whatever ApplyTheme installed. This is what carries the
  // OS colours out to the ~23 call sites that still style themselves from
  // tokens: under Theme::kSystem they resolve against the live palette instead
  // of a baked table, so they track the desktop without waiting for the
  // per-widget stylesheet conversion (backlog P6.4).
  return GetThemeTokens(ActiveTheme());
}

std::optional<QFont> MonoValueFont() {
  if (GetSeverityTheme() == SeverityTheme::kLegacy)
    return std::nullopt;
  QFont font = QApplication::font();
  // The design-language `--font-mono` stack (design-language.md §3), with the
  // common macOS/Linux monospace faces standing in for `ui-monospace`.
  font.setFamilies({QStringLiteral("Cascadia Mono"), QStringLiteral("Consolas"),
                    QStringLiteral("SF Mono"), QStringLiteral("Menlo"),
                    QStringLiteral("DejaVu Sans Mono")});
  font.setStyleHint(QFont::Monospace);
  font.setFixedPitch(true);
  return font;
}

// Composites `c` over `backdrop` and returns the opaque result.
//
// A QPalette entry has to be opaque. The `border` tokens are deliberately
// translucent whites (rgba(255,255,255,.10/.18)) because CSS composites a
// hairline over whatever is behind it; hand the same brush to a QStyle and it
// paints a control frame at whatever alpha over whatever backdrop it happens
// to have, which is how the dark theme ended up with an unchecked checkbox
// indicator at 1.3:1 against its row.
QColor Flatten(const QColor& backdrop, const QColor& c) {
  const qreal a = c.alphaF();
  return QColor::fromRgbF(backdrop.redF() * (1 - a) + c.redF() * a,
                          backdrop.greenF() * (1 - a) + c.greenF() * a,
                          backdrop.blueF() * (1 - a) + c.blueF() * a);
}

// WCAG relative luminance (WCAG 2.2 §Relative luminance).
qreal RelativeLuminance(const QColor& c) {
  const auto channel = [](qreal v) {
    return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
  };
  return 0.2126 * channel(c.redF()) + 0.7152 * channel(c.greenF()) +
         0.0722 * channel(c.blueF());
}

qreal ContrastRatio(const QColor& a, const QColor& b) {
  const qreal la = RelativeLuminance(a);
  const qreal lb = RelativeLuminance(b);
  return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

// The minimum contrast a control frame must reach against the surface behind
// it — WCAG 2.2 SC 1.4.11 Non-text Contrast,
// https://www.w3.org/WAI/WCAG22/Understanding/non-text-contrast.html. An
// unchecked checkbox is the worst case: its whole affordance is the frame.
constexpr qreal kNonTextContrast = 3.0;

// Returns `c` lightened or darkened just far enough to reach `ratio` against
// `against`, moving away from it. Returns `c` unchanged when it already does.
QColor EnsureContrast(const QColor& c, const QColor& against, qreal ratio) {
  if (ContrastRatio(c, against) >= ratio)
    return c;
  // Step towards whichever end of the range is further from `against`, so a
  // dark theme brightens and a light theme darkens.
  const bool lighten = RelativeLuminance(against) < 0.5;
  QColor best = c;
  for (int step = 1; step <= 255; ++step) {
    const int delta = lighten ? step : -step;
    const QColor candidate{std::clamp(c.red() + delta, 0, 255),
                           std::clamp(c.green() + delta, 0, 255),
                           std::clamp(c.blue() + delta, 0, 255)};
    best = candidate;
    if (ContrastRatio(candidate, against) >= ratio)
      break;
  }
  return best;
}

QPalette BuildThemePalette(const ThemeTokens& t) {
  QPalette p;

  // Base surfaces and text.
  p.setColor(QPalette::Window, t.bg);
  p.setColor(QPalette::WindowText, t.fg);
  p.setColor(QPalette::Base, t.surface);
  p.setColor(QPalette::AlternateBase, t.surface_muted);
  p.setColor(QPalette::Text, t.fg);
  p.setColor(QPalette::PlaceholderText, t.fg_subtle);
  p.setColor(QPalette::ToolTipBase, t.surface);
  p.setColor(QPalette::ToolTipText, t.fg);

  // Buttons.
  p.setColor(QPalette::Button, t.surface_muted);
  p.setColor(QPalette::ButtonText, t.fg);
  p.setColor(QPalette::BrightText, t.bad);

  // Selection / links.
  p.setColor(QPalette::Highlight, t.accent);
  p.setColor(QPalette::HighlightedText, t.accent_fg);
  p.setColor(QPalette::Link, t.accent);
  p.setColor(QPalette::LinkVisited, t.accent);

  // Frame shading, derived from the token surfaces so the bevels read as flat
  // hairlines rather than default grey. Flattened against the window because a
  // palette entry must be opaque (see Flatten), and lifted to the non-text
  // contrast floor because these are the roles a style draws control frames
  // from — the unchecked checkbox indicator among them.
  p.setColor(QPalette::Light, Flatten(t.bg, t.surface_muted));
  p.setColor(QPalette::Midlight, Flatten(t.bg, t.surface_muted));
  p.setColor(QPalette::Mid,
             EnsureContrast(Flatten(t.bg, t.border_strong), t.bg,
                            kNonTextContrast));
  p.setColor(QPalette::Dark,
             EnsureContrast(Flatten(t.bg, t.border_strong), t.bg,
                            kNonTextContrast));
  p.setColor(QPalette::Shadow, Flatten(t.bg, t.rail_bg));

  // Disabled group: dim the text/foreground roles.
  p.setColor(QPalette::Disabled, QPalette::WindowText, t.fg_subtle);
  p.setColor(QPalette::Disabled, QPalette::Text, t.fg_subtle);
  p.setColor(QPalette::Disabled, QPalette::ButtonText, t.fg_subtle);
  p.setColor(QPalette::Disabled, QPalette::HighlightedText, t.fg_subtle);
  p.setColor(QPalette::Disabled, QPalette::Highlight, t.surface_muted);

  return p;
}

QString BuildThemeStyleSheet(const ThemeTokens& t) {
  // Everything this sheet used to do is now the platform style's job
  // (docs/client/ux/principles.md §9, backlog P6.2). What remains is only what
  // QPalette has no way to express.
  //
  // Removed, and why — each of these repainted something the native style
  // already draws correctly, and drew it the same way on every OS:
  //
  //   QMenuBar/QMenu      -> Window/Base/Highlight; native menus also bring
  //                          platform-correct popups, shadows and padding.
  //   QToolBar/QToolButton-> Window/ButtonText/Highlight; the style already
  //                          draws hover/pressed/checked affordances.
  //   QDockWidget         -> the old rule also set `titlebar-close-icon:none`,
  //                          which silently removed the close button.
  //   Item views/headers  -> Base/AlternateBase/Text/Highlight (+ Mid for
  //                          gridlines). Tree/Table already re-derive these
  //                          per-widget from the live palette.
  //   QTabWidget/QTabBar  -> the accent-top-marker "editor tab" is a browser
  //                          idiom; native tabs are what desktop users expect.
  //   QStatusBar          -> painted itself charcoal in every theme, the most
  //                          conspicuous non-native cue in the window.
  //   QPushButton         -> Button/ButtonText, and `:default` is a style
  //   state. Inputs/combos       -> Base/Text/Highlight; native focus rings are
  //   also
  //                          the ones the OS accessibility settings affect.
  //   QScrollBar          -> fixed 10px thin bars ignored platform metrics and
  //                          macOS overlay-scrollbar behaviour.
  //
  // Do not re-add any of the above. The bar for a new rule here is that the
  // effect is impossible through QPalette or QStyle::PixelMetric — and if you
  // clear it, say so in a comment like these.
  //
  // Destructive actions are the one surviving case: Qt has no palette role for
  // "this button does something irreversible", so it stays a dynamic property
  // (`button->setProperty("role", "danger")`) resolved here from the semantic
  // token. It is currently unused — the control/write surfaces that need it
  // have not been reshelled yet — but it is the sanctioned pattern rather than
  // dead styling, and it keeps the semantic-token seam honest.
  //
  // The *unchecked* checkbox indicator is the second, and it clears the bar
  // above: no palette role reaches it. Fusion fills the indicator from `Base`
  // and derives its outline from `Window.darker(140)`, which on a dark window
  // darkens to #151515 — measured at 1.08:1 against the row, present but not
  // perceivable, and unreachable by Mid/Dark or any other role. Item views
  // make it worse: SetDefaultItemColors folds Window into Base so a grid
  // matches its chrome, which leaves the indicator fill identical to the row.
  //
  // An unchecked box is the whole affordance for "you may add this signal to
  // the active table" (docs/product/ui-mockups/screens/trend.html draws it as an
  // always-present bordered box), so it has to clear WCAG 2.2 SC 1.4.11's 3:1.
  //
  // Deliberately scoped to `::indicator` and to the *unchecked* state only.
  // The sub-control is the smallest thing that fixes it; leaving `:checked`
  // alone keeps the platform's own tick and accent fill, which are correct
  // already. No width/height either — the indicator keeps the style's
  // PM_IndicatorWidth so it still follows platform metrics and DPI.
  const QColor frame =
      EnsureContrast(Flatten(t.bg, t.border_strong), t.bg, kNonTextContrast);
  return QStringLiteral(
             "QPushButton[role=\"danger\"]{background:%1;color:%2;"
             "border-color:%1;}"
             "QCheckBox::indicator:unchecked,"
             "QAbstractItemView::indicator:unchecked{"
             "border:1px solid %3;border-radius:3px;background:%4;}")
      .arg(Css(t.bad), Css(t.accent_fg), Css(frame), Css(t.surface_muted));
}

namespace {

// Keeps a `kSystem` theme following the desktop for the rest of the session.
//
// Without this the client resolved the OS appearance exactly once, at startup:
// switching macOS to dark left the tokens, the severity ramp and the mono
// numerals on the light tables until the next launch. Connected on the first
// ApplyTheme(), which is necessarily after QApplication exists.
void EnsureSystemThemeWatcher() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  static const base::NoDestructor<QObject> context;
  static bool connected = false;
  if (connected)
    return;
  QStyleHints* hints = QGuiApplication::styleHints();
  if (!hints)
    return;
  connected = true;
  QObject::connect(
      hints, &QStyleHints::colorSchemeChanged, context.get(),
      [](Qt::ColorScheme) {
        // Only while we are actually following the OS — an
        // explicit Dark/Light/High-contrast choice is the
        // operator's and must survive a desktop switch.
        if (MutableThemeInstalled() && MutableActiveTheme() == Theme::kSystem) {
          ApplyTheme(Theme::kSystem, MutableActiveScope());
        }
      });
#endif
}

}  // namespace

void ApplyTheme(Theme theme, ThemeScope scope) {
  MutableActiveTheme() = theme;
  MutableActiveScope() = scope;
  MutableThemeInstalled() = true;
  EnsureSystemThemeWatcher();

  // Deliberately no setStyle() here. The client runs the platform style so it
  // looks native (docs/client/ux/principles.md §9); forcing Fusion was what
  // made it look the same — and equally foreign — on every OS. The style is
  // settled once at startup by InstalledStyle, which also honours an explicit
  // operator override; a theme change must not stomp it.
  if (theme == Theme::kSystem) {
    // Matching the OS means *not* installing a palette of our own: the one the
    // platform style already produced is the desktop's real colour scheme,
    // down to the user's accent colour. Overwriting it with a look-alike table
    // is what made the client merely resemble the host instead of matching it.
    // Restore the style's standard palette in case an explicit theme was
    // applied earlier in this session, then let the tokens follow it.
    if (QStyle* style = QApplication::style()) {
      QApplication::setPalette(style->standardPalette());
    }
  } else {
    QApplication::setPalette(BuildThemePalette(GetThemeTokens(theme)));
  }

  const ThemeTokens& tokens = GetThemeTokens(theme);
  if (auto* app = qApp) {
    // Palette-first: install the global stylesheet only for kFull. Clearing it
    // for kPaletteOnly keeps a live switch from leaving a stale sheet behind.
    app->setStyleSheet(scope == ThemeScope::kFull ? BuildThemeStyleSheet(tokens)
                                                  : QString());
  }

  // Last, so the resolve below reads the palette we just installed: under
  // kSystem, ResolveTheme() falls back to reading the live palette. Keeping the
  // ramp here rather than at the call sites is what stops the chrome and the
  // process-semantic colours drifting apart.
  SetSeverityTheme(SeverityThemeFor(ResolveTheme(theme)));
}

void ClearTheme() {
  // Never touch the palette when we do not own it: with the reshell off, the
  // application palette belongs to the platform style and overwriting it with
  // standardPalette() would discard, for instance, the user's accent colour.
  if (!MutableThemeInstalled())
    return;

  MutableThemeInstalled() = false;
  // Back to the value ActiveThemeTokens() hands out with the reshell off. It
  // reads the dark table under the legacy ramp regardless, but leaving a stale
  // light/high-contrast value here would be a trap for anything added later.
  MutableActiveTheme() = Theme::kDark;
  MutableActiveScope() = ThemeScope::kFull;

  // Sheet first, palette second, and the order is load-bearing: while a global
  // stylesheet is installed, QApplication::style() is Qt's QStyleSheetStyle
  // wrapper rather than the platform style, so standardPalette() would be read
  // off the wrapper. Clearing the sheet unwraps it first.
  if (auto* app = qApp) {
    app->setStyleSheet(QString());
  }
  if (QStyle* style = QApplication::style()) {
    QApplication::setPalette(style->standardPalette());
  }
  // The legacy ramp is what makes the quality dots, severity marks and mono
  // numerals disappear again — they are opt-in parts of the token themes, and
  // several of them test GetSeverityTheme() directly.
  SetSeverityTheme(SeverityTheme::kLegacy);
}

}  // namespace scada::aui
