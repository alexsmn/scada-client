#include "inspector/qt/inspector_panel.h"

#include "aui/color.h"
#include "aui/test/app_environment.h"
#include "base/utf_convert.h"
#include "controller/selection_model.h"
#include "model/data_items_node_ids.h"
#include "node_service/test/fake_node_service.h"
#include "scada/data_value.h"
#include "scada/qualifier.h"
#include "timed_data/base_timed_data.h"
#include "timed_data/timed_data_service.h"
#include "timed_data/timed_data_spec.h"

#include <gtest/gtest.h>

#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>

#include <memory>
#include <optional>
#include <string>

namespace {

// The pure quality-band mapping needs no QApplication.
TEST(InspectorQualityBandTest, GoodQualifierIsGood) {
  EXPECT_EQ(InspectorQualityBandFor(scada::Qualifier{}),
            InspectorQualityBand::kGood);
}

TEST(InspectorQualityBandTest, BadQualifierIsBad) {
  const scada::Qualifier bad{scada::Qualifier::BAD};
  EXPECT_EQ(InspectorQualityBandFor(bad), InspectorQualityBand::kBad);
}

// Regression: a value that was never delivered carries a default-constructed
// Qualifier, which is zero and therefore "not BAD". Mapping that through the
// Qualifier alone reported Good, so a display element bound to a node the
// server does not have showed a good-quality pill next to an empty readout.
TEST(InspectorQualityBandTest, NeverDeliveredValueIsUnknown) {
  EXPECT_EQ(InspectorQualityBandFor(scada::DataValue{}),
            InspectorQualityBand::kUnknown);
}

TEST(InspectorQualityBandTest, DeliveredValueKeepsItsQualifierBand) {
  const scada::DataValue good{42.0, scada::Qualifier{}, scada::kNullTime,
                              scada::kNullTime};
  EXPECT_EQ(InspectorQualityBandFor(good), InspectorQualityBand::kGood);

  const scada::DataValue bad{42.0, scada::Qualifier{scada::Qualifier::BAD},
                             scada::kNullTime, scada::kNullTime};
  EXPECT_EQ(InspectorQualityBandFor(bad), InspectorQualityBand::kBad);
}

class InspectorPanelTest : public ::testing::Test {
 protected:
  // Qt requires a QApplication before any QWidget; destroyed with the fixture.
  AppEnvironment app_env_;
};

TEST_F(InspectorPanelTest, StartsInEmptyState) {
  InspectorPanel panel{InspectorPanelContext{}};
  auto* stack =
      panel.findChild<QStackedWidget*>(QStringLiteral("inspectorStack"));
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 0);
}

TEST_F(InspectorPanelTest, ShowElementFillsReadoutAndControl) {
  bool control_enabled = true;
  InspectorPanel panel{InspectorPanelContext{
      .is_control_enabled = [&] { return control_enabled; }}};

  panel.ShowElement(
      InspectorElementView{.title = QStringLiteral("Q1"),
                           .node_id_text = QStringLiteral("ns=2;s=North.Q1"),
                           .value_text = QStringLiteral("195.7 A"),
                           .quality = InspectorQualityBand::kGood,
                           .updated_text = QStringLiteral("21:53:58"),
                           .controllable = true});

  auto* stack =
      panel.findChild<QStackedWidget*>(QStringLiteral("inspectorStack"));
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 1);

  auto* value = panel.findChild<QLabel*>(QStringLiteral("inspectorValue"));
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->text(), QStringLiteral("195.7 A"));

  auto* control =
      panel.findChild<QPushButton*>(QStringLiteral("inspectorControl"));
  ASSERT_NE(control, nullptr);
  EXPECT_TRUE(control->isEnabled());
}

TEST_F(InspectorPanelTest, ControlDisabledWhenNotControllable) {
  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowElement(
      InspectorElementView{.title = QStringLiteral("Bus"),
                           .value_text = QStringLiteral("115 kV"),
                           .quality = InspectorQualityBand::kGood,
                           .updated_text = QStringLiteral("21:53"),
                           .controllable = false});
  auto* control =
      panel.findChild<QPushButton*>(QStringLiteral("inspectorControl"));
  ASSERT_NE(control, nullptr);
  EXPECT_FALSE(control->isEnabled());
}

