#pragma once

#include "base/struct_writer.h"
#include "common/aggregation.h"
#include "scada/date_time.h"
#include "base/time_range.h"

namespace {

const size_t kMaxColumnCount = 1000;
const size_t kMaxRowCount = 10000;

struct SummaryModelParams {
  scada::DateTime start_time;
  scada::DateTime end_time;
  size_t row_count;
};

inline SummaryModelParams CalculateSummaryModelParams(
    const TimeRange& time_range,
    scada::Duration interval) {
  assert(!interval.is_zero());

  auto [start_time, end_time] = ToDateTimeRange(time_range);

  // Align bounds to the aggregation interval.
  auto origin_time = scada::GetLocalAggregateStartTime();
  // Round down.
  start_time =
      scada::GetAggregateInterval(start_time, origin_time, interval).first;
  // Round up.
  end_time =
      scada::GetAggregateInterval(end_time, origin_time, interval).second;

  auto delta = end_time - start_time;
  int64_t row_count = delta / interval;
  row_count = std::min(row_count, static_cast<int64_t>(kMaxRowCount));

  SummaryModelParams result;
  result.start_time = start_time;
  result.end_time = start_time + interval * row_count;
  result.row_count = static_cast<size_t>(row_count);

  assert(!result.start_time.is_null());
  assert(!result.end_time.is_null());
  assert(result.start_time <= result.end_time);
  assert(result.row_count <= kMaxRowCount);

  return result;
}

inline bool operator==(const SummaryModelParams& a,
                       const SummaryModelParams& b) {
  return std::tie(a.start_time, a.end_time, a.row_count) ==
         std::tie(b.start_time, b.end_time, b.row_count);
}

inline std::ostream& operator<<(std::ostream& stream,
                                const SummaryModelParams& params) {
  StructWriter{stream}
      .AddField("start_time", ToString(params.start_time))
      .AddField("end_time", ToString(params.end_time))
      .AddField("row_count", ToString(params.row_count));
  return stream;
}

}  // namespace
