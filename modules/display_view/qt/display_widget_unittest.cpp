#include "display_view/qt/display_widget.h"

#include "aui/test/app_environment.h"

#include <gtest/gtest.h>

#include <QRectF>
#include <QSize>

#include <filesystem>
#include <limits>
#include <string>
#include <vector>

namespace {

// A 200x100 page, shown in a 400x200 widget: a 2x scale on both axes, which
// keeps the arithmetic checkable by eye while still exercising the flip.
const DisplayRect kPage{0, 0, 200, 100};
const QSize kWidget{400, 200};

// The halo is drawn from the hit's page bounds, so a flip error here puts it
// on the wrong shape -- mirrored about the page's middle, which looks
// plausible on a symmetric diagram and is wrong on every other one.
TEST(DisplayPageRectToWidgetTest, FlipsPageYIntoWidgetY) {
  // A shape occupying page y 10..30, i.e. NEAR THE BOTTOM of a page whose y
  // grows upward. In widget coordinates that is near the bottom too, so the
  // widget top edge is the far one: (100 - 30) * 2 = 140.
  const QRectF mapped =
      DisplayPageRectToWidget(DisplayRect{20, 10, 40, 20}, kPage, kWidget);

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
      DisplayPageRectToWidget(DisplayRect{20, 30, 40, -20}, kPage, kWidget);

  EXPECT_DOUBLE_EQ(mapped.top(), 140.0);
  EXPECT_DOUBLE_EQ(mapped.height(), 40.0);
}

// Degenerate inputs answer "nothing to draw" rather than dividing by zero: the
// widget is unsized before its first layout, and a document with no usable
// page metrics is a real case the facade tolerates.
TEST(DisplayPageRectToWidgetTest, DegenerateInputsAreEmpty) {
  EXPECT_TRUE(DisplayPageRectToWidget(kPage, DisplayRect{0, 0, 0, 0}, kWidget)
                  .isEmpty());
  EXPECT_TRUE(DisplayPageRectToWidget(kPage, kPage, QSize{0, 0}).isEmpty());
  EXPECT_TRUE(DisplayPageRectToWidget(kPage, kPage, QSize{400, 0}).isEmpty());
}

TEST(DisplayPageRectToWidgetTest, NonFiniteInputsAreEmpty) {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  EXPECT_TRUE(
      DisplayPageRectToWidget(DisplayRect{nan, 0, 10, 10}, kPage, kWidget)
          .isEmpty());
  EXPECT_TRUE(
      DisplayPageRectToWidget(kPage, DisplayRect{0, 0, nan, 100}, kWidget)
          .isEmpty());
}

TEST(DisplayShapeLabelTest, PrefersTheAuthoredName) {
  DisplayShapeHit hit;
  hit.name = "Q1";
  hit.text = "110 kV";

  EXPECT_EQ(DisplayShapeLabel(hit), QStringLiteral("Q1"));
}

TEST(DisplayShapeLabelTest, FallsBackToTheDrawnText) {
  DisplayShapeHit hit;
  hit.text = "110 kV";

  EXPECT_EQ(DisplayShapeLabel(hit), QStringLiteral("110 kV"));
}

// A shape with neither gets no label, and the chrome then says nothing rather
// than showing an internal id the operator cannot match to anything.
TEST(DisplayShapeLabelTest, IsEmptyWhenTheShapeHasNeither) {
  DisplayShapeHit hit;
  hit.id = 42;

  EXPECT_TRUE(DisplayShapeLabel(hit).isEmpty());
}

// What the schematic was last given, read back. The runtime draws a reading
// but cannot report one, and the client's E2E Display menu check is how a run
// learns what an opened schematic shows; this pins that readback against the
// real runtime, when the configure named one (display_runtime_unittest.cpp
// points the loader at it before the first test).
TEST(DisplayWidgetReadingTest, RemembersTheLastReadingPerDataSource) {
#ifdef SCADA_DISPLAY_TEST_DOCUMENT
  const std::filesystem::path document{SCADA_DISPLAY_TEST_DOCUMENT};
#else
  const std::filesystem::path document;
#endif
  if (!DisplayRuntime::Get() || document.empty() ||
      !std::filesystem::exists(document)) {
    GTEST_SKIP() << "no display runtime or fixture document in this build";
  }

  AppEnvironment app_env;
  DisplayWidget widget;
  ASSERT_TRUE(widget.Open(document, DisplayDocumentKind::kAuto))
      << widget.error_message().toStdString();
  const std::vector<std::string> data_sources = widget.DataSources();
  ASSERT_FALSE(data_sources.empty());
  const std::string& bound = data_sources.front();

  EXPECT_FALSE(widget.LastReading(bound));

  widget.ShowReading({.data_source = bound, .value = 1.0, .text = "1"});
  widget.ShowReading(
      {.data_source = bound, .value = 73.25, .text = "73.25", .quality = 0});
  const std::optional<DisplayDataSourceReading> reading =
      widget.LastReading(bound);
  ASSERT_TRUE(reading);
  EXPECT_EQ(reading->text, "73.25");
  ASSERT_TRUE(reading->value);
  EXPECT_DOUBLE_EQ(*reading->value, 73.25);
  EXPECT_EQ(reading->quality, 0);

  EXPECT_FALSE(widget.LastReading("not bound anywhere"));

  // A reading belongs to the document it was given to.
  ASSERT_TRUE(widget.Open(document, DisplayDocumentKind::kAuto));
  EXPECT_FALSE(widget.LastReading(bound));
}

}  // namespace