TEST_F(InspectorPanelTest, ClearReturnsToEmptyState) {
  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowElement(InspectorElementView{.title = QStringLiteral("Q1"),
                                         .value_text = QStringLiteral("1")});
  panel.Clear();
  auto* stack =
      panel.findChild<QStackedWidget*>(QStringLiteral("inspectorStack"));
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 0);
}

// The limits block lists the configured bands and marks the breached one, so
// the operator can see which threshold a coloured value crossed.
TEST_F(InspectorPanelTest, LimitRowsMarkTheBreachedBand) {
  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowElement(InspectorElementView{
      .title = QStringLiteral("Ua"),
      .value_text = QStringLiteral("10.9"),
      .limits = {
          {.label = QStringLiteral("HiHi"), .value = QStringLiteral("11.5")},
          {.label = QStringLiteral("Hi"),
           .value = QStringLiteral("10.8"),
           .breached = true}}});

  auto* limits = panel.findChild<QWidget*>(QStringLiteral("inspectorLimits"));
  ASSERT_NE(limits, nullptr);
  EXPECT_FALSE(limits->isHidden());

  // The breached row is identified by its own object name, so the marking does
  // not depend on colour alone.
  auto breached =
      panel.findChildren<QLabel*>(QStringLiteral("inspectorLimitBreached"));
  ASSERT_EQ(breached.size(), 1);
  EXPECT_EQ(breached.front()->text(), QStringLiteral("10.8"));

  auto plain =
      panel.findChildren<QLabel*>(QStringLiteral("inspectorLimitValue"));
  ASSERT_EQ(plain.size(), 1);
  EXPECT_EQ(plain.front()->text(), QStringLiteral("11.5"));
}

// A node with no configured bands renders no limits block at all rather than
// an empty section.
TEST_F(InspectorPanelTest, NoLimitsHidesTheBlock) {
  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowElement(InspectorElementView{.title = QStringLiteral("Ua"),
                                         .value_text = QStringLiteral("10.9")});

  auto* limits = panel.findChild<QWidget*>(QStringLiteral("inspectorLimits"));
  ASSERT_NE(limits, nullptr);
  EXPECT_TRUE(limits->isHidden());
}

// The limit rows belong to the selected node, so switching to a node with
// fewer bands drops the stale rows instead of accumulating them.
// The series section belongs to a view that plots something. Every other view
// supplies no series, and the section is then absent rather than empty — an
// "Own pane: No" row about a selection that is not on a chart says nothing.
TEST_F(InspectorPanelTest, SeriesSectionIsAbsentWithoutASeries) {
  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowElement(InspectorElementView{.title = QStringLiteral("Ua")});

  auto* series = panel.findChild<QWidget*>(QStringLiteral("inspectorSeries"));
  ASSERT_NE(series, nullptr);
  EXPECT_TRUE(series->isHidden());
}

// What the Graph tab's own inspector used to draw down the right edge of the
// chart, before the two panels were folded into one (2026-08-30): the colour
// the series is plotted in, and its display flags.
TEST_F(InspectorPanelTest, SeriesSectionReportsThePresentation) {
  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowElement(InspectorElementView{.title = QStringLiteral("Ua")});
  // Red: a colour the series can actually be drawn in. Palette entry 0 is
  // Transparent, which the section deliberately does not offer.
  const QColor plotted = scada::aui::Color{scada::aui::ColorCode::Red}.qcolor();
  panel.ShowSeries(InspectorSeriesView{
      .color = plotted, .own_pane = true, .dots = false, .stepped = true});

  auto* series = panel.findChild<QWidget*>(QStringLiteral("inspectorSeries"));
  ASSERT_NE(series, nullptr);
  EXPECT_FALSE(series->isHidden());

  // A swatch per offerable palette colour, and the plotted one says in words
  // that it is the current one — a ring is not a signal a reader can see.
  const auto swatches =
      panel.findChildren<QPushButton*>(QStringLiteral("inspectorSeriesSwatch"));
  // Every palette colour but Transparent, which would hide the line.
  ASSERT_EQ(static_cast<std::size_t>(swatches.size()),
            scada::aui::GetColorCount() - 1);
  for (const QPushButton* swatch : swatches) {
    EXPECT_NE(swatch->property("seriesColor").value<QColor>().alpha(), 0);
  }
  int described = 0;
  for (const QPushButton* swatch : swatches) {
    if (!swatch->accessibleDescription().isEmpty()) {
      ++described;
      EXPECT_EQ(swatch->property("seriesColor").value<QColor>().rgba(),
                plotted.rgba());
    }
  }
  EXPECT_EQ(described, 1);
}

