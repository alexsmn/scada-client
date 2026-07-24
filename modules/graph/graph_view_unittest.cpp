#include "graph/graph_view.h"
#include "base/time/time_wire_codec.h"

#include "aui/severity_colors.h"
#include "aui/test/app_environment.h"
#include "base/async_completion.h"
#include "base/test/awaitable_test.h"
#include "controller/test/controller_environment.h"
#include "graph/metrix_data_source.h"
#include "graph/metrix_graph.h"
#include "graph/series_inspector.h"
#include "node_service/node_model.h"
#include "node_service/test/model_node_service.h"
#include "resources/common_resources.h"
#include "scada/client.h"
#include "scada/history_service_mock.h"
#include "timed_data/timed_data_service_fake.h"

#include "base/debug_util.h"
#include "scada/co_result.h"

#include <QImage>

using namespace testing;

namespace {

constexpr scada::NodeId kTestNodeId{1, 1};

class TestNodeModel final : public NodeModel {
 public:
  explicit TestNodeModel(scada::node node) : node_{std::move(node)} {}

  scada::Status GetStatus() const override { return scada::StatusCode::Good; }
  NodeFetchStatus GetFetchStatus() const override {
    return NodeFetchStatus::Max;
  }
  Awaitable<void> Fetch(
      const NodeFetchStatus& requested_status) const override {
    co_return;
  }
  void StartFetch(const NodeFetchStatus& requested_status) const override {}
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

scada::DataValue MakeDataValue(double value, scada::Time timestamp) {
  return {scada::Variant{value}, {}, timestamp, timestamp};
}

TimedDataSpec MakeTimedDataSpec(NodeRef node, scada::Time timestamp) {
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

  time_model->SetTimeRange(scada::RelativeTimeRange::Type::Day);
  // EXPECT_THAT(time_model->GetTimeRange(),
  // Eq(scada::RelativeTimeRange::Type::Day));
}

TEST_F(GraphViewTest, GraphSetupCommandRegistered) {
  EXPECT_THAT(graph_view_.GetCommandHandler(ID_GRAPH_SETUP), NotNull());
}

TEST_F(GraphViewTest, GraphSetupCommandEnabledWhenGraphHasLine) {
  auto* command_handler = graph_view_.GetCommandHandler(ID_GRAPH_SETUP);
  ASSERT_THAT(command_handler, NotNull());
  EXPECT_TRUE(command_handler->IsCommandEnabled(ID_GRAPH_SETUP));

  int selection_change_count = 0;
  graph_view_.GetSelectionModel()->change_handler = [&] {
    ++selection_change_count;
  };

  graph_view_.AddContainedItem(kTestNodeId, 0);

  EXPECT_TRUE(command_handler->IsCommandEnabled(ID_GRAPH_SETUP));
  EXPECT_GT(selection_change_count, 0);
}

TEST_F(GraphViewTest, NewLineUsesDefaultLineWidth) {
  env_.profile_.graph_view.default_width = 4;

  graph_view_.AddContainedItem(kTestNodeId, 0);

  WindowDefinition definition;
  graph_view_.Save(definition);

  const WindowItem* graph_item = nullptr;
  for (const auto& item : definition.items) {
    if (item.name_is("Item")) {
      graph_item = &item;
      break;
    }
  }
  ASSERT_THAT(graph_item, NotNull());
  EXPECT_EQ(graph_item->GetInt("width"), 4);
}

// Regression: deleting the selected pane frees its lines, one of which the
// reshell series inspector may point at. DeleteSelectedPane must re-derive the
// inspector's line afterwards (it used to leave a dangling pointer that the
// next repaint dereferenced — a use-after-free). Standalone (not the fixture)
// so the reshell theme is active during Init, which is when the inspector is
// created.
TEST(GraphViewInspectorTest, DeletingSelectedPaneRefreshesSeriesInspector) {
  AppEnvironment app_env;
  ControllerEnvironment env;

  const scada::aui::SeverityTheme previous_theme =
      scada::aui::GetSeverityTheme();
  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kDark);
  GraphView view{env.MakeControllerContext()};
  WindowDefinition def;
  std::unique_ptr<UiView> ui = view.Init(def);
  scada::aui::SetSeverityTheme(previous_theme);

  view.AddContainedItem(kTestNodeId, 0);

  SeriesInspector* inspector = view.inspector();
  ASSERT_THAT(inspector, NotNull());  // reshell theme => inspector exists

  // Delete the selected pane via its command (DeleteSelectedPane is private).
  CommandHandler* delete_handler = view.GetCommandHandler(ID_GRAPH_DELETE_PANE);
  ASSERT_THAT(delete_handler, NotNull());
  ASSERT_TRUE(delete_handler->IsCommandEnabled(ID_GRAPH_DELETE_PANE));
  delete_handler->ExecuteCommand(ID_GRAPH_DELETE_PANE);

