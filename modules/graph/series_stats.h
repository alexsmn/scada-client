#pragma once

#include "scada/data_value.h"
#include "scada/date_time.h"

#include <cstddef>
#include <span>

// Aggregate statistics for one trend series over the visible time range, shown
// in the trend value grid's Min / Max / Average columns (see the reshell mockup
// client/docs/ui-mockups/screens/trend.html).
//
// Only good-quality, numeric samples contribute: bad-quality samples (comms
// loss / failed) and non-numeric values are excluded so a dropout cannot
// corrupt the extremes or the mean. This matches the points the trend actually
// plots (see MetrixDataSource's point enumerator).
struct SeriesStats {
  // False when no good numeric sample fell in the range; the numeric fields are
  // then left at zero and callers should render a placeholder (e.g. "—").
  bool valid = false;
  double min = 0.0;
  double max = 0.0;
  double average = 0.0;
  // Number of good numeric samples that contributed to the aggregates.
  std::size_t count = 0;
};

// Computes SeriesStats over the samples whose source timestamp lies in the
// inclusive range [from, to]. Pure and free of Qt/service dependencies so it is
// unit-tested directly. `values` need not be sorted. A sample contributes when
// its qualifier is good and its variant yields a numeric (double-convertible)
// value.
SeriesStats ComputeSeriesStats(std::span<const scada::DataValue> values,
                               scada::DateTime from,
                               scada::DateTime to);
