#include "graph/graph_view.h"
#include "base/time/time_wire_codec.h"

#include "aui/severity_colors.h"
#include "aui/test/app_environment.h"
#include "base/async_completion.h"
#include "base/test/awaitable_test.h"
#include "controller/test/controller_environment.h"
#include "graph/metrix_data_source.h"
#include "graph/metrix_graph.h"
#include "graph/trend_pens.h"
#include "model/data_items_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/test/fake_node_service.h"
#include "resources/common_resources.h"
#include "scada/client.h"
#include "scada/history_service_mock.h"
#include "timed_data/timed_data_service_fake.h"

#include "base/debug_util.h"
#include "common/test/scoped_temp_dir.h"
#include "controller/window_info.h"
#include "graph/graph_component.h"
#include "graph/graph_empty_state.h"
#include "scada/co_result.h"

#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QSet>
#include <QWidget>

using namespace testing;

namespace {

constexpr scada::NodeId kTestNodeId{1, 1};

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

// Counts view-observer registrations, which is what a change of a line's
// history window costs the timed data (a range union, a trim and a gap fetch).
class CountingTimedData final : public FakeTimedData {
 public:
  using FakeTimedData::FakeTimedData;

  void AddViewObserver(TimedDataViewObserver& observer,
                       const scada::TimeRange& range) override {
    ++view_observer_registrations;
    FakeTimedData::AddViewObserver(observer, range);
  }

  int view_observer_registrations = 0;
};

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

// Backlog 874: the graph prints and saves as a picture. It used to have no
// WIN_CAN_PRINT and no way out of the window at all.
TEST_F(GraphViewTest, PrintsAndSavesAsAPicture) {
  EXPECT_TRUE(kGraphWindowInfo.printable());
  EXPECT_EQ(graph_view_.GetImageExportModel(),
            static_cast<ImageExportModel*>(&graph_view_));
  EXPECT_THAT(graph_view_.GetCommandHandler(ID_EXPORT_IMAGE), NotNull());
}

// The graph's values leave as a table too, for CSV and Excel export; they
// used to leave only as a picture.
TEST_F(GraphViewTest, ExportsItsValuesAsATable) {
  ASSERT_EQ(graph_view_.GetExportModel(),
            static_cast<ExportModel*>(&graph_view_));
  auto data = graph_view_.GetExportData();
  auto* table = std::get_if<ExportModel::TableExportData>(&data);
  ASSERT_THAT(table, NotNull());
  ASSERT_FALSE(table->columns.empty());
  EXPECT_EQ(table->columns[0].data_type,
            scada::aui::TableColumn::DataType::DateTime);
}

TEST_F(GraphViewTest, SaveImageWritesThePictureAtItsOnScreenSize) {
  scada::ScopedTempDir temp_dir{"scada_graph_image_test"};
  auto* widget = static_cast<QWidget*>(ui_view_.get());
  widget->resize(320, 200);
  ASSERT_EQ(graph_view_.GetImageSize(), QSize(320, 200));

  const auto path = temp_dir.path() / "graph.png";
  ASSERT_TRUE(graph_view_.SaveImage(path));

  QImage saved;
  ASSERT_TRUE(saved.load(QString::fromStdU16String(path.u16string())));
  EXPECT_EQ(saved.size(), QSize(320, 200));

  // Not a blank canvas: the axes and grid draw in more than one colour.
  QSet<QRgb> colours;
  for (int y = 0; y < saved.height(); y += 4)
    for (int x = 0; x < saved.width(); x += 4)
      colours.insert(saved.pixel(x, y));
  EXPECT_GT(colours.size(), 1);
}

// Backlog 1125: a graph that plots nothing says so and says how to fill it.
// The Overview's dominant trend used to be two thirds of the workspace drawing
// only a time axis.
namespace {

QWidget* FindEmptyState(QWidget& graph) {
  return graph.findChild<QWidget*>(QStringLiteral("graphEmptyState"));
}

// True when some pixel of |image| is dominated by |channel| (0 red, 1 green,
// 2 blue) — text drawn in that pure colour over black.
bool HasPixelDominatedBy(const QImage& image, int channel) {
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      const QColor c = image.pixelColor(x, y);
      const int v[] = {c.red(), c.green(), c.blue()};
      if (v[channel] > 128 && v[(channel + 1) % 3] < 40 &&
          v[(channel + 2) % 3] < 40)
        return true;
    }
  }
  return false;
}

}  // namespace

