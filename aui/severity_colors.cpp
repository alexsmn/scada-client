#include "aui/severity_colors.h"

namespace scada::aui {

namespace {

// One event-row class under one theme: a background fill and the text colour
// that reads on it.
struct Slot {
  Rgba background;
  Rgba text;
};

struct ThemeTable {
  Slot critical;
  Slot warning;
};

// Values from docs/client/ux/design-language.md — critical maps to
// `severity-critical`, warning to `severity-medium`. The bright dark-theme
// fills carry dark text; the dark light/high-contrast fills carry light text.
constexpr Rgba kDarkText{11, 22, 35};      // #0b1623
constexpr Rgba kLightText{255, 255, 255};  // #ffffff
constexpr Rgba kHcText{0, 0, 0};           // #000000

constexpr ThemeTable kDark{
    .critical = {.background = {232, 90, 82}, .text = kDarkText},
    .warning = {.background = {230, 178, 75}, .text = kDarkText},
};

constexpr ThemeTable kLight{
    .critical = {.background = {143, 36, 31}, .text = kLightText},
    .warning = {.background = {193, 138, 36}, .text = kLightText},
};

constexpr ThemeTable kHighContrast{
    .critical = {.background = {255, 107, 107}, .text = kHcText},
    .warning = {.background = {255, 255, 0}, .text = kHcText},
};

// Solid severity ramp (for text / dots / bars) per token theme: warning maps to
// severity-medium, critical to severity-critical (docs/client/ux/
// design-language.md).
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

// Quality ramp (Explorer status dots) per theme — the good/uncertain/bad
// tokens from docs/client/ux/design-language.md.
struct QualityRamp {
  Rgba good;
  Rgba uncertain;
  Rgba bad;
};

constexpr QualityRamp kDarkQuality{.good = {68, 192, 145},
                                   .uncertain = {230, 178, 75},
                                   .bad = {240, 113, 104}};
constexpr QualityRamp kLightQuality{.good = {20, 130, 95},
                                    .uncertain = {182, 122, 23},
                                    .bad = {197, 61, 53}};
constexpr QualityRamp kHcQuality{.good = {0, 255, 122},
                                 .uncertain = {255, 255, 0},
                                 .bad = {255, 107, 107}};

const QualityRamp& QualityRampFor(SeverityTheme theme) {
  switch (theme) {
    case SeverityTheme::kLight:
      return kLightQuality;
    case SeverityTheme::kHighContrast:
      return kHcQuality;
    case SeverityTheme::kDark:
      break;
  }
  return kDarkQuality;
}

const SolidRamp& SolidRampFor(SeverityTheme theme) {
  switch (theme) {
    case SeverityTheme::kLight:
      return kLightSolid;
    case SeverityTheme::kHighContrast:
      return kHcSolid;
    case SeverityTheme::kDark:
      break;
  }
  return kDarkSolid;
}

// Function-local static (not a namespace-scope global) holds the active theme.
SeverityTheme& CurrentTheme() {
  static SeverityTheme theme = SeverityTheme::kDark;
  return theme;
}

const ThemeTable& TableFor(SeverityTheme theme) {
  switch (theme) {
    case SeverityTheme::kLight:
      return kLight;
    case SeverityTheme::kHighContrast:
      return kHighContrast;
    case SeverityTheme::kDark:
      break;
  }
  return kDark;
}

const Slot& SlotFor(const ThemeTable& table, EventBackground background) {
  switch (background) {
    case EventBackground::kWarning:
      return table.warning;
    case EventBackground::kCritical:
      break;
  }
  return table.critical;
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
  return {.background = Color{slot.background}, .text = Color{slot.text}};
}

std::optional<Color> SeverityColor(SeverityLevel level) {
  if (level == SeverityLevel::kNone)
    return std::nullopt;
  const SolidRamp& ramp = SolidRampFor(CurrentTheme());
  return Color{level == SeverityLevel::kCritical ? ramp.critical
                                                 : ramp.warning};
}

Color QualityColor(Quality quality) {
  const QualityRamp& ramp = QualityRampFor(CurrentTheme());
  switch (quality) {
    case Quality::kBad:
      return Color{ramp.bad};
    case Quality::kUncertain:
      return Color{ramp.uncertain};
    case Quality::kGood:
      break;
  }
  return Color{ramp.good};
}

}  // namespace scada::aui
