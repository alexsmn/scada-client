#include "device_diagnostics/qt/device_diagnostics_panel.h"

#include "aui/test/app_environment.h"
#include "aui/translation.h"
#include "node_service/test/fake_node_service.h"
#include "timed_data/timed_data_service.h"

#include <gtest/gtest.h>

#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>

namespace {

std::vector<DeviceDiagnosticRow> SampleRows() {
  return {
      DeviceDiagnosticRow{QStringLiteral("Messages RX"),
                          QStringLiteral("128402"), /*bad=*/false},
      DeviceDiagnosticRow{QStringLiteral("Messages TX"),
                          QStringLiteral("64118"), /*bad=*/false},
  };
}

class DeviceDiagnosticsPanelTest : public ::testing::Test {
 protected:
  // Qt requires a QApplication before any QWidget; destroyed with the fixture.
  AppEnvironment app_env_;
};

TEST_F(DeviceDiagnosticsPanelTest, StartsInEmptyState) {
  DeviceDiagnosticsPanel panel{DeviceDiagnosticsPanelContext{}};
  auto* stack = panel.findChild<QStackedWidget*>();
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 0);
}

TEST_F(DeviceDiagnosticsPanelTest, ShowDiagnosticsFillsHeroAndRows) {
  DeviceDiagnosticsPanel panel{DeviceDiagnosticsPanelContext{}};

  panel.ShowDiagnostics(QStringLiteral("RTU-02"),
                        QStringLiteral("IEC 60870-5-104"), DeviceLinkBand::kDown,
                        QStringLiteral("no response"), SampleRows());

  auto* stack = panel.findChild<QStackedWidget*>();
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 1);

  auto* status =
      panel.findChild<QLabel*>(QStringLiteral("diagnosticsHeroStatus"));
  ASSERT_NE(status, nullptr);
  EXPECT_EQ(status->text(), QStringLiteral("Link down"));

  // Both counter rows plus the hero labels are present; assert the values show.
  const QList<QLabel*> labels = panel.findChildren<QLabel*>();
  bool has_rx_value = false;
  for (const QLabel* label : labels) {
    if (label->text() == QStringLiteral("128402"))
      has_rx_value = true;
  }
  EXPECT_TRUE(has_rx_value);
}

TEST_F(DeviceDiagnosticsPanelTest, UpBandShowsLinkUp) {
  DeviceDiagnosticsPanel panel{DeviceDiagnosticsPanelContext{}};
  panel.ShowDiagnostics(QStringLiteral("RTU-01"), QStringLiteral("Modbus"),
                        DeviceLinkBand::kUp, QString{}, SampleRows());
  auto* status =
      panel.findChild<QLabel*>(QStringLiteral("diagnosticsHeroStatus"));
  ASSERT_NE(status, nullptr);
  EXPECT_EQ(status->text(), QStringLiteral("Link up"));
}

TEST_F(DeviceDiagnosticsPanelTest, ActionsRenderTrackEnablementAndExecute) {
  bool clicked = false;
  bool enabled = false;
  DeviceDiagnosticsPanelContext context;
  context.actions.push_back(
      DiagnosticAction{.label = u"Reconnect",
                       .execute = [&clicked] { clicked = true; },
                       .is_enabled = [&enabled] { return enabled; }});
  // No is_enabled → always enabled.
  context.actions.push_back(DiagnosticAction{.label = u"Open log"});
  DeviceDiagnosticsPanel panel{std::move(context)};

  panel.ShowDiagnostics(QStringLiteral("RTU-02"), QString{}, DeviceLinkBand::kUp,
                        QString{}, {});

  QPushButton* reconnect = nullptr;
  QPushButton* open_log = nullptr;
  for (QPushButton* button : panel.findChildren<QPushButton*>()) {
    if (button->text() == QStringLiteral("Reconnect"))
      reconnect = button;
    if (button->text() == QStringLiteral("Open log"))
      open_log = button;
  }
  ASSERT_NE(reconnect, nullptr);
  ASSERT_NE(open_log, nullptr);

  EXPECT_FALSE(reconnect->isEnabled());  // is_enabled() == false
  EXPECT_TRUE(open_log->isEnabled());    // no gate

  enabled = true;
  panel.ShowDiagnostics(QStringLiteral("RTU-02"), QString{}, DeviceLinkBand::kUp,
                        QString{}, {});
  EXPECT_TRUE(reconnect->isEnabled());

  reconnect->click();
  EXPECT_TRUE(clicked);
}

