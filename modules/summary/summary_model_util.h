#pragma once

#include "base/check.h"
#include "base/relative_time_range.h"
#include "base/struct_writer.h"
#include "common/aggregation.h"
#include "scada/date_time.h"

#include <optional>
#include <span>
#include <vector>

namespace {

const size_t kMaxColumnCount = 1000;
const size_t kMaxRowCount = 10000;

}  // namespace

struct SummaryModelParams {
  bool operator==(const SummaryModelParams&) const = default;

  scada::Time start_time = scada::kNullTime;
  scada::Time end_time = scada::kNullTime;
  size_t row_count;
  // True when the period asked for more than kMaxRowCount rows, so
  // `end_time` falls short of it (backlog 871).
  bool truncated = false;
};

inline SummaryModelParams CalculateSummaryModelParams(
    const scada::RelativeTimeRange& time_range,
    scada::Duration interval,
    scada::Time now) {
  scada::base::Check(interval != scada::Duration::zero());

  auto [start_time, end_time] = scada::ToTimeRange(time_range, now);

  // Align bounds to the aggregation interval.
  auto origin_time = scada::GetLocalAggregateStartTime();

  // Round down.
  start_time =
      scada::GetAggregateInterval(start_time, origin_time, interval).first;

  // Round up.
  auto end_range = scada::GetAggregateInterval(end_time, origin_time, interval);
  end_time = end_time == end_range.first ? end_range.first : end_range.second;

  auto delta = end_time - start_time;
  int64_t row_count = delta / interval;
  const bool truncated = row_count > static_cast<int64_t>(kMaxRowCount);
  row_count = std::min(row_count, static_cast<int64_t>(kMaxRowCount));

  SummaryModelParams result;
  result.truncated = truncated;
  result.start_time = start_time;
  result.end_time = start_time + interval * row_count;
  result.row_count = static_cast<size_t>(row_count);

  scada::base::Check(!scada::IsNull(result.start_time));
  scada::base::Check(!scada::IsNull(result.end_time));
  scada::base::Check(result.start_time <= result.end_time);
  scada::base::Check(result.row_count <= kMaxRowCount);

  return result;
}

// The Summary's «Расход» (consumption) of a cumulative counter, per row,
// from the counter's last reading in each interval (the Last aggregate).
//
// `lead_in` is the last reading in the interval before the first row, which
// is what the first row's consumption is measured from. A row's consumption is
// its last reading minus the last reading before it. A row with no reading
// consumed nothing — a counter that did not change has no samples to report,
// which is exactly why the Last values leave empty cells — so it is 0 once a
// reading before it is known, and unknown (nullopt) until then.
//
// A counter that was reset or replaced reads lower than before; that row
// shows the negative difference rather than hiding it, so the operator sees
// the reset where it happened.
inline std::vector<std::optional<double>> ComputeConsumption(
    std::optional<double> lead_in,
    std::span<const std::optional<double>> last_readings) {
  std::vector<std::optional<double>> consumption;
  consumption.reserve(last_readings.size());
  std::optional<double> baseline = lead_in;
  for (const std::optional<double>& reading : last_readings) {
    if (!baseline)
      consumption.push_back(std::nullopt);
    else if (!reading)
      consumption.push_back(0.0);
    else
      consumption.push_back(*reading - *baseline);
    if (reading)
      baseline = reading;
  }
  return consumption;
}

inline std::ostream& operator<<(std::ostream& stream,
                                const SummaryModelParams& params) {
  StructWriter{stream}
      .AddField("start_time", ToString(params.start_time))
      .AddField("end_time", ToString(params.end_time))
      .AddField("row_count", ToString(params.row_count))
      .AddField("truncated", params.truncated ? "true" : "false");
  return stream;
}