// The swatch is the panel's one writing field, and it writes through the host
// rather than through a view pointer of its own.
TEST_F(InspectorPanelTest, SeriesSwatchAsksTheHostToRecolour) {
  std::optional<QColor> chosen;
  InspectorPanel panel{InspectorPanelContext{
      .on_series_color_chosen = [&chosen](QColor color) { chosen = color; }}};
  panel.ShowElement(InspectorElementView{.title = QStringLiteral("Ua")});
  panel.ShowSeries(InspectorSeriesView{
      .color = scada::aui::Color{scada::aui::ColorCode::Red}.qcolor()});

  const auto swatches =
      panel.findChildren<QPushButton*>(QStringLiteral("inspectorSeriesSwatch"));
  ASSERT_GT(swatches.size(), 1);
  QPushButton* second = swatches[1];
  second->click();

  ASSERT_TRUE(chosen.has_value());
  EXPECT_EQ(chosen->rgb(),
            second->property("seriesColor").value<QColor>().rgb());
}

// The section describes a selection, not the panel: clearing must take it with
// the rest, or the next element card opens still reporting the chart's series.
TEST_F(InspectorPanelTest, ClearHidesTheSeriesSection) {
  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowElement(InspectorElementView{.title = QStringLiteral("Ua")});
  panel.ShowSeries(InspectorSeriesView{
      .color = scada::aui::Color{scada::aui::ColorCode::Red}.qcolor()});
  auto* series = panel.findChild<QWidget*>(QStringLiteral("inspectorSeries"));
  ASSERT_NE(series, nullptr);
  ASSERT_FALSE(series->isHidden());

  panel.Clear();

  EXPECT_TRUE(series->isHidden());
}

TEST_F(InspectorPanelTest, LimitRowsRebuildOnSelectionChange) {
  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowElement(InspectorElementView{
      .limits = {
          {.label = QStringLiteral("HiHi"), .value = QStringLiteral("11.5")},
          {.label = QStringLiteral("Hi"), .value = QStringLiteral("10.8")}}});
  EXPECT_EQ(
      panel.findChildren<QLabel*>(QStringLiteral("inspectorLimitValue")).size(),
      2);

  panel.ShowElement(
      InspectorElementView{.limits = {{.label = QStringLiteral("Lo"),
                                       .value = QStringLiteral("9.5")}}});
  auto rows =
      panel.findChildren<QLabel*>(QStringLiteral("inspectorLimitValue"));
  ASSERT_EQ(rows.size(), 1);
  EXPECT_EQ(rows.front()->text(), QStringLiteral("9.5"));
}

// A disabled Control button explains itself: a greyed control with no reason
// leaves the operator guessing whether the system is broken or they lack the
// right.
TEST_F(InspectorPanelTest, DisabledControlShowsItsReason) {
  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowElement(InspectorElementView{
      .title = QStringLiteral("Ua"),
      .controllable = false,
      .control_reason = QStringLiteral("no output channel")});

  auto* reason =
      panel.findChild<QLabel*>(QStringLiteral("inspectorControlReason"));
  ASSERT_NE(reason, nullptr);
  EXPECT_FALSE(reason->isHidden());
  EXPECT_EQ(reason->text(), QStringLiteral("no output channel"));
}

