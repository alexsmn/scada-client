#include "services/data_quality.h"

#include "aui/translation.h"

scada::aui::Quality DataQualityFor(scada::Qualifier qualifier) {
  if (qualifier.bad() || qualifier.failed() || qualifier.offline() ||
      qualifier.misconfigured()) {
    return scada::aui::Quality::kBad;
  }

  if (qualifier.stale())
    return scada::aui::Quality::kUncertain;

  return scada::aui::Quality::kGood;
}

std::optional<scada::aui::Quality> DataQualityFor(
    const scada::DataValue& value) {
  if (value.is_null())
    return std::nullopt;

  return DataQualityFor(value.qualifier);
}

std::u16string DataQualityLabel(scada::aui::Quality quality) {
  switch (quality) {
    case scada::aui::Quality::kBad:
      return Translate("Bad");
    case scada::aui::Quality::kUncertain:
      return Translate("Uncertain");
    case scada::aui::Quality::kGood:
      break;
  }
  return Translate("Good");
}

std::u16string DataQualityLabel(std::optional<scada::aui::Quality> quality) {
  if (!quality)
    return Translate("No data");
  return DataQualityLabel(*quality);
}
