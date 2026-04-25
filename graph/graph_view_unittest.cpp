#include "graph/graph_view.h"

#include "aui/test/app_environment.h"
#include "base/test/awaitable_test.h"
#include "resources/common_resources.h"
#include "controller/test/controller_environment.h"
#include "graph/metrix_data_source.h"
#include "graph/metrix_graph.h"
#include "node_service/node_model.h"
#include "scada/client.h"
#include "scada/history_service_mock.h"
#include "timed_data/timed_data_service_fake.h"

#include "base/debug_util.h"

#include <QImage>

using namespace testing;

namespace {

constexpr scada::NodeId kTestNodeId{1, 1};

class TestNodeModel final : public NodeModel {
 public:
  explicit TestNodeModel(scada::node node) : node_{std::move(node)} {}

  scada::Status GetStatus() const override { return scada::StatusCode::Good; }
  NodeFetchStatus GetFetchStatus() const override {
    return NodeFetchStatus::Max();
  }
  void Fetch(const NodeFetchStatus& requested_status,
             const FetchCallback& callback) const override {
    if (callback)
      callback();
  }
  scada::Variant GetAttribute(scada::AttributeId attribute_id) const override {
    return {};
  }
  NodeRef GetDataType() const override { return {}; }
  NodeRef::Reference GetReference(const scada::NodeId& reference_type_id,
                                  bool forward,
                                  const scada::NodeId& node_id) const override {
    return {};
  }
  std::vector<NodeRef::Reference> GetReferences(
      const scada::NodeId& reference_type_id,
      bool forward) const override {
    return {};
  }
  NodeRef GetTarget(const scada::NodeId& reference_type_id,
                    bool forward) const override {
    return {};
  }
  std::vector<NodeRef> GetTargets(const scada::NodeId& reference_type_id,
                                  bool forward) const override {
    return {};
  }
  NodeRef GetAggregate(
      const scada::NodeId& aggregate_declaration_id) const override {
    return {};
  }
  NodeRef GetChild(const scada::QualifiedName& child_name) const override {
    return {};
  }
  void Subscribe(NodeRefObserver& observer) const override {}
  void Unsubscribe(NodeRefObserver& observer) const override {}
  scada::node GetScadaNode() const override { return node_; }

 private:
  scada::node node_;
};

class NodeFakeTimedData final : public FakeTimedData {
 public:
  explicit NodeFakeTimedData(NodeRef node) : node_{std::move(node)} {}

  NodeRef GetNode() const override { return node_; }

 private:
  NodeRef node_;
};

scada::DataValue MakeDataValue(double value, scada::DateTime timestamp) {
  return {scada::Variant{value}, {}, timestamp, timestamp};
}

TimedDataSpec MakeTimedDataSpec(NodeRef node, scada::DateTime timestamp) {
  auto timed_data = std::make_shared<NodeFakeTimedData>(std::move(node));
  timed_data->data_values.push_back(MakeDataValue(1.0, timestamp));
  timed_data->ready_ranges.push_back({timestamp, timestamp});
  return TimedDataSpec{timed_data};
}

}  // namespace

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

TEST(MetrixDataSourceTest, AppliesEarliestTimestampFromHistoryRead) {
  auto executor = std::make_shared<TestExecutor>();
  StrictMock<scada::MockHistoryService> history_service;
  scada::services services{.history_service = &history_service};
  scada::client client{services};
  NodeRef node{std::make_shared<TestNodeModel>(client.node(kTestNodeId))};

  const auto earliest = scada::DateTime::FromDoubleT(100.0);
  const auto latest = scada::DateTime::FromDoubleT(200.0);

  EXPECT_CALL(history_service, HistoryReadRaw(_, _))
      .WillOnce(Invoke([&](const scada::HistoryReadRawDetails& details,
                           const scada::HistoryReadRawCallback& callback) {
        EXPECT_EQ(details.node_id, kTestNodeId);
        EXPECT_EQ(details.max_count, 1u);
        callback(scada::HistoryReadRawResult{
            .values = {MakeDataValue(1.0, earliest)}});
      }));

  MetrixDataSource data_source{MakeTestAnyExecutor(executor)};
  data_source.SetTimedData(MakeTimedDataSpec(node, latest));
  Drain(executor);

  auto horizontal_range = data_source.GetHorizontalRange();
  EXPECT_EQ(horizontal_range.low(), earliest.ToDoubleT());
  EXPECT_EQ(horizontal_range.high(), latest.ToDoubleT());
}

TEST(MetrixDataSourceTest, DropsCanceledEarliestTimestampRead) {
  auto executor = std::make_shared<TestExecutor>();
  StrictMock<scada::MockHistoryService> history_service;
  scada::services services{.history_service = &history_service};
  scada::client client{services};
  NodeRef node{std::make_shared<TestNodeModel>(client.node(kTestNodeId))};

  scada::HistoryReadRawCallback first_callback;
  scada::HistoryReadRawCallback second_callback;

  EXPECT_CALL(history_service, HistoryReadRaw(_, _))
      .WillOnce(Invoke([&](const scada::HistoryReadRawDetails& details,
                           const scada::HistoryReadRawCallback& callback) {
        first_callback = callback;
      }))
      .WillOnce(Invoke([&](const scada::HistoryReadRawDetails& details,
                           const scada::HistoryReadRawCallback& callback) {
        second_callback = callback;
      }));

  const auto stale_earliest = scada::DateTime::FromDoubleT(50.0);
  const auto current_earliest = scada::DateTime::FromDoubleT(100.0);
  const auto first_latest = scada::DateTime::FromDoubleT(200.0);
  const auto second_latest = scada::DateTime::FromDoubleT(300.0);

  MetrixDataSource data_source{MakeTestAnyExecutor(executor)};
  data_source.SetTimedData(MakeTimedDataSpec(node, first_latest));
  Drain(executor);
  ASSERT_TRUE(first_callback);

  data_source.SetTimedData(MakeTimedDataSpec(node, second_latest));
  Drain(executor);
  ASSERT_TRUE(second_callback);

  first_callback(scada::HistoryReadRawResult{
      .values = {MakeDataValue(1.0, stale_earliest)}});
  Drain(executor);
  EXPECT_TRUE(data_source.GetHorizontalRange().empty());

  second_callback(scada::HistoryReadRawResult{
      .values = {MakeDataValue(1.0, current_earliest)}});
  Drain(executor);

  auto horizontal_range = data_source.GetHorizontalRange();
  EXPECT_EQ(horizontal_range.low(), current_earliest.ToDoubleT());
  EXPECT_EQ(horizontal_range.high(), second_latest.ToDoubleT());
}