// The hint describes what pressing Control does. While control is blocked it
// would advertise an action the operator cannot take, directly above the
// sentence explaining why — so it goes away with the action.
TEST_F(InspectorPanelTest, DisabledControlHidesItsHint) {
  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowElement(InspectorElementView{
      .title = QStringLiteral("Ua"),
      .controllable = false,
      .control_reason = QStringLiteral("no output channel")});

  auto* hint = panel.findChild<QLabel*>(QStringLiteral("inspectorControlHint"));
  ASSERT_NE(hint, nullptr);
  EXPECT_TRUE(hint->isHidden());
}

// An available control needs no explanation.
TEST_F(InspectorPanelTest, EnabledControlHidesTheReason) {
  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowElement(InspectorElementView{
      .title = QStringLiteral("Ua"),
      .controllable = true,
      .control_reason = QStringLiteral("ignored while controllable")});

  auto* reason =
      panel.findChild<QLabel*>(QStringLiteral("inspectorControlReason"));
  ASSERT_NE(reason, nullptr);
  EXPECT_TRUE(reason->isHidden());
}

// The alarm card's History block lists the event's lifecycle, and a step that
// has not happened yet shows an em dash where its time would be so the column
// stays aligned and the gap reads as "not yet".
TEST_F(InspectorPanelTest, EventTimelineListsTheLifecycle) {
  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowEvent(InspectorEventView{
      .source = QStringLiteral("КП-03"),
      .timeline = {{.time = QStringLiteral("14:17:00"),
                    .text = QStringLiteral("Raised")},
                   {.time = QString{},
                    .text = QStringLiteral("Awaiting acknowledgement")}}});

  auto* timeline =
      panel.findChild<QWidget*>(QStringLiteral("inspectorTimeline"));
  ASSERT_NE(timeline, nullptr);
  EXPECT_FALSE(timeline->isHidden());

  auto times =
      panel.findChildren<QLabel*>(QStringLiteral("inspectorTimelineTime"));
  ASSERT_EQ(times.size(), 2);
  EXPECT_EQ(times[0]->text(), QStringLiteral("14:17:00"));
  EXPECT_EQ(times[1]->text(), QStringLiteral("—"));
}

// An event card without lifecycle steps renders no History block rather than
// an empty section.
TEST_F(InspectorPanelTest, EmptyEventTimelineHidesTheBlock) {
  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowEvent(InspectorEventView{.source = QStringLiteral("КП-03")});

  auto* timeline =
      panel.findChild<QWidget*>(QStringLiteral("inspectorTimeline"));
  ASSERT_NE(timeline, nullptr);
  EXPECT_TRUE(timeline->isHidden());
}

// A minimal live datum identified by its formula alone (no backing node) —
// the shape of a Table expression row.
class FormulaTimedData : public BaseTimedData {
 public:
  explicit FormulaTimedData(std::string formula)
      : formula_{std::move(formula)} {}

  virtual std::string GetFormula(bool aliases) const override {
    return formula_;
  }
  virtual scada::LocalizedText GetTitle() const override {
    return UtfConvert<char16_t>(formula_);
  }

 private:
  const std::string formula_;
};

class FormulaTimedDataService : public TimedDataService {
 public:
  virtual std::shared_ptr<TimedData> GetNodeTimedData(
      const scada::NodeId& node_id,
      const scada::AggregateFilter& aggregation) override {
    return nullptr;
  }
  virtual std::shared_ptr<TimedData> GetFormulaTimedData(
      std::string_view formula,
      const scada::AggregateFilter& aggregation) override {
    return std::make_shared<FormulaTimedData>(std::string{formula});
  }
};

// A Table expression row selects a node-less spec that still carries a live
// value; the inspector shows it (regression: it used to clear on any null
// node id).
TEST_F(InspectorPanelTest, FormulaRowSelectionFillsTheInspector) {
  FormulaTimedDataService service;
  SelectionModel selection{{service}};
  TimedDataSpec spec{service, "{TIT.200}+{TIT.201}"};
  selection.SelectTimedData(spec);

  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowSelection(selection);

  auto* stack =
      panel.findChild<QStackedWidget*>(QStringLiteral("inspectorStack"));
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 1);

  auto* subtitle =
      panel.findChild<QLabel*>(QStringLiteral("inspectorSubtitle"));
  ASSERT_NE(subtitle, nullptr);
  EXPECT_EQ(subtitle->text(), QStringLiteral("{TIT.200}+{TIT.201}"));
}

