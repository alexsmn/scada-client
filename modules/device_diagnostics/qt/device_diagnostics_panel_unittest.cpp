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
                        QStringLiteral("no response"), SampleRows(),
                        /*metrics_enabled=*/false);

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
                        DeviceLinkBand::kUp, QString{}, SampleRows(),
                        /*metrics_enabled=*/true);
  auto* status =
      panel.findChild<QLabel*>(QStringLiteral("diagnosticsHeroStatus"));
  ASSERT_NE(status, nullptr);
  EXPECT_EQ(status->text(), QStringLiteral("Link up"));
}

TEST_F(DeviceDiagnosticsPanelTest, MetricsActionTracksEnablement) {
  DeviceDiagnosticsPanel panel{DeviceDiagnosticsPanelContext{}};

  panel.ShowDiagnostics(QStringLiteral("RTU-02"), QString{}, DeviceLinkBand::kUp,
                        QString{}, {}, /*metrics_enabled=*/false);
  auto* metrics = panel.findChild<QPushButton*>();
  ASSERT_NE(metrics, nullptr);
  EXPECT_FALSE(metrics->isEnabled());

  panel.ShowDiagnostics(QStringLiteral("RTU-02"), QString{}, DeviceLinkBand::kUp,
                        QString{}, {}, /*metrics_enabled=*/true);
  EXPECT_TRUE(metrics->isEnabled());
}

TEST_F(DeviceDiagnosticsPanelTest, ClearReturnsToEmptyState) {
  DeviceDiagnosticsPanel panel{DeviceDiagnosticsPanelContext{}};
  panel.ShowDiagnostics(QStringLiteral("RTU-02"), QString{}, DeviceLinkBand::kUp,
                        QString{}, SampleRows(), /*metrics_enabled=*/true);
  panel.Clear();
  auto* stack = panel.findChild<QStackedWidget*>();
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 0);
}

}  // namespace
