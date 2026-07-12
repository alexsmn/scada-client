#include "display_frame/qt/single_line_style.h"

#include "aui/qt/theme_qt.h"

SwitchSymbolStyle SwitchStyle(const scada::aui::ThemeTokens& tokens,
                              SwitchState state,
                              SignalQuality quality) {
  // Bad quality is never shown as a confident open/closed state.
  if (quality == SignalQuality::kBad)
    return {tokens.bad, DeviceShape::kIndeterminate};

  switch (state) {
    case SwitchState::kClosed:
      return {tokens.sl_closed, DeviceShape::kFilled};
    case SwitchState::kOpen:
      return {tokens.sl_open, DeviceShape::kHollow};
    case SwitchState::kIntermediate:
    case SwitchState::kUnknown:
      break;
  }
  return {tokens.uncertain, DeviceShape::kIndeterminate};
}

QColor ConductorColor(const scada::aui::ThemeTokens& tokens,
                      Energization energization,
                      SignalQuality quality) {
  if (quality == SignalQuality::kBad)
    return tokens.bad;

  return energization == Energization::kEnergized ? tokens.sl_live
                                                  : tokens.sl_energized;
}
