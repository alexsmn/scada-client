#pragma once

#include "aui/aui_ns_compat.h"

#include <QColor>
#include <QPalette>
#include <QString>

namespace scada::aui {

// The shipped application themes. Dark is the desktop default (control-room
// norm; low glare at night); Light mirrors the web client default; HighContrast
// is the accessibility / bright-ambient fallback. See the UX design system at
// client/docs/ux/design-language.md.
enum class Theme { kDark, kLight, kHighContrast };

// The full semantic colour-token set for one theme. The values are kept
// numerically identical to the web design system (web/ds-bundle) and to
// client/docs/ux/design-language.md so the desktop and web clients read as one
// product. Components must consume these tokens (via the palette or the
// generated stylesheet) rather than hard-coding colours.
struct ThemeTokens {
  // Surfaces.
  QColor bg;             // app background
  QColor bg_elevated;    // sidebars, tab bar, dialog footers
  QColor surface;        // panels, cards, dialogs, input views
  QColor surface_muted;  // table headers, inset fields, buttons
  QColor rail_bg;        // activity bar & status strip (charcoal in all themes)
  QColor topbar_bg;      // top context bar / menu bar

  // Text.
  QColor fg;          // primary text
  QColor fg_muted;    // secondary text
  QColor fg_subtle;   // labels, captions, placeholders
  QColor fg_on_dark;  // text that always sits on the charcoal rail

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

// Returns the immutable token table for a theme.
const ThemeTokens& GetThemeTokens(Theme theme);

// Parses a theme from its persisted QSettings string ("dark" | "light" | "hc"),
// falling back to `fallback` for unknown/empty input.
Theme ThemeFromString(const QString& name, Theme fallback = Theme::kDark);

// Formats a theme as its persisted QSettings string.
QString ThemeToString(Theme theme);

// Builds a Fusion-compatible QPalette from the theme tokens. Fusion honours the
// palette uniformly across platforms, which native styles do not.
QPalette BuildThemePalette(const ThemeTokens& tokens);

// Builds the application QSS stylesheet from the theme tokens. The sheet styles
// the shared chrome vocabulary (menu/tool bars, docks, tables, trees, tabs,
// status bar, buttons, fields, scrollbars) so the whole app reads as one
// system. Severity/quality accents are exposed through dynamic widget
// properties (e.g. a QPushButton with `role` == "danger").
QString BuildThemeStyleSheet(const ThemeTokens& tokens);

// How much of the theme to install. Palette-first: `kPaletteOnly` recolours the
// app through the QPalette alone (safest with ActiveX/embedded and
// custom-painted widgets), while `kFull` also installs the generated global
// stylesheet for the full workbench chrome. Prefer extending the palette and
// targeted per-widget styling over growing the global sheet.
enum class ThemeScope { kPaletteOnly, kFull };

// Applies a theme to the whole application: forces the Fusion style and installs
// the palette, plus the generated stylesheet when `scope` is `kFull`. Safe to
// call at runtime to switch themes live. Must run after a QApplication exists.
//
// This is opt-in: nothing calls it unless the operator enables the experimental
// UX (see app/qt/main.cpp). The legacy Fusion look is unchanged by default.
void ApplyTheme(Theme theme, ThemeScope scope = ThemeScope::kFull);

}  // namespace scada::aui