  // No panes remain, so the inspector's line must be cleared — not left
  // pointing at a freed line.
  EXPECT_THAT(inspector->line(), IsNull());
}

TEST_F(GraphViewTest, FakeTimedDataRendersLines) {
  // Set up FakeTimedDataService with pre-populated data.
  FakeTimedDataService fake_service;
  auto now = scada::Now();

  auto td = fake_service.AddTimedData("TS.200");
  for (int i = 0; i < 24; ++i) {
    auto time = now - std::chrono::hours(24 - i);
    td->data_values.push_back(
        scada::DataValue{scada::Variant{100.0 + i * 2.0}, {}, time, time});
  }
  td->ready_ranges.push_back({now - std::chrono::hours(24), now});

  // Create a graph with one line using the fake service.
  MetrixGraph graph{MetrixGraphContext{fake_service}};
  auto& pane = graph.NewPane();
  auto& line = graph.NewLine("TS.200", pane);
  line.SetColor(Qt::blue);

  // Set horizontal range to match data.
  double from = scada::base::EncodeDoubleT((now - std::chrono::hours(24)));
  double to = scada::base::EncodeDoubleT(now);
  graph.horizontal_axis().SetTimeFit(false);
  graph.horizontal_axis().SetRange(GraphRange{from, to, GraphRange::TIME});

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

TEST_F(GraphViewTest, NewColorSkipsLowContrastColorOnDarkBackground) {
  graph_view_.SetGraphColor(QColor{20, 20, 20});

  const QColor color = graph_view_.NewColor().qcolor();

  EXPECT_NE(color, QColor(Qt::black));
  EXPECT_NE(color, QColor(Qt::white));
  EXPECT_NE(color, QColor(Qt::transparent));
  EXPECT_GT(color.red() + color.green() + color.blue(), 120);
}

TEST(MetrixDataSourceTest, AppliesEarliestTimestampFromHistoryRead) {
  TestExecutor executor;
  StrictMock<scada::MockHistoryService> history_service;
  scada::services services{.history_service = &history_service};
  scada::client client{services};
  ModelNodeService node_service;
  NodeRef node = node_service.Add(
      kTestNodeId, std::make_shared<TestNodeModel>(client.node(kTestNodeId)));

  const auto earliest = scada::base::DecodeDoubleT(100.0);
  const auto latest = scada::base::DecodeDoubleT(200.0);

  EXPECT_CALL(history_service, HistoryReadRaw(_))
      .WillOnce(Invoke([&](scada::HistoryReadRawDetails details)
                           -> scada::CoStatusOr<scada::HistoryReadRawResult> {
        EXPECT_EQ(details.node_id, kTestNodeId);
        EXPECT_EQ(details.max_count, 1u);
        co_return scada::HistoryReadRawResult{
            .values = {MakeDataValue(1.0, earliest)}};
      }));

  MetrixDataSource data_source{executor};
  data_source.SetTimedData(MakeTimedDataSpec(node, latest));
  Drain(executor);

  auto horizontal_range = data_source.GetHorizontalRange();
  EXPECT_EQ(horizontal_range.low(), scada::base::EncodeDoubleT(earliest));
  EXPECT_EQ(horizontal_range.high(), scada::base::EncodeDoubleT(latest));
}

TEST(MetrixDataSourceTest, DropsCanceledEarliestTimestampRead) {
  TestExecutor executor;
  StrictMock<scada::MockHistoryService> history_service;
  scada::services services{.history_service = &history_service};
  scada::client client{services};
  ModelNodeService node_service;
  NodeRef node = node_service.Add(
      kTestNodeId, std::make_shared<TestNodeModel>(client.node(kTestNodeId)));

  scada::base::AsyncCompletion first_completion{executor};
  scada::base::AsyncCompletion second_completion{executor};
  scada::HistoryReadRawResult first_result;
  scada::HistoryReadRawResult second_result;
  bool first_started = false;
  bool second_started = false;

  EXPECT_CALL(history_service, HistoryReadRaw(_))
      .WillOnce(Invoke([&](scada::HistoryReadRawDetails details)
                           -> scada::CoStatusOr<scada::HistoryReadRawResult> {
        EXPECT_EQ(details.node_id, kTestNodeId);
        first_started = true;
        co_await first_completion.Wait();
        co_return first_result;
      }))
      .WillOnce(Invoke([&](scada::HistoryReadRawDetails details)
                           -> scada::CoStatusOr<scada::HistoryReadRawResult> {
        EXPECT_EQ(details.node_id, kTestNodeId);
        second_started = true;
        co_await second_completion.Wait();
        co_return second_result;
      }));

  const auto stale_earliest = scada::base::DecodeDoubleT(50.0);
  const auto current_earliest = scada::base::DecodeDoubleT(100.0);
  const auto first_latest = scada::base::DecodeDoubleT(200.0);
  const auto second_latest = scada::base::DecodeDoubleT(300.0);

  MetrixDataSource data_source{executor};
  data_source.SetTimedData(MakeTimedDataSpec(node, first_latest));
  Drain(executor);
  ASSERT_TRUE(first_started);

  data_source.SetTimedData(MakeTimedDataSpec(node, second_latest));
  Drain(executor);
  ASSERT_TRUE(second_started);

  first_result = scada::HistoryReadRawResult{
      .values = {MakeDataValue(1.0, stale_earliest)}};
  first_completion.Complete();
  Drain(executor);
  EXPECT_TRUE(data_source.GetHorizontalRange().empty());

  second_result = scada::HistoryReadRawResult{
      .values = {MakeDataValue(1.0, current_earliest)}};
  second_completion.Complete();
  Drain(executor);

  auto horizontal_range = data_source.GetHorizontalRange();
  EXPECT_EQ(horizontal_range.low(),
            scada::base::EncodeDoubleT(current_earliest));
  EXPECT_EQ(horizontal_range.high(), scada::base::EncodeDoubleT(second_latest));
}
