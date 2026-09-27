#include "main_window/alarm_state_cluster_qt.h"

#include "aui/test/app_environment.h"
#include "base/blinker.h"
#include "base/test/scoped_mock_clock_override.h"
#include "events/alarm_flood.h"

#include <QLabel>

#include <chrono>
#include <memory>

#include <gtest/gtest.h>

namespace {

constexpr scada::Duration kFlashHalfPeriod = std::chrono::milliseconds{700};

class AlarmStateClusterTest : public ::testing::Test {
 protected:
  AlarmStateCluster& MakeCluster() {
    cluster_ = std::make_unique<AlarmStateCluster>([this] { return counts_; });
    return *cluster_;
  }

  // Per-test QApplication; see ActivityBarTest for why it is never static.
  AppEnvironment app_env_;
  events::SeverityTileCounts counts_;
  std::unique_ptr<AlarmStateCluster> cluster_;
};

// isVisible() is false for any widget whose window is not shown; what the
// cluster decides is whether the chip is hidden.
bool IsShownIn(const QLabel& chip) {
  return !chip.isHidden();
}

TEST_F(AlarmStateClusterTest, NoAlarmsLightsNeitherRung) {
  AlarmStateCluster& cluster = MakeCluster();

  EXPECT_FALSE(IsShownIn(*cluster.annunciator()));
  EXPECT_FALSE(IsShownIn(*cluster.flood_pill()));
}

// Rung 1 below the flood threshold: the only thing marking a critical alarm
// nobody has taken.
TEST_F(AlarmStateClusterTest, UnacknowledgedCriticalAnnunciatesWithoutFlood) {
  counts_ = {.critical = 1, .unacknowledged = 1};
  AlarmStateCluster& cluster = MakeCluster();

  EXPECT_TRUE(IsShownIn(*cluster.annunciator()));
  EXPECT_TRUE(cluster.annunciator()->text().contains(
      QStringLiteral("Unacknowledged critical")));
  EXPECT_FALSE(IsShownIn(*cluster.flood_pill()));
}

TEST_F(AlarmStateClusterTest, FloodPillStatesTheUnacknowledgedCount) {
  const int flood = events::kAlarmFloodThreshold + 1;
  counts_ = {.warning = flood, .unacknowledged = flood};
  AlarmStateCluster& cluster = MakeCluster();

  EXPECT_FALSE(IsShownIn(*cluster.annunciator()));
  EXPECT_TRUE(IsShownIn(*cluster.flood_pill()));
  EXPECT_TRUE(cluster.flood_pill()->text().contains(QString::number(flood)));
}

TEST_F(AlarmStateClusterTest, RefreshFollowsTheCountsBothWays) {
  AlarmStateCluster& cluster = MakeCluster();

  counts_ = {.critical = 2, .unacknowledged = 2};
  cluster.Refresh();
  EXPECT_TRUE(IsShownIn(*cluster.annunciator()));

  counts_ = {};
  cluster.Refresh();
  EXPECT_FALSE(IsShownIn(*cluster.annunciator()));
  EXPECT_FALSE(IsShownIn(*cluster.flood_pill()));
}

// V54: the flash phase is a function of the clock, including the first lit
// frame, so a frozen clock gives a capture one stable state and a moved clock
// moves the phase.
TEST_F(AlarmStateClusterTest, FlashPhaseComesFromTheClock) {
  scada::base::ScopedMockClockOverride clock;
  counts_ = {.critical = 1, .unacknowledged = 1};
  AlarmStateCluster& cluster = MakeCluster();

  const bool first = cluster.annunciator_flash_on();
  EXPECT_EQ(first, BlinkPhaseAt(scada::Now(), kFlashHalfPeriod));
  cluster.Refresh();
  EXPECT_EQ(cluster.annunciator_flash_on(), first);

  clock.Advance(kFlashHalfPeriod);
  cluster.Refresh();
  EXPECT_NE(cluster.annunciator_flash_on(), first);
}

}  // namespace