// A node-backed live datum — the shape of an Explorer selection, where the
// readout ticks over a real node and the card's limit bands are that node's
// property children.
class NodeTimedData : public BaseTimedData {
 public:
  explicit NodeTimedData(NodeRef node) : node_{std::move(node)} {}

  virtual NodeRef GetNode() const override { return node_; }
  virtual std::string GetFormula(bool aliases) const override {
    return node_.node_id().ToString();
  }
  virtual scada::LocalizedText GetTitle() const override {
    return node_.display_name();
  }

 private:
  const NodeRef node_;
};

class NodeTimedDataService : public TimedDataService {
 public:
  explicit NodeTimedDataService(NodeService& nodes) : nodes_{nodes} {}

  virtual std::shared_ptr<TimedData> GetNodeTimedData(
      const scada::NodeId& node_id,
      const scada::AggregateFilter& aggregation) override {
    return std::make_shared<NodeTimedData>(nodes_.GetNode(node_id));
  }
  virtual std::shared_ptr<TimedData> GetFormulaTimedData(
      std::string_view formula,
      const scada::AggregateFilter& aggregation) override {
    return nullptr;
  }

 private:
  NodeService& nodes_;
};

// Regression: the limit bands are the selected node's property children, and
// nothing a selection does makes them resident — TimedData fetches the node
// alone. The panel read them straight through, got four empty Variants, and
// concluded the node configured no limits, so the Measurements block was hidden
// for every node an operator selected; the only cards that ever showed it were
// the ones a test or a capture filled by hand. The panel now asks its host to
// load them and redraws when they land.
TEST_F(InspectorPanelTest, LimitsLoadForANodeSelection) {
  constexpr scada::NodeId kItem{700, 12};
  constexpr scada::NodeId kItemType{800, 12};

  FakeNodeService nodes;
  nodes.Add(scada::NodeState{.node_id = kItemType,
                             .node_class = scada::NodeClass::VariableType});
  // Registered without its bands: what the panel can read before the fetch.
  nodes.Add(scada::NodeState{.node_id = kItem,
                             .node_class = scada::NodeClass::Variable,
                             .type_definition_id = kItemType,
                             .attributes = {.display_name = u"Ua"}});

  NodeTimedDataService service{nodes};
  SelectionModel selection{{service}};

  NodeRef asked;
  std::function<void()> redraw;
  InspectorPanel panel{InspectorPanelContext{
      .load_limits = [&](const NodeRef& node, std::function<void()> done) {
        asked = node;
        redraw = std::move(done);
      }}};

  selection.SelectNode(nodes.GetNode(kItem));
  panel.ShowSelection(selection);

  auto* limits = panel.findChild<QWidget*>(QStringLiteral("inspectorLimits"));
  ASSERT_NE(limits, nullptr);
  EXPECT_TRUE(limits->isHidden());
  EXPECT_EQ(asked.node_id(), kItem);
  ASSERT_TRUE(redraw);

  // The fetch lands: the bands become readable, and the host redraws.
  nodes.Add(scada::NodeState{
      .node_id = kItem,
      .node_class = scada::NodeClass::Variable,
      .type_definition_id = kItemType,
      .attributes = {.display_name = u"Ua"},
      .properties = {{scada::data_items::id::AnalogItemType_LimitHiHi, 11.5},
                     {scada::data_items::id::AnalogItemType_LimitHi, 10.8},
                     {scada::data_items::id::AnalogItemType_LimitLo, 9.5},
                     {scada::data_items::id::AnalogItemType_LimitLoLo, 9.0}}});
  redraw();

  EXPECT_FALSE(limits->isHidden());
  EXPECT_EQ(
      panel.findChildren<QLabel*>(QStringLiteral("inspectorLimitValue"))
              .size() +
          panel.findChildren<QLabel*>(QStringLiteral("inspectorLimitBreached"))
              .size(),
      4);
}

