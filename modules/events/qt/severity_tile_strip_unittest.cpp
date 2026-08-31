#include "events/qt/severity_tile_strip.h"

#include "aui/test/app_environment.h"

#include <gtest/gtest.h>

#include <QLabel>

#include <memory>

namespace events {
namespace {

QString TileText(const SeverityTileStrip& strip, int index) {
  auto* tile =
      strip.findChild<QLabel*>(QStringLiteral("severityTile%1").arg(index));
  return tile ? tile->text() : QString{};
}

bool TileIsAsserted(const SeverityTileStrip& strip, int index) {
  auto* tile =
      strip.findChild<QLabel*>(QStringLiteral("severityTile%1").arg(index));
  return tile && tile->styleSheet().contains(QStringLiteral("font-weight:700"));
}

class SeverityTileStripTest : public ::testing::Test {
 protected:
  // Qt requires a QApplication before any QWidget; destroyed with the fixture.
  AppEnvironment app_env_;
};

TEST_F(SeverityTileStripTest, TheFactoryBuildsTheTiles) {
  std::unique_ptr<SeverityTileStrip> strip{MakeSeverityTileStrip([] {
    return SeverityTileCounts{.critical = 3, .warning = 2, .unacknowledged = 5};
  })};

  ASSERT_NE(strip, nullptr);
  // Display order is the builder's: most severe first, unacknowledged last.
  EXPECT_TRUE(TileText(*strip, 0).endsWith(QStringLiteral(" 3")));
  EXPECT_TRUE(TileText(*strip, 1).endsWith(QStringLiteral(" 2")));
  EXPECT_TRUE(TileText(*strip, 2).endsWith(QStringLiteral(" 5")));
  // Each tile names what its number means — colour is never the only signal.
  for (int i = 0; i < 3; ++i) {
    const QString caption =
        TileText(*strip, i).section(QChar{' '}, 0, -2).trimmed();
    EXPECT_FALSE(caption.isEmpty()) << i;
  }
}

// The acceptance line for backlog 2.3: counts update as alarms arrive and
// clear. The strip re-reads its provider on every Refresh().
TEST_F(SeverityTileStripTest, RefreshTracksLiveCounts) {
  SeverityTileCounts counts;
  std::unique_ptr<SeverityTileStrip> strip{
      MakeSeverityTileStrip([&counts] { return counts; })};
  ASSERT_NE(strip, nullptr);

  EXPECT_TRUE(TileText(*strip, 0).endsWith(QStringLiteral(" 0")));

  // An alarm arrives.
  counts = SeverityTileCounts{.critical = 1, .unacknowledged = 1};
  strip->Refresh();
  EXPECT_TRUE(TileText(*strip, 0).endsWith(QStringLiteral(" 1")));
  EXPECT_TRUE(TileText(*strip, 2).endsWith(QStringLiteral(" 1")));

  // It clears but stays unread: it leaves the severity tile and remains
  // unacknowledged (ISA-18.2 separates alarm state from acknowledgement).
  counts = SeverityTileCounts{.unacknowledged = 1};
  strip->Refresh();
  EXPECT_TRUE(TileText(*strip, 0).endsWith(QStringLiteral(" 0")));
  EXPECT_TRUE(TileText(*strip, 2).endsWith(QStringLiteral(" 1")));
}

// A zero count reads calm; a live count asserts itself. Quiet-when-normal is
// the High-Performance HMI rule the tiles exist to serve.
TEST_F(SeverityTileStripTest, ZeroCountsReadCalmAndLiveCountsAssert) {
  SeverityTileCounts counts;
  std::unique_ptr<SeverityTileStrip> strip{
      MakeSeverityTileStrip([&counts] { return counts; })};
  ASSERT_NE(strip, nullptr);

  EXPECT_FALSE(TileIsAsserted(*strip, 0));

  counts = SeverityTileCounts{.critical = 2, .unacknowledged = 2};
  strip->Refresh();
  EXPECT_TRUE(TileIsAsserted(*strip, 0));
  EXPECT_FALSE(TileIsAsserted(*strip, 1));  // no warnings: still calm
  EXPECT_TRUE(TileIsAsserted(*strip, 2));
}

}  // namespace
}  // namespace events
