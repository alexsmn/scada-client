#include "display_frame/qt/display_frame.h"

#include "aui/test/app_environment.h"
#include "base/time/time.h"
#include "events/node_event_provider.h"
#include "scada/date_time.h"
#include "scada/event.h"
#include "scada/session_service_mock.h"

#include <boost/signals2/signal.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <QLabel>
#include <QString>
#include <QStringList>
#include <QTableWidget>
#include <QWidget>

#include <string>
#include <utility>

namespace {

// Minimal NodeEventProvider fake: serves a fixed set of unacknowledged events
// so the Recent-events strip has rows to render. Every other operation is a
// no-op.
class FakeNodeEventProvider : public NodeEventProvider {
 public:
  EventContainer events;

  scada::EventSeverity severity_min() const override {
    return scada::kSeverityMin;
  }
  void SetSeverityMin(scada::EventSeverity) override {}
  const EventContainer& unacked_events() const override { return events; }
  const EventSet* GetItemUnackedEvents(const scada::NodeId&) const override {
    return nullptr;
  }
  void AcknowledgeEvent(scada::EventId) override {}
  void AcknowledgeItemEvents(const scada::NodeId&) override {}
  void AcknowledgeAllEvents() override {}
  bool IsAcking() const override { return false; }
  bool IsAlerting(const scada::NodeId&) const override { return false; }
  void AddObserver(EventObserver&) override {}
  void RemoveObserver(EventObserver&) override {}
  void AddItemObserver(const scada::NodeId&, EventObserver&) override {}
  void RemoveItemObserver(const scada::NodeId&, EventObserver&) override {}
};

scada::Event MakeEvent(scada::EventId id,
                       unsigned severity,
                       std::u16string message) {
  scada::Event event;
  event.event_id = id;
  event.severity = severity;
  event.time = scada::Now();
  event.message = std::move(message);
  return event;
}

class DisplayFrameStripsTest : public ::testing::Test {
 protected:
  // Qt requires a QApplication before any QWidget; destroyed with the fixture.
  AppEnvironment app_env_;
};

TEST_F(DisplayFrameStripsTest, RecentEventsStripRendersProviderEvents) {
  FakeNodeEventProvider provider;
  provider.events.emplace(
      1, MakeEvent(1, scada::kSeverityCritical, u"Overvoltage"));
  provider.events.emplace(
      2, MakeEvent(2, scada::kSeverityWarning, u"Comms degraded"));

  DisplayFrame frame(/*diagram=*/nullptr, QStringLiteral("Bay 1"),
                     DisplayFrameContext{.node_event_provider = &provider});

  // Only the events panel exists (no timed-data service), so the single table
  // is the Recent-events strip: Severity | Time | Object | Message.
  auto* table = frame.findChild<QTableWidget*>();
  ASSERT_NE(table, nullptr);
  EXPECT_EQ(table->rowCount(), 2);
  EXPECT_EQ(table->columnCount(), 4);

  QStringList messages;
  for (int row = 0; row < table->rowCount(); ++row)
    messages << table->item(row, 3)->text();
  EXPECT_TRUE(messages.contains(QStringLiteral("Overvoltage")));
  EXPECT_TRUE(messages.contains(QStringLiteral("Comms degraded")));
}

// Backlog 911: with the display's objects known, the strip lists only the
// events they raised -- an alarm elsewhere in the system belongs to the event
// panel, not to this schematic.
TEST_F(DisplayFrameStripsTest, RecentEventsListOnlyTheDisplaysObjects) {
  const scada::NodeId on_display{101, 1};
  const scada::NodeId elsewhere{202, 1};
  FakeNodeEventProvider provider;
  scada::Event mine = MakeEvent(1, scada::kSeverityCritical, u"Overvoltage");
  mine.source_node_id = on_display;
  scada::Event other = MakeEvent(2, scada::kSeverityCritical, u"Far away");
  other.source_node_id = elsewhere;
  provider.events.emplace(1, mine);
  provider.events.emplace(2, other);

  DisplayFrame frame(
      /*diagram=*/nullptr, QStringLiteral("Bay 1"),
      DisplayFrameContext{.node_event_provider = &provider,
                          .event_sources = std::vector{on_display}});

  auto* table = frame.findChild<QTableWidget*>();
  ASSERT_NE(table, nullptr);
  ASSERT_EQ(table->rowCount(), 1);
  EXPECT_EQ(table->item(0, 3)->text(), QStringLiteral("Overvoltage"));
}

// A display that shows no object has no events of its own.
TEST_F(DisplayFrameStripsTest, RecentEventsAreEmptyForADisplayWithNoObjects) {
  FakeNodeEventProvider provider;
  provider.events.emplace(
      1, MakeEvent(1, scada::kSeverityCritical, u"Overvoltage"));

  DisplayFrame frame(
      /*diagram=*/nullptr, QStringLiteral("Bay 1"),
      DisplayFrameContext{.node_event_provider = &provider,
                          .event_sources = std::vector<scada::NodeId>{}});

  auto* table = frame.findChild<QTableWidget*>();
  ASSERT_NE(table, nullptr);
  EXPECT_EQ(table->rowCount(), 0);
}

TEST_F(DisplayFrameStripsTest, NoBayStripsWithoutDataSources) {
  DisplayFrame frame(/*diagram=*/nullptr, QStringLiteral("Bay 1"),
                     DisplayFrameContext{});
  EXPECT_EQ(frame.findChild<QTableWidget*>(), nullptr);
}

// The equipment-state legend the mockup asks for: four entries, present
// whether or not the frame has live-data sources, because it explains the
// diagram rather than the bay strips.
TEST_F(DisplayFrameStripsTest, LegendCarriesTheFourEquipmentStates) {
  DisplayFrame frame(/*diagram=*/nullptr, QStringLiteral("Bay 1"),
                     DisplayFrameContext{});

  auto* legend = frame.findChild<QWidget*>(QStringLiteral("displayLegend"));
  ASSERT_NE(legend, nullptr);

  // Each entry is a swatch label plus a text label; only the text labels carry
  // any, so collecting the non-empty ones counts the entries.
  QStringList captions;
  for (QLabel* label : legend->findChildren<QLabel*>()) {
    if (!label->text().isEmpty())
      captions << label->text();
  }
  EXPECT_EQ(captions.size(), 4);
}

}  // namespace

