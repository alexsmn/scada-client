#include "display_frame/qt/display_frame.h"

#include <gtest/gtest.h>

#include <QPoint>
#include <QSize>

#include <cmath>
#include <limits>

// The DisplayFrame zoom maths are pure free functions so they can be verified
// without a running QApplication.
namespace {

TEST(DisplayFrameZoomTest, ClampKeepsInRangeValues) {
  EXPECT_DOUBLE_EQ(ClampDisplayZoom(1.0), 1.0);
  EXPECT_DOUBLE_EQ(ClampDisplayZoom(0.5), 0.5);
  EXPECT_DOUBLE_EQ(ClampDisplayZoom(4.0), 4.0);
}

TEST(DisplayFrameZoomTest, ClampBoundsExtremes) {
  EXPECT_DOUBLE_EQ(ClampDisplayZoom(0.0), 0.05);
  EXPECT_DOUBLE_EQ(ClampDisplayZoom(-3.0), 0.05);
  EXPECT_DOUBLE_EQ(ClampDisplayZoom(1000.0), 8.0);
}

TEST(DisplayFrameZoomTest, ClampRejectsNonFinite) {
  EXPECT_DOUBLE_EQ(ClampDisplayZoom(std::numeric_limits<double>::quiet_NaN()),
                   1.0);
  EXPECT_DOUBLE_EQ(ClampDisplayZoom(std::numeric_limits<double>::infinity()),
                   1.0);
}

TEST(DisplayFrameZoomTest, FitContainsWidthLimited) {
  // A wide page in a square viewport is width-limited.
  EXPECT_DOUBLE_EQ(DisplayFitFactor(QSize{1000, 500}, QSize{500, 500}), 0.5);
}

TEST(DisplayFrameZoomTest, FitContainsHeightLimited) {
  // A tall page in a square viewport is height-limited.
  EXPECT_DOUBLE_EQ(DisplayFitFactor(QSize{500, 1000}, QSize{500, 500}), 0.5);
}

TEST(DisplayFrameZoomTest, FitDegenerateInputsAreOneToOne) {
  EXPECT_DOUBLE_EQ(DisplayFitFactor(QSize{0, 0}, QSize{500, 500}), 1.0);
  EXPECT_DOUBLE_EQ(DisplayFitFactor(QSize{500, 500}, QSize{0, 0}), 1.0);
}

TEST(DisplayFrameZoomTest, FitIsClampedToMaxZoom) {
  // A tiny page in a huge viewport must not exceed the max zoom.
  EXPECT_DOUBLE_EQ(DisplayFitFactor(QSize{10, 10}, QSize{10000, 10000}), 8.0);
}

TEST(DisplayFrameZoomTest, PercentRoundsToNearest) {
  EXPECT_EQ(DisplayZoomPercent(1.0), 100);
  EXPECT_EQ(DisplayZoomPercent(0.5), 50);
  EXPECT_EQ(DisplayZoomPercent(1.234), 123);
  EXPECT_EQ(DisplayZoomPercent(1.236), 124);
}

// The legend floats over the diagram viewport, so its placement is arithmetic
// rather than layout and is checkable without a QApplication.

TEST(DisplayLegendOriginTest, SitsAtTheViewportBottomLeft) {
  // 400 - 30 - 12 = 358.
  EXPECT_EQ(DisplayLegendOrigin(QSize{300, 30}, QSize{800, 400}),
            (QPoint{14, 358}));
}

// A viewport shorter than the legend would otherwise place it above the top
// edge, hiding the entries entirely; overlapping the diagram is the lesser
// loss.
TEST(DisplayLegendOriginTest, ClampsToTheTopOfATinyViewport) {
  EXPECT_EQ(DisplayLegendOrigin(QSize{300, 90}, QSize{800, 40}),
            (QPoint{14, 0}));
}

}  // namespace
