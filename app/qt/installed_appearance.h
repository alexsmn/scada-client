#pragma once

#include "aui/qt/theme_qt.h"

#include <QSettings>
#include <QString>

#include <optional>

// Restores the operator's appearance choice at startup and persists a runtime
// change on exit — the theme counterpart of InstalledStyle, and deliberately
// shaped like it.
//
// The experimental UX design-token theming is opt-in and off by default: the
// reshell ships as incremental vertical slices, not a big-bang switchover. When
// enabled, the tokens go on over the platform style (chosen just before by
// InstalledStyle) before the login dialog, so pre-login chrome is themed too.
//
// Settings read here, all under `Ux/`:
//
// - `Experimental` (bool) — the opt-in itself. Settings → Colour scheme is the
//   only UI that writes it.
// - `Theme` (string) — "system" (the default, follows the OS), "dark",
//   "light" or "hc".
// - `StyleSheet` (bool) — false gives palette-only, the direction of travel
//   (backlog P6.2). No UI: an expert escape hatch.
//
// Like InstalledStyle, this only writes back a choice the operator actually
// made at runtime, so a session that changed nothing leaves the stored settings
// exactly as it found them rather than materialising defaults into them.
class InstalledAppearance {
 public:
  explicit InstalledAppearance(QSettings& settings) : settings_{settings} {
    if (settings.value("Ux/Experimental", false).toBool()) {
      const scada::aui::Theme theme = scada::aui::ThemeFromString(
          settings.value("Ux/Theme").toString(), scada::aui::Theme::kSystem);
      const scada::aui::ThemeScope scope =
          settings.value("Ux/StyleSheet", true).toBool()
              ? scada::aui::ThemeScope::kFull
              : scada::aui::ThemeScope::kPaletteOnly;
      // ApplyTheme settles the severity/quality ramp to match; it used to be a
      // second, hand-written mapping here.
      scada::aui::ApplyTheme(theme, scope);
    }
    installed_ = CurrentChoice();
  }

  ~InstalledAppearance() {
    const std::optional<scada::aui::Theme> current = CurrentChoice();
    if (current == installed_) {
      return;
    }
    settings_.setValue("Ux/Experimental", current.has_value());
    // Leave `Ux/Theme` alone when switching off, so turning the reshell back on
    // returns to the appearance the operator had picked rather than the
    // default.
    if (current) {
      settings_.setValue("Ux/Theme", scada::aui::ThemeToString(*current));
    }
  }

 private:
  // The live appearance: nullopt when no theme is installed (the platform
  // look). Read from the theme module rather than from QSettings so it reflects
  // what the operator actually sees — which, once the menu can switch it
  // mid-session, is the only thing worth persisting.
  static std::optional<scada::aui::Theme> CurrentChoice() {
    return scada::aui::IsThemeInstalled()
               ? std::optional<scada::aui::Theme>{scada::aui::ActiveTheme()}
               : std::nullopt;
  }

  QSettings& settings_;
  std::optional<scada::aui::Theme> installed_;
};