// Mockup authoring rule 6: a disabled control states a reason an operator can
// act on. The reason appears only while the control is dead — a permanent
// caption under a working button is noise.
TEST_F(DeviceDiagnosticsPanelTest, DisabledActionShowsItsReason) {
  bool enabled = false;
  DeviceDiagnosticsPanelContext context;
  context.actions.push_back(DiagnosticAction{
      .label = u"Reconnect now",
      .execute = [] {},
      .is_enabled = [&enabled] { return enabled; },
      .disabled_reason = u"Your account cannot issue control commands"});
  DeviceDiagnosticsPanel panel{std::move(context)};

  panel.ShowDiagnostics(QStringLiteral("RTU-02"), QString{},
                        DeviceLinkBand::kUp, QString{}, {});

  const auto find_reason = [&panel]() -> QLabel* {
    for (QLabel* label : panel.findChildren<QLabel*>()) {
      if (label->text() ==
          QStringLiteral("Your account cannot issue control commands"))
        return label;
    }
    return nullptr;
  };

  // isVisibleTo, not isVisible: the panel is never shown in a widget test, so
  // isVisible() is false for every child regardless of the flag under test.
  QLabel* reason = find_reason();
  ASSERT_NE(reason, nullptr);
  EXPECT_TRUE(reason->isVisibleTo(&panel));

  enabled = true;
  panel.ShowDiagnostics(QStringLiteral("RTU-02"), QString{},
                        DeviceLinkBand::kUp, QString{}, {});
  EXPECT_FALSE(reason->isVisibleTo(&panel));
}

// The three ways a link action resolves to NO button rather than a dead one.
// "This build cannot call methods" and "this device has no link" are not
// reasons an operator can act on, so they are absences, not explanations.
TEST_F(DeviceDiagnosticsPanelTest, LinkActionIsOmittedWhenItCouldNotWork) {
  const ProtocolDiagnostics* protocol =
      ProtocolDiagnosticsFor("Iec60870DeviceType");
  ASSERT_TRUE(protocol);

  // No method-call path wired by the host.
  {
    DeviceDiagnosticsPanel panel{DeviceDiagnosticsPanelContext{}};
    EXPECT_FALSE(panel.MakeLinkAction(protocol->link_action, NodeRef{}));
  }

  // Call path wired, but the device has no link to act on.
  {
    DeviceDiagnosticsPanelContext context;
    context.call_link_method = [](const NodeRef&, const scada::NodeId&) {};
    DeviceDiagnosticsPanel panel{std::move(context)};
    EXPECT_FALSE(panel.MakeLinkAction(protocol->link_action, NodeRef{}));
  }

  // A protocol that declares no action contributes no button either.
  {
    DeviceDiagnosticsPanelContext context;
    context.call_link_method = [](const NodeRef&, const scada::NodeId&) {};
    DeviceDiagnosticsPanel panel{std::move(context)};
    EXPECT_FALSE(panel.MakeLinkAction(ProtocolLinkAction{}, NodeRef{}));
  }
}

TEST_F(DeviceDiagnosticsPanelTest, ClearReturnsToEmptyState) {
  DeviceDiagnosticsPanel panel{DeviceDiagnosticsPanelContext{}};
  panel.ShowDiagnostics(QStringLiteral("RTU-02"), QString{}, DeviceLinkBand::kUp,
                        QString{}, SampleRows());
  panel.Clear();
  auto* stack = panel.findChild<QStackedWidget*>();
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 0);
}

