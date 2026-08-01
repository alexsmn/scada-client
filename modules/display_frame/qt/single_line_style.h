#pragma once

#include <QColor>

namespace scada::aui {
struct ThemeTokens;
}

// Single-line (mimic) diagram equipment-state styling: maps a switching
// device's or conductor's state to the `sl_*` design tokens so the renderer
// colours breakers, disconnectors, busbars and conductors consistently,
// independent of the authored drawing. See
// docs/client/ux/design-language.md §2 "Single-line diagram semantics".
//
// This is the renderer-agnostic client-side half of backlog 2.7: the mapping is
// pure and unit-tested here; feeding the resulting colours into a renderer is
// per-renderer glue (the VDS runtime needs a palette/state ABI — see
// designer/runtime; Modus sets them over its COM interface on Windows).

// Position of a switching device, typically decoded from a double-point
// telesignal.
enum class SwitchState { kClosed, kOpen, kIntermediate, kUnknown };

// Whether a conductor / busbar is energized (topology-derived).
enum class Energization { kEnergized, kDeEnergized };

// Telemetry quality of the driving signal.
enum class SignalQuality { kGood, kUncertain, kBad };

// Symbol shape for a switching device. State is carried as SHAPE as well as
// colour (design-language.md §2: filled square = closed, hollow = open) so it
// stays legible without relying on colour alone.
enum class DeviceShape { kFilled, kHollow, kIndeterminate };

// Colour + shape for a switching-device symbol.
struct SwitchSymbolStyle {
  QColor color;
  DeviceShape shape;

  friend bool operator==(const SwitchSymbolStyle&,
                         const SwitchSymbolStyle&) = default;
};

// Maps a switching device's position + quality to its single-line style:
// closed -> `sl_closed` + filled; open -> `sl_open` + hollow (neutral — an open
// breaker is not an alarm); intermediate / unknown -> `uncertain` + an
// indeterminate shape. Bad quality overrides the colour to the `bad` token and
// forces the indeterminate shape, so stale / invalid telemetry is never drawn
// as a confident state (design-language.md §2: bad quality marks the symbol, it
// does not silently freeze).
SwitchSymbolStyle SwitchStyle(const scada::aui::ThemeTokens& tokens,
                              SwitchState state,
                              SignalQuality quality);

// Maps a conductor / busbar energization + quality to its colour: energized ->
// `sl_live` (restrained amber — never alarm-red); de-energized ->
// `sl_energized` (neutral). Bad quality overrides to the `bad` token.
QColor ConductorColor(const scada::aui::ThemeTokens& tokens,
                      Energization energization,
                      SignalQuality quality);
