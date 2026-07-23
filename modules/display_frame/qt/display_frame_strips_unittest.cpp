#include "display_frame/qt/display_frame.h"

#include "aui/test/app_environment.h"
#include "base/time/time.h"
#include "scada/date_time.h"
#include "events/node_event_provider.h"
#include "scada/event.h"

#include <gtest/gtest.h>

#include <QString>
#include <QStringList>
#include <QTableWidget>

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

TEST_F(DisplayFrameStripsTest, NoBayStripsWithoutDataSources) {
  DisplayFrame frame(/*diagram=*/nullptr, QStringLiteral("Bay 1"),
                     DisplayFrameContext{});
  EXPECT_EQ(frame.findChild<QTableWidget*>(), nullptr);
}

}  // namespace