// The Live pill follows the session. It was a fixed label that stayed green
// while the client was disconnected (backlog 911).
TEST_F(DisplayFrameStripsTest, LivePillReadsOfflineWhileDisconnected) {
  testing::NiceMock<scada::MockSessionService> session;
  ON_CALL(session, IsConnected(testing::_))
      .WillByDefault(testing::Return(false));

  DisplayFrame frame(/*diagram=*/nullptr, QStringLiteral("Bay 1"),
                     DisplayFrameContext{.session_service = &session});

  auto* live = frame.findChild<QLabel*>(QStringLiteral("liveIndicator"));
  ASSERT_NE(live, nullptr);
  EXPECT_EQ(live->text(), QStringLiteral("Offline"));
}

TEST_F(DisplayFrameStripsTest, LivePillReadsLiveWhileConnected) {
  testing::NiceMock<scada::MockSessionService> session;
  ON_CALL(session, IsConnected(testing::_))
      .WillByDefault(testing::Return(true));

  DisplayFrame frame(/*diagram=*/nullptr, QStringLiteral("Bay 1"),
                     DisplayFrameContext{.session_service = &session});

  auto* live = frame.findChild<QLabel*>(QStringLiteral("liveIndicator"));
  ASSERT_NE(live, nullptr);
  EXPECT_EQ(live->text(), QStringLiteral("Live"));
}

// ...and keeps following it afterwards: a session dropping and coming back
// turns the pill to «Нет связи» and back to «Онлайн» without reopening the
// display (backlog 911).
TEST_F(DisplayFrameStripsTest, LivePillFollowsSessionStateChanges) {
  testing::NiceMock<scada::MockSessionService> session;
  ON_CALL(session, IsConnected(testing::_))
      .WillByDefault(testing::Return(true));
  boost::signals2::signal<void(bool, const scada::Status&)> state_changed;
  ON_CALL(session, SubscribeSessionStateChanged(testing::_))
      .WillByDefault(
          [&state_changed](
              const scada::SessionService::SessionStateChangedCallback& cb) {
            return boost::signals2::scoped_connection{
                state_changed.connect(cb)};
          });

  DisplayFrame frame(/*diagram=*/nullptr, QStringLiteral("Bay 1"),
                     DisplayFrameContext{.session_service = &session});
  auto* live = frame.findChild<QLabel*>(QStringLiteral("liveIndicator"));
  ASSERT_NE(live, nullptr);
  ASSERT_EQ(live->text(), QStringLiteral("Live"));

  state_changed(false, scada::StatusCode::Bad_Disconnected);
  EXPECT_EQ(live->text(), QStringLiteral("Offline"));

  state_changed(true, scada::StatusCode::Good);
  EXPECT_EQ(live->text(), QStringLiteral("Live"));
}