TEST_F(GraphViewTest, EmptyGraphSaysHowToFillIt) {
  auto& graph = *static_cast<QWidget*>(ui_view_.get());
  QWidget* empty_state = FindEmptyState(graph);
  ASSERT_THAT(empty_state, NotNull());
  EXPECT_TRUE(empty_state->isVisibleTo(&graph));
  EXPECT_FALSE(GraphEmptyState::TitleText().isEmpty());
  EXPECT_FALSE(GraphEmptyState::HintText().isEmpty());
  // It must not swallow the graph's own clicks and context menu.
  EXPECT_TRUE(empty_state->testAttribute(Qt::WA_TransparentForMouseEvents));
}

TEST_F(GraphViewTest, EmptyStateGoesWithTheFirstLineAndReturnsWithoutIt) {
  auto& graph = *static_cast<QWidget*>(ui_view_.get());
  QWidget* empty_state = FindEmptyState(graph);
  ASSERT_THAT(empty_state, NotNull());

  graph_view_.AddContainedItem(kTestNodeId, 0);
  EXPECT_FALSE(empty_state->isVisibleTo(&graph));

  // Deleting the pane takes its line with it. (RemoveContainedItem would match
  // by node id, which this fixture's formula lines do not resolve back to.)
  auto* handler = graph_view_.GetCommandHandler(ID_GRAPH_DELETE_PANE);
  ASSERT_THAT(handler, NotNull());
  handler->ExecuteCommand(ID_GRAPH_DELETE_PANE);
  EXPECT_TRUE(empty_state->isVisibleTo(&graph));
}

TEST_F(GraphViewTest, EmptyStateFollowsTheGraphsSize) {
  auto& graph = *static_cast<QWidget*>(ui_view_.get());
  graph.resize(400, 240);
  graph.show();
  QWidget* empty_state = FindEmptyState(graph);
  ASSERT_THAT(empty_state, NotNull());
  EXPECT_EQ(empty_state->geometry(), graph.rect());

  graph.resize(500, 300);
  EXPECT_EQ(empty_state->geometry(), graph.rect());
}

// Palette roles, not baked colours: the title takes WindowText and the hint
// PlaceholderText, so both follow the theme.
TEST_F(GraphViewTest, EmptyStatePaintsWithPaletteRoles) {
  auto& graph = *static_cast<QWidget*>(ui_view_.get());
  QWidget* empty_state = FindEmptyState(graph);
  ASSERT_THAT(empty_state, NotNull());
  empty_state->resize(600, 200);
  QPalette palette = empty_state->palette();
  palette.setColor(QPalette::WindowText, Qt::red);
  palette.setColor(QPalette::PlaceholderText, Qt::green);
  empty_state->setPalette(palette);

  QImage image{empty_state->size(), QImage::Format_ARGB32_Premultiplied};
  image.fill(Qt::black);
  {
    QPainter painter{&image};
    empty_state->render(&painter, {}, {}, QWidget::DrawChildren);
  }
  EXPECT_TRUE(HasPixelDominatedBy(image, /*red=*/0));
  EXPECT_TRUE(HasPixelDominatedBy(image, /*green=*/1));
}