// The panel reads its link rows off the device's PARENT — whether it draws a
// link section at all is decided by comparing the parent type's browse name
// against the protocol's — and a selection makes the parent no more resident
// than the Inspector's limit bands. An unasked-for device therefore loses its
// link section and its Reconnect action, on a device that has a link. The panel
// asks its host to load the device now, and redraws when it lands.
//
// The load's arrival is modelled the way the address space actually behaves:
// the parent link is not resolvable until the load runs.
TEST_F(DeviceDiagnosticsPanelTest, LoadsTheDeviceBeforeReadingItsLink) {
  constexpr scada::NodeId kDevice{103, 12};
  constexpr scada::NodeId kDeviceType{331, 12};
  constexpr scada::NodeId kLink{50, 12};
  constexpr scada::NodeId kLinkType{332, 12};

  class NoTimedData : public TimedDataService {
   public:
    std::shared_ptr<TimedData> GetNodeTimedData(
        const scada::NodeId&,
        const scada::AggregateFilter&) override {
      return nullptr;
    }
    std::shared_ptr<TimedData> GetFormulaTimedData(
        std::string_view,
        const scada::AggregateFilter&) override {
      return nullptr;
    }
  } timed_data;

  FakeNodeService nodes;
  nodes.Add(scada::NodeState{.node_id = kDeviceType,
                             .node_class = scada::NodeClass::ObjectType,
                             .attributes = {.browse_name = scada::QualifiedName{
                                                "Iec60870DeviceType"}}});
  // Registered parentless: the state the device is in before anything fetched
  // it, in which the panel can find no link.
  nodes.Add(scada::NodeState{.node_id = kDevice,
                             .node_class = scada::NodeClass::Object,
                             .type_definition_id = kDeviceType,
                             .attributes = {.display_name = u"КП-01"}});

  DeviceDiagnosticsPanelContext context;
  context.call_link_method = [](const NodeRef&, const scada::NodeId&) {};
  NodeRef asked;
  std::function<void()> redraw;
  context.load = [&](const NodeRef& device, std::function<void()> done) {
    asked = device;
    redraw = std::move(done);
  };
  DeviceDiagnosticsPanel panel{std::move(context)};

  panel.ShowDevice(nodes.GetNode(kDevice), timed_data);

  EXPECT_EQ(asked.node_id(), kDevice)
      << "the panel read the device's link without asking for it first";
  ASSERT_TRUE(redraw);
  EXPECT_TRUE(panel.findChildren<QPushButton*>().empty())
      << "a link action appeared before the link was resolvable";

  // The load lands: the device turns out to hang under a protocol link.
  nodes.Add(scada::NodeState{
      .node_id = kLinkType,
      .node_class = scada::NodeClass::ObjectType,
      .attributes = {.browse_name = scada::QualifiedName{"Iec60870LinkType"}}});
  nodes.Add(scada::NodeState{.node_id = kLink,
                             .node_class = scada::NodeClass::Object,
                             .type_definition_id = kLinkType,
                             .attributes = {.display_name = u"Link"}});
  nodes.Add(scada::NodeState{.node_id = kDevice,
                             .node_class = scada::NodeClass::Object,
                             .type_definition_id = kDeviceType,
                             .parent_id = kLink,
                             .reference_type_id = scada::id::Organizes,
                             .attributes = {.display_name = u"КП-01"}});
  redraw();

  const auto buttons = panel.findChildren<QPushButton*>();
  ASSERT_EQ(buttons.size(), 1)
      << "the link action did not appear once the link resolved";
  EXPECT_EQ(buttons.front()->text(),
            QString::fromStdU16String(Translate("Reconnect now")));
}

// A reply for a device the operator has moved off must not repaint the rows.
TEST_F(DeviceDiagnosticsPanelTest, StaleLoadReplyIsIgnored) {
  constexpr scada::NodeId kFirst{103, 12};
  constexpr scada::NodeId kSecond{104, 12};
  constexpr scada::NodeId kDeviceType{331, 12};

  class NoTimedData : public TimedDataService {
   public:
    std::shared_ptr<TimedData> GetNodeTimedData(
        const scada::NodeId&,
        const scada::AggregateFilter&) override {
      return nullptr;
    }
    std::shared_ptr<TimedData> GetFormulaTimedData(
        std::string_view,
        const scada::AggregateFilter&) override {
      return nullptr;
    }
  } timed_data;

  FakeNodeService nodes;
  nodes.Add(scada::NodeState{.node_id = kDeviceType,
                             .node_class = scada::NodeClass::ObjectType});
  for (const scada::NodeId& id : {kFirst, kSecond}) {
    nodes.Add(scada::NodeState{.node_id = id,
                               .node_class = scada::NodeClass::Object,
                               .type_definition_id = kDeviceType,
                               .attributes = {.display_name = u"device"}});
  }

  DeviceDiagnosticsPanelContext context;
  int loads = 0;
  std::function<void()> first_redraw;
  context.load = [&](const NodeRef& device, std::function<void()> done) {
    ++loads;
    if (device.node_id() == kFirst)
      first_redraw = std::move(done);
  };
  DeviceDiagnosticsPanel panel{std::move(context)};

  panel.ShowDevice(nodes.GetNode(kFirst), timed_data);
  panel.ShowDevice(nodes.GetNode(kSecond), timed_data);
  EXPECT_EQ(loads, 2);

  ASSERT_TRUE(first_redraw);
  first_redraw();

  // No third load: a stale reply that re-entered ShowDevice would have asked
  // again for the first device.
  EXPECT_EQ(loads, 2);
}

}  // namespace
