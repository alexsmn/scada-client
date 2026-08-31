#pragma once

#include "aui/qt/theme_qt.h"

#include <QSettings>
#include <QString>

// Restores the operator's appearance choice at startup and persists a runtime
// change on exit — the theme counterpart of InstalledStyle, and deliberately
// shaped like it.
//
// The tokens go on over the platform style (chosen just before by
// InstalledStyle) before the login dialog, so pre-login chrome is themed too.
// There is no un-themed appearance: an operator who has never chosen one
// follows the host OS light/dark preference (docs/client/ux/principles.md §9).
//
// Settings read here, all under `Ux/`:
//
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
    const scada::aui::Theme theme = scada::aui::ThemeFromString(
        settings.value("Ux/Theme").toString(), scada::aui::Theme::kSystem);
    const scada::aui::ThemeScope scope =
        settings.value("Ux/StyleSheet", true).toBool()
            ? scada::aui::ThemeScope::kFull
            : scada::aui::ThemeScope::kPaletteOnly;
    // ApplyTheme settles the severity/quality ramp to match; it used to be a
    // second, hand-written mapping here.
    scada::aui::ApplyTheme(theme, scope);
    installed_ = scada::aui::ActiveTheme();
  }

  ~InstalledAppearance() {
    // Read from the theme module rather than from QSettings so it reflects what
    // the operator actually sees — which, once Settings → Colour scheme can
    // switch it mid-session, is the only thing worth persisting.
    const scada::aui::Theme current = scada::aui::ActiveTheme();
    if (current == installed_) {
      return;
    }
    settings_.setValue("Ux/Theme", scada::aui::ThemeToString(current));
  }

 private:
  QSettings& settings_;
  scada::aui::Theme installed_ = scada::aui::Theme::kSystem;
};
