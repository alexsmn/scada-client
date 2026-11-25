#include "controller/period_comparison.h"

#include "aui/translation.h"

scada::Duration PeriodComparisonShift(PeriodComparison comparison,
                                      scada::Duration period_length) {
  switch (comparison) {
    case PeriodComparison::kNone:
      return scada::Duration::zero();
    case PeriodComparison::kDayEarlier:
      return std::chrono::days(1);
    case PeriodComparison::kWeekEarlier:
      return std::chrono::weeks(1);
    case PeriodComparison::kPreviousPeriod:
      return period_length;
  }
  return scada::Duration::zero();
}

std::string_view PeriodComparisonName(PeriodComparison comparison) {
  switch (comparison) {
    case PeriodComparison::kNone:
      return {};
    case PeriodComparison::kDayEarlier:
      return "day";
    case PeriodComparison::kWeekEarlier:
      return "week";
    case PeriodComparison::kPreviousPeriod:
      return "previous";
  }
  return {};
}

PeriodComparison ParsePeriodComparison(std::string_view name) {
  for (PeriodComparison comparison :
       {PeriodComparison::kDayEarlier, PeriodComparison::kWeekEarlier,
        PeriodComparison::kPreviousPeriod}) {
    if (name == PeriodComparisonName(comparison))
      return comparison;
  }
  return PeriodComparison::kNone;
}

std::u16string PeriodComparisonSuffix(PeriodComparison comparison) {
  switch (comparison) {
    case PeriodComparison::kNone:
      return {};
    case PeriodComparison::kDayEarlier:
      return u" (" + Translate("a day earlier") + u")";
    case PeriodComparison::kWeekEarlier:
      return u" (" + Translate("a week earlier") + u")";
    case PeriodComparison::kPreviousPeriod:
      return u" (" + Translate("previous period") + u")";
  }
  return {};
}
