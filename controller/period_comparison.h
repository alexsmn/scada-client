#pragma once

#include "scada/date_time.h"

#include <optional>
#include <string>
#include <string_view>

// What a view sets beside each of its items: the same item over an earlier
// window, shifted so that it lines up with the shown period. The Summary adds
// a column per item, the Graph a dashed line per line. Driven by the
// ID_COMPARE_* commands («Сравнить с»).
enum class PeriodComparison {
  kNone,
  kDayEarlier,
  kWeekEarlier,
  // As long as the shown period, ending where it starts.
  kPreviousPeriod,
};

// How far back the comparison reads, given how long the shown period is.
// Zero for kNone.
scada::Duration PeriodComparisonShift(PeriodComparison comparison,
                                      scada::Duration period_length);

// The comparison's name in a saved window, empty for kNone; and back, with
// kNone for anything it does not know, so an unknown name opens without one.
std::string_view PeriodComparisonName(PeriodComparison comparison);
PeriodComparison ParsePeriodComparison(std::string_view name);

// What an item's comparison series is called after the item's own name: «(на
// сутки раньше)». Translated; empty for kNone.
std::u16string PeriodComparisonSuffix(PeriodComparison comparison);