// A reply that arrives after the operator has moved on must not repaint the
// card with the previous signal's bands.
TEST_F(InspectorPanelTest, StaleLimitsReplyIsIgnored) {
  constexpr scada::NodeId kFirst{700, 12};
  constexpr scada::NodeId kSecond{701, 12};
  constexpr scada::NodeId kItemType{800, 12};

  FakeNodeService nodes;
  nodes.Add(scada::NodeState{.node_id = kItemType,
                             .node_class = scada::NodeClass::VariableType});
  nodes.Add(scada::NodeState{.node_id = kFirst,
                             .node_class = scada::NodeClass::Variable,
                             .type_definition_id = kItemType,
                             .attributes = {.display_name = u"Ua"}});
  nodes.Add(scada::NodeState{.node_id = kSecond,
                             .node_class = scada::NodeClass::Variable,
                             .type_definition_id = kItemType,
                             .attributes = {.display_name = u"Ub"}});

  NodeTimedDataService service{nodes};
  SelectionModel selection{{service}};

  std::function<void()> first_redraw;
  InspectorPanel panel{InspectorPanelContext{
      .load_limits = [&](const NodeRef& node, std::function<void()> done) {
        if (node.node_id() == kFirst)
          first_redraw = std::move(done);
      }}};

  selection.SelectNode(nodes.GetNode(kFirst));
  panel.ShowSelection(selection);
  selection.SelectNode(nodes.GetNode(kSecond));
  panel.ShowSelection(selection);

  // The first node's bands arrive late.
  nodes.Add(scada::NodeState{
      .node_id = kFirst,
      .node_class = scada::NodeClass::Variable,
      .type_definition_id = kItemType,
      .attributes = {.display_name = u"Ua"},
      .properties = {{scada::data_items::id::AnalogItemType_LimitHi, 10.8}}});
  ASSERT_TRUE(first_redraw);
  first_redraw();

  auto* limits = panel.findChild<QWidget*>(QStringLiteral("inspectorLimits"));
  ASSERT_NE(limits, nullptr);
  EXPECT_TRUE(limits->isHidden());
  auto* title = panel.findChild<QLabel*>(QStringLiteral("inspectorSubtitle"));
  ASSERT_NE(title, nullptr);
  EXPECT_EQ(title->text(), QString::fromStdString(kSecond.ToString()));
}

// A journal-event selection shows the alarm card: the severity band pill
// (named + numbered), the message, the pending acknowledgement state, and a
// live Acknowledge action.
TEST_F(InspectorPanelTest, EventSelectionShowsTheAlarmCard) {
  FormulaTimedDataService service;
  SelectionModel selection{{service}};
  scada::Event event;
  event.event_id = 5;
  event.severity = scada::kSeverityCritical;
  event.source_node_id = scada::NodeId{7, 3};
  event.message = u"comms lost";
  selection.SelectEvent(event, NodeRef{});

  bool acknowledged = false;
  InspectorPanel panel{
      InspectorPanelContext{.on_acknowledge = [&] { acknowledged = true; },
                            .is_acknowledge_enabled = [] { return true; }}};
  panel.ShowSelection(selection);

  auto* stack =
      panel.findChild<QStackedWidget*>(QStringLiteral("inspectorStack"));
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 2);

  auto* severity =
      panel.findChild<QLabel*>(QStringLiteral("inspectorEventSeverity"));
  ASSERT_NE(severity, nullptr);
  EXPECT_TRUE(severity->text().contains(QStringLiteral("Critical")));
  EXPECT_TRUE(
      severity->text().contains(QString::number(scada::kSeverityCritical)));

  auto* message =
      panel.findChild<QLabel*>(QStringLiteral("inspectorEventMessage"));
  ASSERT_NE(message, nullptr);
  EXPECT_EQ(message->text(), QStringLiteral("comms lost"));

  auto* pending =
      panel.findChild<QLabel*>(QStringLiteral("inspectorEventAcknowledged"));
  ASSERT_NE(pending, nullptr);
  EXPECT_TRUE(pending->text().contains(QStringLiteral("pending")));

  auto* acknowledge =
      panel.findChild<QPushButton*>(QStringLiteral("inspectorAcknowledge"));
  ASSERT_NE(acknowledge, nullptr);
  EXPECT_TRUE(acknowledge->isEnabled());
  acknowledge->click();
  EXPECT_TRUE(acknowledged);
}

