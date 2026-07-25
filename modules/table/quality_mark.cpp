#include "modules/table/quality_mark.h"

scada::aui::Quality QualityFromQualifier(scada::Qualifier qualifier) {
  // Hard failures where the value is not trustworthy at all: an explicit bad
  // flag, a terminal failure, a broken device connection, or a misconfigured
  // point. These read as bad ("Bad · comms" in the mockup).
  if (qualifier.bad() || qualifier.failed() || qualifier.offline() ||
      qualifier.misconfigured()) {
    return scada::aui::Quality::kBad;
  }

  // A value that is present but stale (too old / not updated) is uncertain,
  // not bad — the operator still sees the last known number, flagged.
  if (qualifier.stale())
    return scada::aui::Quality::kUncertain;

  return scada::aui::Quality::kGood;
}

std::optional<scada::aui::Quality> QualityFromValue(
    const scada::DataValue& value) {
  // Nothing delivered: no value and a zero qualifier. Report no quality at all
  // rather than letting the zero qualifier fall through to Good.
  if (value.is_null())
    return std::nullopt;

  return QualityFromQualifier(value.qualifier);
}
