#pragma once

#include "aui/color.h"

#include <optional>

namespace scada::aui {

// The active severity colour theme for event/alarm surfaces. `kLegacy` keeps
// the historical hardcoded colours so the default UI is unchanged; the token
// themes use the shared design-system severity ramp
// (client/docs/ux/design-language.md). Set once at startup from the same opt-in
// as the palette theme
// (`app/qt/main.cpp`); defaults to `kLegacy`.
enum class SeverityTheme { kLegacy, kDark, kLight, kHighContrast };

// Sets / reads the active severity theme. Not thread-safe: set once at startup
// on the UI thread, read on the UI thread while rendering.
void SetSeverityTheme(SeverityTheme theme);
SeverityTheme GetSeverityTheme();

// The event-row background classes. This is the single source of severity/alarm
// colours: every severity surface (event journal today; alarm strip, tree,
// inspector, and status as they gain colouring) resolves its colour here, so
// changing a token restyles them all at once.
enum class EventBackground { kUnacknowledged, kCritical, kWarning };

// Background — and, for the token themes, text — colour for an event-row class
// under the active theme. `text` is unset for `kLegacy`, which left the row's
// text colour untouched.
struct EventRowColors {
  Color background;
  std::optional<Color> text;
};

// Resolves the colours for an event-row class under the active severity theme.
EventRowColors EventRowColorsFor(EventBackground background);

// A value/event's alarm severity, coarsened to the levels the design ramp
// distinguishes. Used for *solid* severity cues (status text, tree dots,
// inspector marks) — as opposed to the soft `EventBackground` row fills above.
enum class SeverityLevel { kNone, kWarning, kCritical };

// The solid severity colour for a level under the active theme. Returns
// std::nullopt for `kNone`, and — because the legacy UI never coloured these
// cues — for the legacy theme too, so the default look is unchanged and the
// colour appears only under the opt-in token themes.
std::optional<Color> SeverityColor(SeverityLevel level);

// A node/value's data quality, for the Explorer status dots and other quality
// cues.
enum class Quality { kGood, kUncertain, kBad };

// The status-dot colour for a data quality under the active theme (the
// good/uncertain/bad tokens). Returns std::nullopt under the legacy theme — the
// dots are part of the opt-in token themes — so the default tree is unchanged.
std::optional<Color> QualityColor(Quality quality);

}  // namespace scada::aui
