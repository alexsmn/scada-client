#include "modules/table/sparkline.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

std::vector<double> NumericSeries(std::span<const scada::DataValue> values) {
  std::vector<double> series;
  series.reserve(values.size());
  for (const scada::DataValue& value : values) {
    double sample = 0;
    if (value.value.get(sample))
      series.push_back(sample);
  }
  return series;
}

std::vector<SparklinePoint> ComputeSparklinePoints(
    std::span<const double> values,
    float width,
    float height,
    float padding) {
  // Degenerate cell.
  if (width <= 0 || height <= 0)
    return {};

  // Drop non-finite samples: NaN/inf commonly represent a bad/unavailable
  // value, and even one would poison std::minmax_element and the range, making
  // every plotted y NaN (a garbage polyline). The line is drawn over the
  // remaining good samples.
  std::vector<double> finite;
  finite.reserve(values.size());
  for (double value : values) {
    if (std::isfinite(value))
      finite.push_back(value);
  }

  // Nothing to connect.
  if (finite.size() < 2)
    return {};

  const auto [min_it, max_it] =
      std::minmax_element(finite.begin(), finite.end());
  const double min_value = *min_it;
  const double max_value = *max_it;
  const double range = max_value - min_value;

  // Vertical band the line may occupy, inset by the padding so the stroke and
  // dot markers are not clipped at the top/bottom edges.
  const float usable_height = std::max(0.0f, height - 2 * padding);
  const float mid_y = height / 2;

  // Higher value -> smaller y (towards the top). A flat series (range == 0)
  // has no meaningful vertical position, so centre it.
  const auto y_of = [&](double value) {
    return range > 0
               ? padding + static_cast<float>((max_value - value) / range) *
                               usable_height
               : mid_y;
  };

  const int count = static_cast<int>(finite.size());
  // One point per pixel column is all the cell can show. A row's trailing
  // window can hold thousands of samples, so past two per column the series
  // is reduced to each column's min and max, in sample order: the drawn
  // envelope is unchanged and the polyline stays bounded by the cell width.
  const int columns = std::max(2, static_cast<int>(width));
  if (count <= 2 * columns) {
    std::vector<SparklinePoint> points;
    points.reserve(finite.size());
    for (int i = 0; i < count; ++i) {
      const float x = width * static_cast<float>(i) / (count - 1);
      points.push_back({x, y_of(finite[i])});
    }
    return points;
  }

  std::vector<SparklinePoint> points;
  points.reserve(2 * columns);
  for (int column = 0; column < columns; ++column) {
    const auto first = finite.begin() + int64_t{column} * count / columns;
    const auto last = finite.begin() + int64_t{column + 1} * count / columns;
    const auto [lo, hi] = std::minmax_element(first, last);
    const float x = width * static_cast<float>(column) / (columns - 1);
    if (lo == hi) {
      points.push_back({x, y_of(*lo)});
    } else {
      points.push_back({x, y_of(*std::min(lo, hi))});
      points.push_back({x, y_of(*std::max(lo, hi))});
    }
  }
  return points;
}
