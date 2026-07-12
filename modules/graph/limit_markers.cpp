#include "graph/limit_markers.h"

namespace {

// Appends a band to `markers` when it is configured (not the unset sentinel).
void AppendIfSet(std::vector<LimitMarker>& markers,
                 LimitKind kind,
                 double value,
                 double unknown) {
  if (value != unknown)
    markers.push_back(LimitMarker{kind, value});
}

}  // namespace

std::vector<LimitMarker> ComputeLimitMarkers(double lolo,
                                             double lo,
                                             double hi,
                                             double hihi,
                                             double unknown) {
  std::vector<LimitMarker> markers;
  markers.reserve(4);
  // Low → high so overlapping captions stack predictably bottom-to-top.
  AppendIfSet(markers, LimitKind::kLoLo, lolo, unknown);
  AppendIfSet(markers, LimitKind::kLo, lo, unknown);
  AppendIfSet(markers, LimitKind::kHi, hi, unknown);
  AppendIfSet(markers, LimitKind::kHiHi, hihi, unknown);
  return markers;
}

scada::aui::SeverityLevel SeverityOf(LimitKind kind) {
  switch (kind) {
    case LimitKind::kLoLo:
    case LimitKind::kHiHi:
      return scada::aui::SeverityLevel::kCritical;
    case LimitKind::kLo:
    case LimitKind::kHi:
      return scada::aui::SeverityLevel::kWarning;
  }
  return scada::aui::SeverityLevel::kNone;
}

std::string_view LimitBandNameKey(LimitKind kind) {
  switch (kind) {
    case LimitKind::kLoLo:
      return "Alarm low";
    case LimitKind::kLo:
      return "Warning low";
    case LimitKind::kHi:
      return "Warning high";
    case LimitKind::kHiHi:
      return "Alarm high";
  }
  return {};
}
