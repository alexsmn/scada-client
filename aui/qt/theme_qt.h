#pragma once

#include <QColor>
#include <QFont>
#include <QPalette>
#include <QString>

#include <optional>

namespace scada::aui {

// The shipped application appearances.
//
// `kSystem` is the default: the client is a native desktop application and
// follows the host OS light/dark preference (docs/client/ux/principles.md §9).
// The other three are explicit operator overrides for sites that standardise on
// one appearance — Dark remains the recommended control-room setting, but it is
// no longer forced. See docs/client/ux/design-language.md §1.
//
// `kSystem` never reaches a token table directly; resolve it first with
// ResolveTheme() / ResolveSystemTheme().
enum class Theme { kSystem, kDark, kLight, kHighContrast };

// The full semantic colour-token set for one theme. The values are kept
// numerically identical to the web design system (web/ds-bundle) and to
// docs/client/ux/design-language.md so the desktop and web clients read as one
// product. Components must consume these tokens (via the palette or the
// generated stylesheet) rather than hard-coding colours.
struct ThemeTokens {
  // Surfaces.
  QColor bg;             // app background
  QColor bg_elevated;    // sidebars, tab bar, dialog footers
  QColor surface;        // panels, cards, dialogs, input views
  QColor surface_muted;  // table headers, inset fields, buttons
  // Vestigial. The charcoal rail is retired: the activity bar and status strip
  // are ordinary chrome, so this equals `bg` in every theme and in the
  // system-derived table. Kept only so its consumers need not change ahead of
  // backlog P6.3, which removes it; do not reintroduce a distinct value.
  QColor rail_bg;    // activity bar & status strip
  QColor topbar_bg;  // top context bar / menu bar

  // Text.
  QColor fg;         // primary text
  QColor fg_muted;   // secondary text
  QColor fg_subtle;  // labels, captions, placeholders
  // Vestigial, like `rail_bg`: with no charcoal rail left to sit on, this is
  // just `fg`. Retired together with `rail_bg`.
  QColor fg_on_dark;

  // Lines.
  QColor border;         // hairlines
  QColor border_strong;  // field / control borders

  // Accent and quality.
  QColor accent;       // active / navigation / primary action / focus
  QColor accent_fg;    // text on a solid accent fill
  QColor accent_soft;  // selection fill, active tint
  QColor good;         // healthy / in-service / good quality
  QColor uncertain;    // uncertain / stale quality
  QColor bad;          // bad quality / comms loss / destructive

  // Alarm severity ramp (one ramp, used identically on every surface).
  QColor severity_critical;
  QColor severity_high;
  QColor severity_medium;
  QColor severity_low;

  // Single-line diagram (schematic display) equipment-state semantics.
  QColor sl_live;       // energized primary conductor / live busbar
  QColor sl_energized;  // de-energized / idle conductor (neutral)
  QColor sl_closed;     // switching device closed / in service
  QColor sl_open;       // switching device open (neutral, not an alarm)
};

// Returns the immutable token table for a theme. `kSystem` resolves through
// ResolveSystemTheme() first.
const ThemeTokens& GetThemeTokens(Theme theme);

// The concrete appearance the host OS is currently asking for: kLight or kDark.
// Reads QStyleHints::colorScheme(); falls back to kDark where the platform
// reports no preference or the Qt build predates the API. The OS
// high-contrast state is not exposed portably by Qt, so kHighContrast is only
// ever reached by an explicit operator choice.
Theme ResolveSystemTheme();

// Maps `kSystem` onto the concrete appearance the OS asks for and returns every
// other theme unchanged. Call this before anything that needs a real palette.
Theme ResolveTheme(Theme theme);

// The token table matching the active severity theme (the reshell opt-in
// state set at startup): kLight/kHighContrast map to their tables, everything
// else — including the legacy theme, for standalone reshell chrome that
// renders the dark tokens regardless — maps to dark.
const ThemeTokens& ActiveThemeTokens();

// Parses a theme from its persisted QSettings string
// ("system" | "dark" | "light" | "hc"), falling back to `fallback` for
// unknown/empty input.
Theme ThemeFromString(const QString& name, Theme fallback = Theme::kSystem);

// Formats a theme as its persisted QSettings string.
QString ThemeToString(Theme theme);

// The monospace font for values, NodeIds, timestamps, and measurements — the
// design-system `--font-mono` stack (Cascadia Mono → Consolas → ui-monospace;
// docs/client/ux/design-language.md §3), sized like the application font so it
// sits inline with UI text. Returns std::nullopt under the legacy severity
// theme so the default look is unchanged: monospace numerals are part of the
// opt-in token themes. Requires a QApplication (reads the application font).
std::optional<QFont> MonoValueFont();

// Builds a QPalette from the theme tokens. This is the primary, and preferred,
// way the client colours itself: native styles honour the palette for most
// roles, and it is the only theming mechanism that survives under the platform
// style. Prefer extending this over adding stylesheet rules.
QPalette BuildThemePalette(const ThemeTokens& tokens);

// Builds the application QSS stylesheet from the theme tokens.
//
// Deliberately almost empty. Everything a native style can draw is left to the
// native style (docs/client/ux/principles.md §9); this sheet now carries only
// what QPalette cannot express — currently just the destructive-action role
// (`widget->setProperty("role", "danger")`).
//
// Do not add rules here. Express colour through BuildThemePalette() and size
// through QStyle::PixelMetric/QFontMetrics. A rule earns its place only by
// being impossible through both, and the implementation lists what was removed
// so it does not creep back.
QString BuildThemeStyleSheet(const ThemeTokens& tokens);

// How much of the theme to install. Palette-first: `kPaletteOnly` recolours the
// app through the QPalette alone (safest with ActiveX/embedded and
// custom-painted widgets), while `kFull` also installs the generated global
// stylesheet for the full workbench chrome. Prefer extending the palette and
// targeted per-widget styling over growing the global sheet.
enum class ThemeScope { kPaletteOnly, kFull };

// Applies a theme to the whole application: installs the palette, plus the
// generated stylesheet when `scope` is `kFull`. Safe to call at runtime to
// switch themes live. Must run after a QApplication exists.
//
// Deliberately does NOT change the widget style. The client runs the platform
// style so it looks native on each OS (docs/client/ux/principles.md §9); the
// style is chosen once at startup by InstalledStyle, and an operator override
// must not be silently discarded by a theme change.
//
// This is opt-in: nothing calls it unless the operator enables the experimental
// UX (see app/qt/main.cpp).
void ApplyTheme(Theme theme, ThemeScope scope = ThemeScope::kFull);

}  // namespace scada::aui