// An already-acknowledged event shows its acknowledgement time and offers no
// action.
TEST_F(InspectorPanelTest, AcknowledgedEventDisablesTheAction) {
  FormulaTimedDataService service;
  SelectionModel selection{{service}};
  scada::Event event;
  event.event_id = 6;
  event.source_node_id = scada::NodeId{7, 3};
  event.message = u"restored";
  event.acked = true;
  event.acknowledged_time = scada::Now();
  selection.SelectEvent(event, NodeRef{});

  InspectorPanel panel{
      InspectorPanelContext{.is_acknowledge_enabled = [] { return true; }}};
  panel.ShowSelection(selection);

  auto* acknowledged_label =
      panel.findChild<QLabel*>(QStringLiteral("inspectorEventAcknowledged"));
  ASSERT_NE(acknowledged_label, nullptr);
  EXPECT_FALSE(acknowledged_label->text().contains(QStringLiteral("pending")));
  EXPECT_NE(acknowledged_label->text(), QStringLiteral("—"));

  auto* acknowledge =
      panel.findChild<QPushButton*>(QStringLiteral("inspectorAcknowledge"));
  ASSERT_NE(acknowledge, nullptr);
  EXPECT_FALSE(acknowledge->isEnabled());
}

// The event card's To-graph action opens the alarm's source through the
// wired command when the shell reports it available.
TEST_F(InspectorPanelTest, GoToSourceFiresTheWiredCommand) {
  FormulaTimedDataService service;
  SelectionModel selection{{service}};
  scada::Event event;
  event.event_id = 7;
  event.source_node_id = scada::NodeId{7, 3};
  event.message = u"comms lost";
  selection.SelectEvent(event, NodeRef{});

  bool opened = false;
  InspectorPanel panel{
      InspectorPanelContext{.on_go_to_source = [&] { opened = true; },
                            .is_go_to_source_enabled = [] { return true; }}};
  panel.ShowSelection(selection);

  auto* go_to_source =
      panel.findChild<QPushButton*>(QStringLiteral("inspectorGoToSource"));
  ASSERT_NE(go_to_source, nullptr);
  EXPECT_TRUE(go_to_source->isEnabled());
  go_to_source->click();
  EXPECT_TRUE(opened);
}

// Without an available source command (no wiring, or the graph command
// rejects the selection) the To-graph action stays disabled.
TEST_F(InspectorPanelTest, GoToSourceDisabledWhenUnavailable) {
  FormulaTimedDataService service;
  SelectionModel selection{{service}};
  scada::Event event;
  event.event_id = 8;
  event.source_node_id = scada::NodeId{7, 3};
  event.message = u"comms lost";
  selection.SelectEvent(event, NodeRef{});

  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowSelection(selection);

  auto* go_to_source =
      panel.findChild<QPushButton*>(QStringLiteral("inspectorGoToSource"));
  ASSERT_NE(go_to_source, nullptr);
  EXPECT_FALSE(go_to_source->isEnabled());
}

// A spec with neither node nor formula (a folder/object selection) still
// clears to the empty state.
TEST_F(InspectorPanelTest, DataLessSelectionShowsTheEmptyState) {
  FormulaTimedDataService service;
  SelectionModel selection{{service}};
  selection.SelectTimedData(TimedDataSpec{});

  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowSelection(selection);

  auto* stack =
      panel.findChild<QStackedWidget*>(QStringLiteral("inspectorStack"));
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 0);
}

}  // namespace
