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

// A cleared alarm leaves the severity tiles even while still unread: they
// report what is wrong now (ISA-18.2 separates alarm state from
// acknowledgement). It is still unread, so Unacknowledged keeps it — and a
// cleared-unread critical still annunciates (backlog 1117).
TEST(SeverityTilesTest, InactiveAlarmsCountOnlyAsUnacknowledged) {
  const std::vector<AlarmSummary> alarms{
      Alarm(SeverityLevel::kCritical, false, /*active=*/false),
      Alarm(SeverityLevel::kWarning, false, /*active=*/false),
  };

  EXPECT_EQ(
      CountSeverityTiles(alarms),
      (SeverityTileCounts{.unacknowledged = 2, .unacknowledged_critical = 1}));
}

// An acknowledged alarm still standing is still wrong: it stays in its
// severity tile, and leaves the unacknowledged ones (backlog 1117).
TEST(SeverityTilesTest, AcknowledgedActiveAlarmStaysInItsSeverityTile) {
  const std::vector<AlarmSummary> alarms{
      Alarm(SeverityLevel::kCritical, /*acknowledged=*/true, /*active=*/true)};

  EXPECT_EQ(CountSeverityTiles(alarms), (SeverityTileCounts{.critical = 1}));
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
      BuildSeverityTiles(counts, u"Critical", u"Warning", u"Unacknowledged");

  ASSERT_EQ(tiles.size(), 3u);
  EXPECT_EQ(tiles[0].caption, u"Critical");
  EXPECT_EQ(tiles[0].count, 3);
  EXPECT_EQ(tiles[1].caption, u"Warning");
  EXPECT_EQ(tiles[1].count, 2);
  EXPECT_EQ(tiles[2].caption, u"Unacknowledged");
  EXPECT_EQ(tiles[2].count, 5);
}

// Tile colours come from the severity single source (backlog 0.2), so a token
// change restyles them with every other severity surface. Unacknowledged is a
// workflow state and carries no severity colour.
TEST(SeverityTilesTest, TileColorsComeFromSeveritySingleSource) {
  const std::vector<SeverityTile> tiles =
      BuildSeverityTiles(SeverityTileCounts{}, u"C", u"W", u"U");

  ASSERT_EQ(tiles.size(), 3u);
  EXPECT_EQ(tiles[0].color,
            scada::aui::SeverityColor(SeverityLevel::kCritical));
  EXPECT_EQ(tiles[1].color, scada::aui::SeverityColor(SeverityLevel::kWarning));
  EXPECT_FALSE(tiles[2].color.has_value());
}

}  // namespace
}  // namespace events
