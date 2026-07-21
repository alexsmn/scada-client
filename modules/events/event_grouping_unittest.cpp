#include "events/event_grouping.h"

#include "scada/event.h"

#include <gtest/gtest.h>

#include <vector>

namespace events {
namespace {

scada::NodeId Node(int index) {
  return {static_cast<scada::NumericId>(index), 2};
}

scada::Event MakeEvent(int node, std::u16string message, int64_t seconds) {
  return {.event_id = static_cast<scada::EventId>(seconds + 1),
          .time = scada::DateTime::UnixEpoch() +
                  scada::base::TimeDelta::FromSeconds(seconds),
          .severity = scada::kSeverityCritical,
          .source_node_id = Node(node),
          .message = std::move(message)};
}

std::vector<const scada::Event*> Pointers(
    const std::vector<scada::Event>& events) {
  std::vector<const scada::Event*> pointers;
  for (const scada::Event& event : events)
    pointers.push_back(&event);
  return pointers;
}

TEST(EventGroupingTest, EmptyInputGroupsToNothing) {
  EXPECT_TRUE(GroupRepeatedEvents({}).empty());
}

TEST(EventGroupingTest, DistinctAlarmsDoNotCollapse) {
  const std::vector<scada::Event> events{MakeEvent(1, u"comms lost", 0),
                                         MakeEvent(2, u"comms lost", 1),
                                         MakeEvent(1, u"over limit", 2)};

  const std::vector<EventGroup> groups = GroupRepeatedEvents(Pointers(events));

  // Same message from a different source, and a different message from the
  // same source, are different alarms.
  ASSERT_EQ(groups.size(), 3u);
  for (const EventGroup& group : groups)
    EXPECT_EQ(group.count(), 1);
}

TEST(EventGroupingTest, RepeatsOfOneAlarmCollapseIntoOneGroup) {
  const std::vector<scada::Event> events{MakeEvent(1, u"comms lost", 0),
                                         MakeEvent(1, u"comms lost", 1),
                                         MakeEvent(1, u"comms lost", 2)};

  const std::vector<EventGroup> groups = GroupRepeatedEvents(Pointers(events));

  ASSERT_EQ(groups.size(), 1u);
  EXPECT_EQ(groups[0].count(), 3);
}

// The row shows what is happening now, so the newest occurrence represents the
// group regardless of the order the events arrive in.
TEST(EventGroupingTest, TheNewestOccurrenceRepresentsTheGroup) {
  const std::vector<scada::Event> events{MakeEvent(1, u"comms lost", 5),
                                         MakeEvent(1, u"comms lost", 40),
                                         MakeEvent(1, u"comms lost", 12)};

  const std::vector<EventGroup> groups = GroupRepeatedEvents(Pointers(events));

  ASSERT_EQ(groups.size(), 1u);
  EXPECT_EQ(groups[0].representative, &events[1]);
  EXPECT_EQ(groups[0].repeats.size(), 2u);
  // No occurrence is lost: the rest stay reachable for acknowledgement.
  EXPECT_EQ(groups[0].count(), 3);
}

// A caller passing events in display order keeps that order, so grouping never
// reshuffles the journal.
TEST(EventGroupingTest, GroupOrderFollowsFirstAppearance) {
  const std::vector<scada::Event> events{
      MakeEvent(1, u"a", 0), MakeEvent(2, u"b", 1), MakeEvent(1, u"a", 2)};

  const std::vector<EventGroup> groups = GroupRepeatedEvents(Pointers(events));

  ASSERT_EQ(groups.size(), 2u);
  EXPECT_EQ(groups[0].representative->source_node_id, Node(1));
  EXPECT_EQ(groups[1].representative->source_node_id, Node(2));
}

// The same condition escalating must not split into two rows — that would undo
// the collapse exactly when a source starts chattering harder.
TEST(EventGroupingTest, SeverityIsNotPartOfTheGroupKey) {
  std::vector<scada::Event> events{MakeEvent(1, u"comms lost", 0),
                                   MakeEvent(1, u"comms lost", 1)};
  events[1].severity = scada::kSeverityWarning;

  EXPECT_EQ(GroupRepeatedEvents(Pointers(events)).size(), 1u);
}

TEST(FormatGroupedMessageTest, SingleOccurrenceKeepsItsMessage) {
  EXPECT_EQ(FormatGroupedMessage(u"comms lost", 1), u"comms lost");
  EXPECT_EQ(FormatGroupedMessage(u"comms lost", 0), u"comms lost");
}

TEST(FormatGroupedMessageTest, RepeatsCarryTheirCount) {
  EXPECT_EQ(FormatGroupedMessage(u"comms lost", 7), u"comms lost \u00d77");
}

}  // namespace
}  // namespace events
