#include "events/event_grouping.h"

#include "base/u16format.h"

#include <map>
#include <utility>

namespace events {
namespace {

// What makes two occurrences "the same alarm": the source and what it said.
// Severity is not part of the key — the same condition reported at a different
// severity is still the same condition, and splitting on it would break the
// collapse exactly when a chattering source escalates.
using GroupKey = std::pair<scada::NodeId, std::u16string>;

GroupKey KeyOf(const scada::Event& event) {
  return {event.node_id, event.message};
}

}  // namespace

bool IsSameAlarm(const scada::Event& a, const scada::Event& b) {
  return KeyOf(a) == KeyOf(b);
}

std::vector<EventGroup> GroupRepeatedEvents(
    std::span<const scada::Event* const> events) {
  // Members per group, in input order. Group position is fixed by the first
  // occurrence, so a caller passing events in display order keeps that order
  // regardless of which member ends up representing the group.
  std::vector<std::vector<const scada::Event*>> members;
  std::map<GroupKey, size_t> index_of;

  for (const scada::Event* event : events) {
    if (!event)
      continue;

    auto [it, inserted] = index_of.try_emplace(KeyOf(*event), members.size());
    if (inserted)
      members.emplace_back();
    members[it->second].push_back(event);
  }

  std::vector<EventGroup> groups;
  groups.reserve(members.size());

  for (std::vector<const scada::Event*>& group_members : members) {
    // The newest occurrence represents the group: during a flood the operator
    // is looking at what is happening now, and the row's time, value and
    // acknowledgement should be the latest ones.
    auto newest = group_members.begin();
    for (auto i = group_members.begin(); i != group_members.end(); ++i) {
      if ((*newest)->time < (*i)->time)
        newest = i;
    }

    EventGroup group{.representative = *newest};
    group.repeats.reserve(group_members.size() - 1);
    for (auto i = group_members.begin(); i != group_members.end(); ++i) {
      if (i != newest)
        group.repeats.push_back(*i);
    }
    groups.push_back(std::move(group));
  }

  return groups;
}

std::u16string FormatGroupedMessage(const std::u16string& message, int count) {
  if (count <= 1)
    return message;

  // U+00D7 MULTIPLICATION SIGN, escaped rather than written literally so the
  // source file stays ASCII (the client keeps non-ASCII in the .ts files).
  return u16format(L"{} \u00d7{}", message, count);
}

}  // namespace events