// The hint is for the operator at the screen; a printed or saved picture of an
// empty graph shows the graph alone, and the hint is back afterwards.
TEST_F(GraphViewTest, PictureOfAnEmptyGraphLeavesTheHintOut) {
  auto& graph = *static_cast<QWidget*>(ui_view_.get());
  graph.resize(400, 240);
  graph.show();
  QWidget* empty_state = FindEmptyState(graph);
  ASSERT_THAT(empty_state, NotNull());

  auto render = [&] {
    QImage image{graph.size(), QImage::Format_ARGB32_Premultiplied};
    image.fill(Qt::transparent);
    QPainter painter{&image};
    graph_view_.RenderImage(painter);
    return image;
  };

  const QImage with_hint_shown = render();
  EXPECT_TRUE(empty_state->isVisibleTo(&graph));

  empty_state->hide();
  const QImage without_hint = render();
  empty_state->show();

  EXPECT_EQ(with_hint_shown, without_hint);
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
// series model may still be reporting. DeleteSelectedPane must re-derive from
// the now-current pane (it used to leave the in-tab inspector holding a
// dangling pointer that the next repaint dereferenced — a use-after-free).
// The series surface moved into the shell Inspector on 2026-08-30, so the
// guarantee is now stated on the model: after the delete there is no series,
// and reading one must not reach the freed line.
TEST(GraphViewSeriesModelTest, DeletingSelectedPaneClearsTheSeries) {
  AppEnvironment app_env;
  ControllerEnvironment env;

  GraphView view{env.MakeControllerContext()};
  WindowDefinition def;
  std::unique_ptr<UiView> ui = view.Init(def);

  view.AddContainedItem(kTestNodeId, 0);
  EXPECT_TRUE(view.HasSeries());

  // Delete the selected pane via its command (DeleteSelectedPane is private).
  CommandHandler* delete_handler = view.GetCommandHandler(ID_GRAPH_DELETE_PANE);
  ASSERT_THAT(delete_handler, NotNull());
  ASSERT_TRUE(delete_handler->IsCommandEnabled(ID_GRAPH_DELETE_PANE));
  delete_handler->ExecuteCommand(ID_GRAPH_DELETE_PANE);

  EXPECT_FALSE(view.HasSeries());
}

// The series model is what the shell Inspector renders, so the colour it
// reports has to be the line's — and a write through it has to reach the line
// and say so, since nothing else tells the Inspector to re-read.
TEST(GraphViewSeriesModelTest, SetSeriesColorRecoloursTheLineAndNotifies) {
  AppEnvironment app_env;
  ControllerEnvironment env;

  GraphView view{env.MakeControllerContext()};
  WindowDefinition def;
  std::unique_ptr<UiView> ui = view.Init(def);
  view.AddContainedItem(kTestNodeId, 0);
  ASSERT_TRUE(view.HasSeries());

  int notifications = 0;
  view.change_handler = [&notifications] { ++notifications; };

  const scada::aui::Color chosen{QColor{Qt::magenta}};
  view.SetSeriesColor(chosen);

  EXPECT_EQ(view.GetSeriesColor(), chosen);
  EXPECT_EQ(notifications, 1);
}

// Toggling a display flag through the view's own command changes what the
// Inspector should be showing, and no selection moves when it does — so the
// model has to announce it or the section goes stale. The starting state is
// read rather than assumed: a new line takes graph_qt's defaults (dots and
// stepped both on), and this test is about the flip, not about those.
TEST(GraphViewSeriesModelTest, DisplayFlagCommandsNotifyTheHost) {
  AppEnvironment app_env;
  ControllerEnvironment env;

  GraphView view{env.MakeControllerContext()};
  WindowDefinition def;
  std::unique_ptr<UiView> ui = view.Init(def);
  view.AddContainedItem(kTestNodeId, 0);
  ASSERT_TRUE(view.HasSeries());
  const bool dots_before = view.AreSeriesDotsShown();

  int notifications = 0;
  view.change_handler = [&notifications] { ++notifications; };

  CommandHandler* dots = view.GetCommandHandler(ID_GRAPH_DOTS);
  ASSERT_THAT(dots, NotNull());
  ASSERT_TRUE(dots->IsCommandEnabled(ID_GRAPH_DOTS));
  dots->ExecuteCommand(ID_GRAPH_DOTS);

  EXPECT_EQ(view.AreSeriesDotsShown(), !dots_before);
  EXPECT_EQ(notifications, 1);
  // One series, alone in its pane — which is what the section reports.
  EXPECT_TRUE(view.IsSeriesOnOwnPane());
}

// MetrixGraph polls every line's history window on a 50 ms timer. That is
// cheap only because a tick over an unchanged axis registers nothing; pin it,
// so a poll that started re-registering at 20 Hz per line would fail here.
TEST_F(GraphViewTest, UpdateDataOverAnUnchangedAxisDoesNotReregister) {
  FakeTimedDataService fake_service;
  const scada::Time now = scada::Now();

  auto timed_data = std::make_shared<CountingTimedData>("TS.201");
  timed_data->data_values.push_back(MakeDataValue(1.0, now));
  timed_data->ready_ranges.push_back({now - std::chrono::hours(1), now});
  fake_service.formulas["TS.201"] = timed_data;

  MetrixGraph graph{MetrixGraphContext{fake_service, env_.executor_}};
  auto& pane = graph.NewPane();
  graph.NewLine("TS.201", pane);

  graph.horizontal_axis().SetTimeFit(false);
  graph.horizontal_axis().SetRange(
      GraphRange{scada::base::EncodeDoubleT(now - std::chrono::hours(1)),
                 scada::base::EncodeDoubleT(now), GraphRange::TIME});
  graph.UpdateData();
  const int after_first = timed_data->view_observer_registrations;

  for (int i = 0; i < 20; ++i)
    graph.UpdateData();
  EXPECT_EQ(timed_data->view_observer_registrations, after_first);

  // A moved window is registered once, on the next tick.
  graph.horizontal_axis().SetRange(
      GraphRange{scada::base::EncodeDoubleT(now - std::chrono::hours(2)),
                 scada::base::EncodeDoubleT(now), GraphRange::TIME});
  graph.UpdateData();
  graph.UpdateData();
  EXPECT_EQ(timed_data->view_observer_registrations, after_first + 1);
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
  MetrixGraph graph{MetrixGraphContext{fake_service, env_.executor_}};
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

namespace {

// Returns the persisted canvas colour, or nullopt when the view did not write
// one — which is how "this chart follows the palette" is represented on disk.
std::optional<std::string> SavedCanvasColor(const WindowDefinition& def) {
  for (const auto& item : def.items) {
    if (!item.name_is("Graph"))
      continue;
    if (std::string_view color = item.GetString("bk_color"); !color.empty())
      return std::string{color};
  }
  return std::nullopt;
}

}  // namespace

// A canvas that merely follows QPalette::Base must not be persisted: the
// previous code wrote `bk_color` on every save, so the first save froze the
// view to whatever appearance it happened to be saved under.
TEST_F(GraphViewTest, SaveOmitsCanvasColorThatFollowsThePalette) {
  WindowDefinition def;
  graph_view_.Save(def);

  EXPECT_EQ(SavedCanvasColor(def), std::nullopt);
}

TEST_F(GraphViewTest, SavePersistsOperatorChosenCanvasColor) {
  graph_view_.SetGraphColor(QColor{20, 30, 40});

  WindowDefinition def;
  graph_view_.Save(def);

  ASSERT_TRUE(SavedCanvasColor(def).has_value());
  EXPECT_EQ(scada::aui::StringToColor(*SavedCanvasColor(def)).qcolor(),
            QColor(20, 30, 40));
}

// Legacy profiles carry a `bk_color` of white that nobody chose — it is the old
// hardwired canvas. Restoring it would pin every existing view to white on a
// dark desktop, so it is dropped and the palette decides.
TEST_F(GraphViewTest, LegacyWhiteCanvasColorIsNotRestored) {
  WindowDefinition loaded;
  loaded.AddItem("Graph").SetString(
      "bk_color", scada::aui::ColorToString(QColor{Qt::white}));
  GraphView view{env_.MakeControllerContext()};
  std::unique_ptr<UiView> ui_view = view.Init(loaded);

  WindowDefinition saved;
  view.Save(saved);

  EXPECT_EQ(SavedCanvasColor(saved), std::nullopt);
}

TEST_F(GraphViewTest, DeliberateCanvasColorSurvivesALoadSaveRoundTrip) {
  WindowDefinition loaded;
  loaded.AddItem("Graph").SetString(
      "bk_color", scada::aui::ColorToString(QColor{10, 12, 16}));
  GraphView view{env_.MakeControllerContext()};
  std::unique_ptr<UiView> ui_view = view.Init(loaded);

  WindowDefinition saved;
  view.Save(saved);

  ASSERT_TRUE(SavedCanvasColor(saved).has_value());
  EXPECT_EQ(scada::aui::StringToColor(*SavedCanvasColor(saved)).qcolor(),
            QColor(10, 12, 16));
}

TEST(MetrixDataSourceTest, AppliesEarliestTimestampFromHistoryRead) {
  TestExecutor executor;
  StrictMock<scada::MockHistoryService> history_service;
  scada::services services{.history_service = &history_service};
  FakeNodeService node_service{services};
  NodeRef node = node_service.Add(scada::NodeState{.node_id = kTestNodeId});

  const auto earliest = scada::base::DecodeDoubleT(100.0);
  const auto latest = scada::base::DecodeDoubleT(200.0);

  EXPECT_CALL(history_service, HistoryReadRaw(_))
      .WillOnce([&](scada::HistoryReadRawDetails details)
                    -> scada::CoStatusOr<scada::HistoryReadRawResult> {
        EXPECT_EQ(details.node_id, kTestNodeId);
        EXPECT_EQ(details.max_count, 1u);
        co_return scada::HistoryReadRawResult{
            .values = {MakeDataValue(1.0, earliest)}};
      });

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
  FakeNodeService node_service{services};
  NodeRef node = node_service.Add(scada::NodeState{.node_id = kTestNodeId});

  scada::base::AsyncCompletion first_completion{executor};
  scada::base::AsyncCompletion second_completion{executor};
  scada::HistoryReadRawResult first_result;
  scada::HistoryReadRawResult second_result;
  bool first_started = false;
  bool second_started = false;

  EXPECT_CALL(history_service, HistoryReadRaw(_))
      .WillOnce([&](scada::HistoryReadRawDetails details)
                    -> scada::CoStatusOr<scada::HistoryReadRawResult> {
        EXPECT_EQ(details.node_id, kTestNodeId);
        first_started = true;
        co_await first_completion.Wait();
        co_return first_result;
      })
      .WillOnce([&](scada::HistoryReadRawDetails details)
                    -> scada::CoStatusOr<scada::HistoryReadRawResult> {
        EXPECT_EQ(details.node_id, kTestNodeId);
        second_started = true;
        co_await second_completion.Wait();
        co_return second_result;
      });

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

// Ten panes is the limit, and a graph saved at the limit must reopen with
// every line. The loader used to check the cap before looking up the line's
// own (already read) pane, so the 10th pane's lines were dropped (backlog
// 926).
TEST_F(GraphViewTest, TenPanesKeepEveryLineOnReopen) {
  constexpr int kPaneCount = 10;
  WindowDefinition loaded;
  for (int i = 0; i < kPaneCount; ++i)
    loaded.AddItem("GraphPane").SetInt("ix", i);
  for (int i = 0; i < kPaneCount; ++i) {
    loaded.AddItem("Item").SetInt("pane", i).SetString(
        "path", NodeIdToScadaString(scada::NodeId(100 + i, 1)));
  }

  GraphView view{env_.MakeControllerContext()};
  std::unique_ptr<UiView> ui_view = view.Init(loaded);

  WindowDefinition saved;
  view.Save(saved);

  int lines = 0;
  for (const auto& item : saved.items) {
    if (item.name_is("Item"))
      ++lines;
  }
  EXPECT_EQ(lines, kPaneCount);
}

// Backlog 1124: the first series of a dark-theme trend was drawn in alarm red,
// because NewColor took the first readable entry of the colour-name palette
// (Black, White, Red, …). A new series now takes the categorical pen sequence.
TEST_F(GraphViewTest, NewColorOnDarkCanvasIsTheFirstTrendPenNotRed) {
  graph_view_.SetGraphColor(QColor{0x17, 0x17, 0x17});

  const QColor color = graph_view_.NewColor().qcolor();

  EXPECT_EQ(color, TrendPenSequence()[0]);
  EXPECT_NE(color, QColor(Qt::red));
}

TEST_F(GraphViewTest, SecondSeriesTakesTheSecondTrendPen) {
  graph_view_.SetGraphColor(QColor{0xff, 0xff, 0xff});

  graph_view_.AddContainedItem(kTestNodeId, 0);

  EXPECT_EQ(graph_view_.GetSeriesColor().qcolor(), TrendPenSequence()[0]);
  EXPECT_EQ(graph_view_.NewColor().qcolor(), TrendPenSequence()[1]);
}

// An operator's own default pen (set from the line setup dialog) is a choice,
// so it wins over the sequence even in a hue the sequence avoids.
TEST_F(GraphViewTest, NewColorHonoursTheOperatorsDefaultPen) {
  graph_view_.SetGraphColor(QColor{0x17, 0x17, 0x17});
  env_.profile_.graph_view.default_color = scada::aui::ColorCode::Red;

  EXPECT_EQ(graph_view_.NewColor(),
            scada::aui::Color{scada::aui::ColorCode::Red});
}

// A saved series keeps the colour it was saved with; only a series saved
// without one takes a default pen.
TEST_F(GraphViewTest, LoadKeepsASavedPenColour) {
  WindowDefinition def{"Graph"};
  def.AddItem("GraphPane").SetInt("ix", 1);
  def.AddItem("Item")
      .SetString("path", NodeIdToScadaString(kTestNodeId))
      .SetString("clr", "#FFFF0000")
      .SetInt("pane", 1);

  GraphView view{env_.MakeControllerContext()};
  std::unique_ptr<UiView> ui = view.Init(def);

  ASSERT_TRUE(view.HasSeries());
  EXPECT_EQ(view.GetSeriesColor(),
            scada::aui::Color{scada::aui::ColorCode::Red});
}

// Backlog 1124: the current-value badge on the axis was a solid badge in the
// pen colour — a red badge for a red pen. The trend's badge is neutral on
// either canvas, whatever the pen.
TEST_F(GraphViewTest, CurrentValueBadgeIsNeutral) {
  MetrixGraph graph{
      MetrixGraphContext{env_.timed_data_service_, env_.executor_}};
  auto& line = graph.NewLine(NodeIdToScadaString(kTestNodeId), graph.NewPane());
  line.SetColor(QColor{Qt::red});

  for (const QColor canvas : {QColor{0x17, 0x17, 0x17}, QColor{Qt::white}}) {
    graph.SetCanvasColor(canvas);
    const QColor fill = graph.GetCurrentValueFill(line);
    EXPECT_EQ(fill.red(), fill.green());
    EXPECT_EQ(fill.green(), fill.blue());
    EXPECT_NE(fill, canvas);
  }
  graph.DeleteAllPanes();
}

// Backlog 1124: the trend named no units anywhere. The series learns its
// node's engineering units, which the value grid prints after the name.
TEST(MetrixDataSourceTest, ReadsTheNodesEngineeringUnits) {
  TestExecutor executor;
  NiceMock<scada::MockHistoryService> history_service;
  scada::services services{.history_service = &history_service};
  FakeNodeService node_service{services};
  NodeRef node = node_service.Add(scada::NodeState{
      .node_id = kTestNodeId,
      .properties = {{scada::data_items::id::AnalogItemType_EngineeringUnits,
                      scada::LocalizedText{u"Hz"}}}});

  MetrixDataSource data_source{executor};
  data_source.SetTimedData(
      MakeTimedDataSpec(node, scada::base::DecodeDoubleT(200.0)));
  Drain(executor);

  EXPECT_EQ(data_source.units(), u"Hz");
}

// The vertical axis is a fixed 50 px, and an analog's default format appends
// its units: a tick read «-32 Мвар» and clipped to «-32 Мва». Axis labels are
// the number alone; the units are printed once, after the name in the grid.
TEST(MetrixDataSourceTest, AxisLabelCarriesNoUnits) {
  TestExecutor executor;
  NiceMock<scada::MockHistoryService> history_service;
  scada::services services{.history_service = &history_service};
  FakeNodeService node_service{services};
  NodeRef node = node_service.Add(scada::NodeState{
      .node_id = kTestNodeId,
      .node_class = scada::NodeClass::Variable,
      .type_definition_id = scada::data_items::id::AnalogItemType,
      .properties = {{scada::data_items::id::AnalogItemType_EngineeringUnits,
                      scada::LocalizedText{u"Hz"}}}});

  MetrixDataSource data_source{executor};
  data_source.SetTimedData(
      MakeTimedDataSpec(node, scada::base::DecodeDoubleT(200.0)));
  Drain(executor);

  // Precondition: this node's default format does carry the units, so the
  // assertion below is about the axis and not about a unitless node.
  ASSERT_NE(data_source.timed_data()
                .GetValueString(scada::Variant{50.0}, {})
                .find(u"Hz"),
            std::u16string::npos);
  EXPECT_EQ(data_source.GetYAxisLabel(50.0).toStdU16String().find(u"Hz"),
            std::u16string::npos);
}
