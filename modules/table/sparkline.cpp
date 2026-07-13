#include "modules/table/sparkline.h"

#include <algorithm>
#include <cmath>
#include <vector>

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

  const int count = static_cast<int>(finite.size());
  std::vector<SparklinePoint> points;
  points.reserve(finite.size());
  for (int i = 0; i < count; ++i) {
    const float x = width * static_cast<float>(i) / (count - 1);
    // Higher value -> smaller y (towards the top). A flat series (range == 0)
    // has no meaningful vertical position, so centre it.
    const float y =
        range > 0
            ? padding + static_cast<float>((max_value - finite[i]) / range) *
                            usable_height
            : mid_y;
    points.push_back({x, y});
  }
  return points;
}
