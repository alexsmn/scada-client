#pragma once

#include <span>
#include <vector>

// A point in a sparkline's local pixel space (origin top-left, y grows down),
// ready to feed a QPainter polyline (or any renderer).
struct SparklinePoint {
  float x = 0;
  float y = 0;
};

// Computes the polyline for a per-row mini-trend (table-watch.html "Trend 1
// h"). Maps `values` (oldest -> newest, spaced evenly across the width) to
// points inside a `width` x `height` cell, leaving `padding` px at top and
// bottom so the stroke is not clipped. The series is normalized to its own
// min/max so small variations stay visible; a flat series draws a centred
// horizontal line. Fewer than two values (nothing to connect), or a
// non-positive size, yields an empty polyline. Pure and Qt-free so the geometry
// can be unit-tested; the Qt sparkline delegate converts the result to QPointF.
std::vector<SparklinePoint> ComputeSparklinePoints(
    std::span<const double> values,
    float width,
    float height,
    float padding = 2.0f);
