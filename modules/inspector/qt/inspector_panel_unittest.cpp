#include "inspector/qt/inspector_panel.h"

#include "aui/test/app_environment.h"
#include "base/utf_convert.h"
#include "controller/selection_model.h"
#include "scada/qualifier.h"
#include "timed_data/base_timed_data.h"
#include "timed_data/timed_data_service.h"
#include "timed_data/timed_data_spec.h"

#include <gtest/gtest.h>

#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>

#include <memory>
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
  event.acknowledged_time = scada::DateTime::Now();
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
