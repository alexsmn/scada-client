#include "events/severity_tiles.h"

#include <gtest/gtest.h>

#include <vector>

namespace events {
namespace {

using scada::aui::SeverityLevel;

AlarmSummary Alarm(SeverityLevel severity, bool acknowledged, bool active) {
  return {.severity = severity, .acknowledged = acknowledged, .active = active};
}

TEST(SeverityTilesTest, CountsBySeverity) {
  const std::vector<AlarmSummary> alarms{
      Alarm(SeverityLevel::kCritical, true, true),
      Alarm(SeverityLevel::kCritical, true, true),
      Alarm(SeverityLevel::kWarning, true, true),
  };

  const SeverityTileCounts counts = CountSeverityTiles(alarms);

  EXPECT_EQ(counts.critical, 2);
  EXPECT_EQ(counts.warning, 1);
  EXPECT_EQ(counts.unacknowledged, 0);
}

// Unacknowledged spans severities — it answers "what has nobody looked at",
// which is a different question from "how bad is it".
TEST(SeverityTilesTest, CountsUnacknowledgedAcrossSeverities) {
  const std::vector<AlarmSummary> alarms{
      Alarm(SeverityLevel::kCritical, false, true),
      Alarm(SeverityLevel::kWarning, false, true),
      Alarm(SeverityLevel::kNone, false, true),
      Alarm(SeverityLevel::kCritical, true, true),
  };

  const SeverityTileCounts counts = CountSeverityTiles(alarms);

  EXPECT_EQ(counts.unacknowledged, 3);
  EXPECT_EQ(counts.critical, 2);
  EXPECT_EQ(counts.warning, 1);
}

// A cleared alarm leaves the tiles even while still unread: the tiles report
// what is wrong now, not what has been read (ISA-18.2 separates alarm state
// from acknowledgement).
TEST(SeverityTilesTest, IgnoresInactiveAlarmsEntirely) {
  const std::vector<AlarmSummary> alarms{
      Alarm(SeverityLevel::kCritical, false, /*active=*/false),
      Alarm(SeverityLevel::kWarning, false, /*active=*/false),
  };

  EXPECT_EQ(CountSeverityTiles(alarms), SeverityTileCounts{});
}

// kNone is not a severity tile, but an unread one still needs looking at.
TEST(SeverityTilesTest, SeverityNoneCountsOnlyAsUnacknowledged) {
  const std::vector<AlarmSummary> alarms{
      Alarm(SeverityLevel::kNone, false, true)};

  const SeverityTileCounts counts = CountSeverityTiles(alarms);

  EXPECT_EQ(counts.critical, 0);
  EXPECT_EQ(counts.warning, 0);
  EXPECT_EQ(counts.unacknowledged, 1);
}

TEST(SeverityTilesTest, EmptyAlarmSetIsAllZero) {
  EXPECT_EQ(CountSeverityTiles({}), SeverityTileCounts{});
}

// Most severe first, so the eye lands on the worst state before the detail.
TEST(SeverityTilesTest, BuildsTilesMostSevereFirst) {
  const SeverityTileCounts counts{
      .critical = 3, .warning = 2, .unacknowledged = 5};

  const std::vector<SeverityTile> tiles =
      BuildSeverityTiles(counts, "Critical", "Warning", "Unacknowledged");

  ASSERT_EQ(tiles.size(), 3u);
  EXPECT_EQ(tiles[0].caption, "Critical");
  EXPECT_EQ(tiles[0].count, 3);
  EXPECT_EQ(tiles[1].caption, "Warning");
  EXPECT_EQ(tiles[1].count, 2);
  EXPECT_EQ(tiles[2].caption, "Unacknowledged");
  EXPECT_EQ(tiles[2].count, 5);
}

// Tile colours come from the severity single source (backlog 0.2), so a token
// change restyles them with every other severity surface. Unacknowledged is a
// workflow state and carries no severity colour.
TEST(SeverityTilesTest, TileColorsComeFromSeveritySingleSource) {
  const std::vector<SeverityTile> tiles =
      BuildSeverityTiles(SeverityTileCounts{}, "C", "W", "U");

  ASSERT_EQ(tiles.size(), 3u);
  EXPECT_EQ(tiles[0].color,
            scada::aui::SeverityColor(SeverityLevel::kCritical));
  EXPECT_EQ(tiles[1].color, scada::aui::SeverityColor(SeverityLevel::kWarning));
  EXPECT_FALSE(tiles[2].color.has_value());
}

}  // namespace
}  // namespace events
