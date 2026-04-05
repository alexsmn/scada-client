#include "graph/graph_view.h"

#include "aui/test/app_environment.h"
#include "common_resources.h"
#include "controller/test/controller_environment.h"
#include "graph/metrix_graph.h"
#include "timed_data/timed_data_service_fake.h"

#include "base/debug_util.h"

#include <QImage>

using namespace testing;

class GraphViewTest : public Test {
 public:
  virtual void SetUp() override;

 protected:
  AppEnvironment app_env_;
  ControllerEnvironment env_;

  GraphView graph_view_{env_.MakeControllerContext()};

  std::unique_ptr<UiView> ui_view_;
};

void GraphViewTest::SetUp() {
  WindowDefinition def;
  ui_view_ = graph_view_.Init(def);
}

TEST_F(GraphViewTest, Test) {
  auto* time_model = graph_view_.GetTimeModel();
  ASSERT_THAT(time_model, NotNull());

  time_model->SetTimeRange(TimeRange::Type::Day);
  // EXPECT_THAT(time_model->GetTimeRange(), Eq(TimeRange::Type::Day));
}

TEST_F(GraphViewTest, FakeTimedDataRendersLines) {
  // Set up FakeTimedDataService with pre-populated data.
  FakeTimedDataService fake_service;
  auto now = base::Time::Now();

  auto td = fake_service.AddTimedData("TS.200");
  for (int i = 0; i < 24; ++i) {
    auto time = now - base::TimeDelta::FromHours(24 - i);
    td->data_values.push_back(
        scada::DataValue{scada::Variant{100.0 + i * 2.0}, {}, time, time});
  }
  td->ready_ranges.push_back({now - base::TimeDelta::FromHours(24), now});

  // Create a graph with one line using the fake service.
  MetrixGraph graph{MetrixGraphContext{fake_service}};
  auto& pane = graph.NewPane();
  auto& line = graph.NewLine("TS.200", pane);
  line.SetColor(Qt::blue);

  // Set horizontal range to match data.
  double from = (now - base::TimeDelta::FromHours(24)).ToDoubleT();
  double to = now.ToDoubleT();
  graph.horizontal_axis().SetTimeFit(false);
  graph.horizontal_axis().SetRange(
      views::GraphRange{from, to, views::GraphRange::TIME});

  graph.UpdateData();

  // Verify the data source has data.
  EXPECT_TRUE(line.data_source().connected());

  auto values = line.data_source().timed_data().values();
  EXPECT_EQ(values.size(), 24u);

  // Verify the data source can enumerate points.
  auto enumerator = line.data_source().EnumPoints(from, to, true, true);
  ASSERT_THAT(enumerator, NotNull());
  EXPECT_GT(enumerator->GetCount(), 0u);

  // Verify vertical axis range was auto-computed from data.
  auto vrange = pane.vertical_axis().range();
  EXPECT_FALSE(vrange.empty());

  // Render the graph to an image and verify lines are actually drawn.
  graph.setFixedSize(400, 300);
  graph.show();
  QApplication::processEvents();
  QImage image = graph.grab().toImage();

  // Count non-white, non-grid pixels in the plot area (the data line).
  // The plot area is roughly from x=10 to x=350, y=10 to y=250 (excluding
  // axes and borders).
  int colored_pixels = 0;
  for (int y = 10; y < image.height() - 30; ++y) {
    for (int x = 10; x < image.width() - 60; ++x) {
      QColor color = image.pixelColor(x, y);
      // Count blue pixels (the line color).
      if (color.blue() > 200 && color.red() < 50 && color.green() < 50) {
        ++colored_pixels;
      }
    }
  }

  EXPECT_GT(colored_pixels, 10)
      << "Expected blue line pixels in the rendered graph";
}
