#pragma once

#include "aui/color.h"

#include <optional>

namespace scada::aui {

// The active severity colour theme for event/alarm surfaces. The themes use the
// shared design-system severity ramp (docs/client/ux/design-language.md) and
// mirror the shipped appearances of `aui/qt/theme_qt.h` minus `kSystem`, which
// is resolved to a concrete appearance before it reaches here. Set by
// `ApplyTheme()`, which keeps the process-semantic colours and the chrome they
// sit on in step; defaults to `kDark`.
enum class SeverityTheme { kDark, kLight, kHighContrast };

// Sets / reads the active severity theme. Not thread-safe: set once at startup
// on the UI thread, read on the UI thread while rendering.
void SetSeverityTheme(SeverityTheme theme);
SeverityTheme GetSeverityTheme();

// The event-row background classes. This is the single source of severity/alarm
// colours: every severity surface (event journal today; alarm strip, tree,
// inspector, and status as they gain colouring) resolves its colour here, so
// changing a token restyles them all at once.
//
// Acknowledgement is deliberately absent: it is a workflow state rather than a
// severity, and the journal carries it on the leading dot column, the
// "— pending —" cell and the alarm footer instead of in the row fill.
enum class EventBackground { kCritical, kWarning };

// Background and text colour for an event-row class under the active theme.
struct EventRowColors {
  Color background;
  Color text;
};

// Resolves the colours for an event-row class under the active severity theme.
EventRowColors EventRowColorsFor(EventBackground background);

// A value/event's alarm severity, coarsened to the levels the design ramp
// distinguishes. Used for *solid* severity cues (status text, tree dots,
// inspector marks) — as opposed to the soft `EventBackground` row fills above.
enum class SeverityLevel { kNone, kWarning, kCritical };

// The solid severity colour for a level under the active theme. Returns
// std::nullopt for `kNone` — the absence of a severity, rather than a colour
// the ramp declines to give.
std::optional<Color> SeverityColor(SeverityLevel level);

// A node/value's data quality, for the Explorer status dots and other quality
// cues.
enum class Quality { kGood, kUncertain, kBad };

// The status-dot colour for a data quality under the active theme (the
// good/uncertain/bad tokens).
Color QualityColor(Quality quality);

}  // namespace scada::aui
