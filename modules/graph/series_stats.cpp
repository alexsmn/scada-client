#include "graph/series_stats.h"

#include <algorithm>

SeriesStats ComputeSeriesStats(std::span<const scada::DataValue> values,
                               scada::DateTime from,
                               scada::DateTime to) {
  SeriesStats stats;
  double sum = 0.0;

  for (const scada::DataValue& sample : values) {
    // Inclusive range so a cursor pinned to an endpoint sample still counts it.
    if (sample.source_timestamp < from || sample.source_timestamp > to)
      continue;
    if (!sample.qualifier.good())
      continue;

    // Reject non-numeric values (strings, node ids, null); numeric integer
    // variants are coerced to double, matching how the trend plots them.
    double y = 0.0;
    if (!sample.value.get(y))
      continue;

    if (stats.count == 0) {
      stats.min = stats.max = y;
    } else {
      stats.min = std::min(stats.min, y);
      stats.max = std::max(stats.max, y);
    }
    sum += y;
    ++stats.count;
  }

  if (stats.count > 0) {
    stats.valid = true;
    stats.average = sum / static_cast<double>(stats.count);
  }
  return stats;
}
