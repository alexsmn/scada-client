#include "display_view/qt/display_widget.h"

#include <gtest/gtest.h>

#include <QRectF>
#include <QSize>

#include <limits>

namespace {

using scada::display::RectF;
using scada::display::view::ShapeHit;

// A 200x100 page, shown in a 400x200 widget: a 2x scale on both axes, which
// keeps the arithmetic checkable by eye while still exercising the flip.
const RectF kPage{0, 0, 200, 100};
const QSize kWidget{400, 200};

// The halo is drawn from the hit's page bounds, so a flip error here puts it
// on the wrong shape -- mirrored about the page's middle, which looks
// plausible on a symmetric diagram and is wrong on every other one.
TEST(DisplayPageRectToWidgetTest, FlipsPageYIntoWidgetY) {
  // A shape occupying page y 10..30, i.e. NEAR THE BOTTOM of a page whose y
  // grows upward. In widget coordinates that is near the bottom too, so the
  // widget top edge is the far one: (100 - 30) * 2 = 140.
  const QRectF mapped =
      DisplayPageRectToWidget(RectF{20, 10, 40, 20}, kPage, kWidget);

  EXPECT_DOUBLE_EQ(mapped.left(), 40.0);
  EXPECT_DOUBLE_EQ(mapped.width(), 80.0);
  EXPECT_DOUBLE_EQ(mapped.top(), 140.0);
  EXPECT_DOUBLE_EQ(mapped.height(), 40.0);
}

// The inverse of the hit test's own mapping, checked at the two extremes: the
// page's top edge is the widget's y=0 and its bottom edge the widget's height.
TEST(DisplayPageRectToWidgetTest, MapsThePageOntoTheWholeWidget) {
  const QRectF mapped = DisplayPageRectToWidget(kPage, kPage, kWidget);

  EXPECT_DOUBLE_EQ(mapped.left(), 0.0);
  EXPECT_DOUBLE_EQ(mapped.top(), 0.0);
  EXPECT_DOUBLE_EQ(mapped.width(), 400.0);
  EXPECT_DOUBLE_EQ(mapped.height(), 200.0);
}

// A negative height is what a rect built from two corners in the other order
// gives; taking the span rather than the sign keeps the halo on the shape.
TEST(DisplayPageRectToWidgetTest, NormalisesANegativeHeight) {
  const QRectF mapped =
      DisplayPageRectToWidget(RectF{20, 30, 40, -20}, kPage, kWidget);

  EXPECT_DOUBLE_EQ(mapped.top(), 140.0);
  EXPECT_DOUBLE_EQ(mapped.height(), 40.0);
}

// Degenerate inputs answer "nothing to draw" rather than dividing by zero: the
// widget is unsized before its first layout, and a document with no usable
// page metrics is a real case the facade tolerates.
TEST(DisplayPageRectToWidgetTest, DegenerateInputsAreEmpty) {
  EXPECT_TRUE(
      DisplayPageRectToWidget(kPage, RectF{0, 0, 0, 0}, kWidget).isEmpty());
  EXPECT_TRUE(DisplayPageRectToWidget(kPage, kPage, QSize{0, 0}).isEmpty());
  EXPECT_TRUE(DisplayPageRectToWidget(kPage, kPage, QSize{400, 0}).isEmpty());
}

TEST(DisplayPageRectToWidgetTest, NonFiniteInputsAreEmpty) {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  EXPECT_TRUE(
      DisplayPageRectToWidget(RectF{nan, 0, 10, 10}, kPage, kWidget).isEmpty());
  EXPECT_TRUE(
      DisplayPageRectToWidget(kPage, RectF{0, 0, nan, 100}, kWidget).isEmpty());
}

TEST(DisplayShapeLabelTest, PrefersTheAuthoredName) {
  ShapeHit hit;
  hit.name = "Q1";
  hit.text = "110 kV";

  EXPECT_EQ(DisplayShapeLabel(hit), QStringLiteral("Q1"));
}

TEST(DisplayShapeLabelTest, FallsBackToTheDrawnText) {
  ShapeHit hit;
  hit.text = "110 kV";

  EXPECT_EQ(DisplayShapeLabel(hit), QStringLiteral("110 kV"));
}

// A shape with neither gets no label, and the chrome then says nothing rather
// than showing an internal id the operator cannot match to anything.
TEST(DisplayShapeLabelTest, IsEmptyWhenTheShapeHasNeither) {
  ShapeHit hit;
  hit.id = 42;

  EXPECT_TRUE(DisplayShapeLabel(hit).isEmpty());
}

}  // namespace
