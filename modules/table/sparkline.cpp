#include "modules/table/sparkline.h"

#include <algorithm>

std::vector<SparklinePoint> ComputeSparklinePoints(
    std::span<const double> values,
    float width,
    float height,
    float padding) {
  // Nothing to connect, or a degenerate cell.
  if (values.size() < 2 || width <= 0 || height <= 0)
    return {};

  const auto [min_it, max_it] =
      std::minmax_element(values.begin(), values.end());
  const double min_value = *min_it;
  const double max_value = *max_it;
  const double range = max_value - min_value;

  // Vertical band the line may occupy, inset by the padding so the stroke and
  // dot markers are not clipped at the top/bottom edges.
  const float usable_height = std::max(0.0f, height - 2 * padding);
  const float mid_y = height / 2;

  const int count = static_cast<int>(values.size());
  std::vector<SparklinePoint> points;
  points.reserve(values.size());
  for (int i = 0; i < count; ++i) {
    const float x = width * static_cast<float>(i) / (count - 1);
    // Higher value -> smaller y (towards the top). A flat series (range == 0)
    // has no meaningful vertical position, so centre it.
    const float y =
        range > 0
            ? padding + static_cast<float>((max_value - values[i]) / range) *
                            usable_height
            : mid_y;
    points.push_back({x, y});
  }
  return points;
}
