#pragma once

#include "aui/severity_colors.h"

#include <string_view>
#include <vector>

// The four analog-limit bands an AnalogItem can carry (AnalogItemType_Limit*),
// ordered low → high. The outer bands (LoLo/HiHi) are alarm thresholds; the
// inner bands (Lo/Hi) are warnings. Drawn as horizontal markers on the trend
// pane — see the reshell mockup docs/product/ui-mockups/screens/trend.html.
enum class LimitKind { kLoLo, kLo, kHi, kHiHi };

// A single configured limit selected for display.
struct LimitMarker {
  LimitKind kind;
  double value;
};

// Returns the configured limits — those not equal to `unknown` — as display
// markers ordered low → high. `unknown` is the sentinel MetrixDataSource stores
// for an unset limit (kGraphUnknownValue); it is passed in so this stays pure
// and free of any Qt/graph dependency for direct unit testing. A limit whose
// value is `unknown` is omitted.
std::vector<LimitMarker> ComputeLimitMarkers(double lolo,
                                             double lo,
                                             double hi,
                                             double hihi,
                                             double unknown);

// The design-ramp severity for a band, used to colour the marker under the
// opt-in severity theme: the outer bands (LoLo/HiHi) resolve to Critical
// (alarm), the inner bands (Lo/Hi) to Warning.
scada::aui::SeverityLevel SeverityOf(LimitKind kind);

// A short English caption key for a band ("Alarm high", "Warning low", …),
// suitable to pass through Translate() at the call site. Never Cyrillic in
// source — the Russian text lives in the translation catalog.
std::string_view LimitBandNameKey(LimitKind kind);
