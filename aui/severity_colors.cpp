#include "aui/severity_colors.h"

namespace scada::aui {

namespace {

// One event-row class under one theme: a background fill and an optional text
// colour (`has_fg` == false keeps the widget's default text colour, as the
// legacy look did).
struct Slot {
  Rgba background;
  Rgba text;
  bool has_text;
};

struct ThemeTable {
  Slot unacknowledged;
  Slot critical;
  Slot warning;
};

// Legacy: the exact historical event-row colours (see the pre-token
// GetEventColors). Kept so the default, opt-in-off UI is pixel-unchanged.
constexpr ThemeTable kLegacy{
    .unacknowledged = {.background = {99, 190, 123},
                       .text = {},
                       .has_text = false},
    .critical = {.background = {248, 105, 107}, .text = {}, .has_text = false},
    .warning = {.background = {255, 235, 132}, .text = {}, .has_text = false},
};

// Token themes: values from client/docs/ux/design-language.md — unacknowledged
// maps to the `good` token, critical to `severity-critical`, warning to
// `severity-medium`. The bright dark-theme fills carry dark text; the dark
// light/high-contrast fills carry light text.
constexpr Rgba kDarkText{11, 22, 35};      // #0b1623
constexpr Rgba kLightText{255, 255, 255};  // #ffffff
constexpr Rgba kHcText{0, 0, 0};           // #000000

constexpr ThemeTable kDark{
    .unacknowledged = {.background = {68, 192, 145},
                       .text = kDarkText,
                       .has_text = true},
    .critical = {.background = {232, 90, 82},
                 .text = kDarkText,
                 .has_text = true},
    .warning = {.background = {230, 178, 75},
                .text = kDarkText,
                .has_text = true},
};

constexpr ThemeTable kLight{
    .unacknowledged = {.background = {20, 130, 95},
                       .text = kLightText,
                       .has_text = true},
    .critical = {.background = {143, 36, 31},
                 .text = kLightText,
                 .has_text = true},
    .warning = {.background = {193, 138, 36},
                .text = kLightText,
                .has_text = true},
};

constexpr ThemeTable kHighContrast{
    .unacknowledged = {.background = {0, 255, 122},
                       .text = kHcText,
                       .has_text = true},
    .critical = {.background = {255, 107, 107},
                 .text = kHcText,
                 .has_text = true},
    .warning = {.background = {255, 255, 0}, .text = kHcText, .has_text = true},
};

// Solid severity ramp (for text / dots / bars) per token theme: warning maps to
// severity-medium, critical to severity-critical (client/docs/ux/
// design-language.md). Legacy has no solid ramp — those cues were never
// coloured — so SeverityColor returns nothing there.
struct SolidRamp {
  Rgba warning;
  Rgba critical;
};

constexpr SolidRamp kDarkSolid{.warning = {230, 178, 75},
                               .critical = {232, 90, 82}};
constexpr SolidRamp kLightSolid{.warning = {193, 138, 36},
                                .critical = {143, 36, 31}};
constexpr SolidRamp kHcSolid{.warning = {255, 255, 0},
                             .critical = {255, 107, 107}};

const SolidRamp* SolidRampFor(SeverityTheme theme) {
  switch (theme) {
    case SeverityTheme::kDark:
      return &kDarkSolid;
    case SeverityTheme::kLight:
      return &kLightSolid;
    case SeverityTheme::kHighContrast:
      return &kHcSolid;
    case SeverityTheme::kLegacy:
      break;
  }
  return nullptr;
}

// Function-local static (not a namespace-scope global) holds the active theme.
SeverityTheme& CurrentTheme() {
  static SeverityTheme theme = SeverityTheme::kLegacy;
  return theme;
}

const ThemeTable& TableFor(SeverityTheme theme) {
  switch (theme) {
    case SeverityTheme::kDark:
      return kDark;
    case SeverityTheme::kLight:
      return kLight;
    case SeverityTheme::kHighContrast:
      return kHighContrast;
    case SeverityTheme::kLegacy:
      break;
  }
  return kLegacy;
}

const Slot& SlotFor(const ThemeTable& table, EventBackground background) {
  switch (background) {
    case EventBackground::kCritical:
      return table.critical;
    case EventBackground::kWarning:
      return table.warning;
    case EventBackground::kUnacknowledged:
      break;
  }
  return table.unacknowledged;
}

}  // namespace

void SetSeverityTheme(SeverityTheme theme) {
  CurrentTheme() = theme;
}

SeverityTheme GetSeverityTheme() {
  return CurrentTheme();
}

EventRowColors EventRowColorsFor(EventBackground background) {
  const Slot& slot = SlotFor(TableFor(CurrentTheme()), background);
  EventRowColors colors{.background = Color{slot.background}};
  if (slot.has_text)
    colors.text = Color{slot.text};
  return colors;
}

std::optional<Color> SeverityColor(SeverityLevel level) {
  if (level == SeverityLevel::kNone)
    return std::nullopt;
  const SolidRamp* ramp = SolidRampFor(CurrentTheme());
  if (!ramp)
    return std::nullopt;  // legacy: severity cues are not coloured
  return Color{level == SeverityLevel::kCritical ? ramp->critical
                                                 : ramp->warning};
}

}  // namespace scada::aui
