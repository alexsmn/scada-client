#include "device_diagnostics/qt/device_diagnostics_panel.h"

#include "aui/test/app_environment.h"

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

TEST_F(DeviceDiagnosticsPanelTest, ClearReturnsToEmptyState) {
  DeviceDiagnosticsPanel panel{DeviceDiagnosticsPanelContext{}};
  panel.ShowDiagnostics(QStringLiteral("RTU-02"), QString{}, DeviceLinkBand::kUp,
                        QString{}, SampleRows());
  panel.Clear();
  auto* stack = panel.findChild<QStackedWidget*>();
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 0);
}

}  // namespace
