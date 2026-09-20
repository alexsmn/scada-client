#include "events/event_table_model.h"

#include "events/audit_events.h"

#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "base/check.h"
#include "base/excel.h"
#include "base/format.h"
#include "base/format_time.h"
#include "base/utf_convert.h"
#include "base/utils.h"
#include "events/alarm_flood.h"
#include "events/current_event_model.h"
#include "events/event_grouping.h"
#include "events/event_severity.h"
#include "events/historical_event_model.h"
#include "events/local_event_model.h"
#include "node_service/node_format.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"

#include <algorithm>
#include <map>
#include <optional>

using namespace std::chrono_literals;

namespace {

// The object a client-side event is attributed to, in place of the node
// display name a server event resolves. Translated: it is the only cell text
// in this column an operator ever sees that is not a node's own name, and as
// a bare u"" literal it rendered English in a Russian journal.

void GetEventColors(const scada::Event& event,
                    scada::aui::Color& text_color,
                    scada::aui::Color& back_color) {
  // The colours themselves come from the single severity source, so they follow
  // the active theme and stay in step with every other severity surface.
  //
  // Severity owns the row colour; acknowledgement is carried by the leading dot
  // column, the "— pending —" acknowledge-time cell and the alarm footer.
  // Classifying unacknowledged first — as the journal did before it had those
  // three — painted every pending row green, including a pending *critical*
  // alarm, which reads as "normal" and spends saturated colour on something
  // that is not a severity: both against principles.md §1 ("reserve bright
  // colour exclusively for abnormal conditions") and §5, which describes the
  // intended design as "a red alarm row also carries a severity label and an
  // unacknowledged dot".
  const std::optional<scada::aui::EventBackground> background =
      events::EventBackgroundForSeverity(event.severity);
  if (!background)
    return;

  const scada::aui::EventRowColors colors =
      scada::aui::EventRowColorsFor(*background);
  back_color = colors.background;
  text_color = colors.text;
}

template <class T>
int Compare(const T& a, const T& b) {
  return a < b ? -1 : b < a ? 1 : 0;
}

}  // namespace

// EventTableModel::Row

void EventTableModel::Row::Update(NodeService& node_service) {
  scada::base::Check(event);

  node = node_service.GetNode(event->source_node_id);
  user = node_service.GetNode(event->user_id);
  acknowledged_user = node_service.GetNode(event->acknowledged_user_id);

  node.StartFetch(NodeFetchStatus::NodeOnly);
  user.StartFetch(NodeFetchStatus::NodeOnly);
  acknowledged_user.StartFetch(NodeFetchStatus::NodeOnly);
}

void EventTableModel::Row::RecountUnacked() {
  unacked = event->acked ? 0 : 1;
  for (const scada::Event* repeat : repeats) {
    if (!repeat->acked)
      ++unacked;
  }
}

bool EventTableModel::Row::IsAffected(const scada::NodeId& node_id) const {
  scada::base::Check(event);
  return event->source_node_id == node_id || event->user_id == node_id ||
         event->acknowledged_user_id == node_id;
}

// EventTableModel

EventTableModel::EventTableModel(EventTableModelContext&& context)
    : EventTableModelContext(std::move(context)) {
  connections_.emplace_back(current_event_model_.on_events.connect(
      std::bind_front(&EventTableModel::OnCurrentEvents, this)));

  connections_.emplace_back(current_event_model_.on_all_acked.connect([this] {
    if (!rows_.empty()) {
      AckRows(0, static_cast<int>(rows_.size()));
    }
  }));

  connections_.emplace_back(historical_event_model_.refilter_now.connect(
      std::bind_front(&EventTableModel::RefilterNow, this)));

  connections_.emplace_back(local_event_model_.on_event.connect(
      std::bind_front(&EventTableModel::OnLocalEvent, this)));

  connections_.emplace_back(node_service_.SubscribeNodeSemanticChanged(
      std::bind_front(&EventTableModel::OnNodeSemanticChanged, this)));
  connections_.emplace_back(node_service_.SubscribeModelChanged(
      std::bind_front(&EventTableModel::OnModelChanged, this)));
}

EventTableModel::~EventTableModel() = default;

void EventTableModel::Init(const scada::RelativeTimeRange& range,
                           ItemIds filter_items) {
  historical_event_model_.Init(range);
  filter_node_ids_ = std::move(filter_items);
  Update();
}

const scada::RelativeTimeRange& EventTableModel::time_range() const {
  return historical_event_model_.time_range();
}

void EventTableModel::SetTimeRange(const scada::RelativeTimeRange& range) {
  if (historical_event_model_.time_range() == range)
    return;

  historical_event_model_.SetTimeRange(range);

  Update();
}

int EventTableModel::GetRowCount() {
  return static_cast<int>(rows_.size());
}

void EventTableModel::GetCell(scada::aui::TableCell& cell) {
  const Row& row = rows_[cell.row];
  GetEventCell(row, *row.event, 1 + static_cast<int>(row.repeats.size()), cell);
}

int EventTableModel::GetOccurrenceCount() const {
  EnsureOccurrenceOffsets();
  return occurrence_offsets_.back();
}

std::pair<const EventTableModel::Row*, const scada::Event*>
EventTableModel::OccurrenceAt(int index) const {
  EnsureOccurrenceOffsets();
  if (index < 0 || index >= occurrence_offsets_.back())
    return {nullptr, nullptr};

  // The row whose occurrence range holds `index` is the last one starting at
  // or before it.
  auto after = std::upper_bound(occurrence_offsets_.begin(),
                                occurrence_offsets_.end(), index);
  const int row_index =
      static_cast<int>(after - occurrence_offsets_.begin()) - 1;
  const Row& row = rows_[row_index];
  const int within = index - occurrence_offsets_[row_index];
  if (within == 0)
    return {&row, row.event};
  return {&row, row.repeats[within - 1]};
}

void EventTableModel::EnsureOccurrenceOffsets() const {
  if (occurrence_offsets_valid_)
    return;

  occurrence_offsets_.clear();
  occurrence_offsets_.reserve(rows_.size() + 1);
  occurrence_offsets_.push_back(0);
  for (const Row& row : rows_) {
    occurrence_offsets_.push_back(occurrence_offsets_.back() + 1 +
                                  static_cast<int>(row.repeats.size()));
  }
  occurrence_offsets_valid_ = true;
}

void EventTableModel::EnsureRowIndex() const {
  if (row_index_valid_)
    return;

  occurrence_rows_.clear();
  alarm_group_rows_.clear();
  occurrence_rows_.reserve(rows_.size());
  for (int index = 0; index < static_cast<int>(rows_.size()); ++index)
    IndexRow(index);
  row_index_valid_ = true;
}

void EventTableModel::IndexRow(int index) const {
  const Row& row = rows_[index];
  occurrence_rows_[row.event] = index;
  for (const scada::Event* repeat : row.repeats)
    occurrence_rows_[repeat] = index;
  // The first row of an alarm wins, as the scan this replaced did.
  alarm_group_rows_.try_emplace({row.type, events::AlarmKeyOf(*row.event)},
                                index);
}

void EventTableModel::GetOccurrenceCell(scada::aui::TableCell& cell) {
  const auto [row, event] = OccurrenceAt(cell.row);
  if (!row)
    return;

  // Each occurrence stands alone here, so none of them carries a count.
  GetEventCell(*row, *event, /*group_count=*/1, cell);
}

void EventTableModel::GetEventCell(const Row& row,
                                   const scada::Event& event,
                                   int group_count,
                                   scada::aui::TableCell& cell) const {
  GetEventColors(event, cell.text_color, cell.cell_color);

  switch (cell.column_id) {
    case EventColumnTime:
      cell.text = UtfConvert<char16_t>(FormatTime(
          event.time, TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
      break;
    case EventColumnSeverity:
      cell.text = WideFormat(event.severity);
      // Name the band as well as the number: an
      // operator triaging a journal reads "Critical", not 80, and naming it
      // means the row's severity no longer depends on its colour alone.
      if (const std::u16string label =
              events::EventSeverityLabel(event.severity);
          !label.empty()) {
        cell.text = label + u" " + cell.text;
      }
      break;
    case EventColumnItem:
      if (row.node)
        cell.text = GetFullDisplayName(row.node);
      else if (row.type == LOCAL_EVENT)
        cell.text = Translate("Local Event");
      break;
    case EventColumnMessage:
      // A flood group carries its occurrence count here, so a chattering source
      // reads as one line with a number instead of fifty lines to scroll past.
      cell.text = events::FormatGroupedMessage(event.message.text, group_count);
      break;
    case EventColumnUser:
      // The row's NodeRef belongs to the occurrence on display; a collapsed
      // repeat can carry a different user, so resolve it from the event being
      // rendered rather than assuming the row's.
      if (row.event == &event) {
        if (row.user)
          cell.text = ToString16(row.user.display_name());
      } else if (NodeRef user = node_service_.GetNode(event.user_id)) {
        cell.text = ToString16(user.display_name());
      }
      break;
    case EventColumnValue:
      if (row.node)
        cell.text = FormatValue(row.node, event.value, event.qualifier);
      else
        cell.text = ToString16(event.value.get_or(scada::LocalizedText{}));
      break;
    case EventColumnAckUser:
      if (row.event == &event) {
        if (row.acknowledged_user)
          cell.text = ToString16(row.acknowledged_user.display_name());
      } else if (NodeRef acknowledged_user =
                     node_service_.GetNode(event.acknowledged_user_id)) {
        cell.text = ToString16(acknowledged_user.display_name());
      }
      break;
    case EventColumnAckTime:
      // An unacknowledged alarm leaves this cell blank, which reads as "no
      // data" rather than "nobody has responded yet". Under the reshell theme
      // say so outright — the journal is an alarm surface, and a pending
      // response is its most actionable state.
      if (!event.acked) {
        cell.text = Translate("— pending —");
        break;
      }
      cell.text = UtfConvert<char16_t>(
          FormatTime(event.acknowledged_time,
                     TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
      break;
    case EventColumnUnacked:
      // The leading pending marker: a dot on every unacknowledged row, so the
      // actionable rows read at a glance. It deliberately keeps the row's own
      // text colour rather than re-encoding the severity: the row background
      // already carries that, and a severity-coloured dot on a
      // severity-coloured row is invisible (a red dot on a red critical row).
      // The dot means "pending", the background means "how bad", and the
      // "— pending —" cell spells the state out (colour is never the only
      // cue).
      if (!event.acked)
        cell.text = u"●";
      break;
    default:
      scada::base::NotReached();
  }
}

int EventTableModel::FindRow(const scada::Event& event) const {
  EnsureRowIndex();
  auto found = occurrence_rows_.find(&event);
  // A repeat collapsed into a row is not that row's event.
  if (found == occurrence_rows_.end() || rows_[found->second].event != &event)
    return -1;
  const Row& row = rows_[found->second];
  scada::base::Check(row.type == CURRENT_EVENT || row.type == LOCAL_EVENT);
  return found->second;
}

bool EventTableModel::IsEventShown(const scada::Event& event) const {
  return PassesFilters(event, /*include_area_filter=*/true);
}

bool EventTableModel::PassesFilters(const scada::Event& event,
                                    bool include_area_filter) const {
  if (event.severity < severity_min_)
    return false;

  if (unacknowledged_only_ && event.acked)
    return false;

  if (audit_only_ && !IsAuditEventType(event.event_type_id))
    return false;

  if (!include_area_filter || filter_node_ids_.empty())
    return true;

  return IsUnderAnyOf(event, filter_node_ids_);
}

bool EventTableModel::IsUnderAnyOf(const scada::Event& event,
                                   const ItemIds& areas) const {
  // The source itself, or any containing node, is one of `areas`.
  if (areas.find(event.source_node_id) != areas.end())
    return true;

  for (auto node = node_service_.GetNode(event.source_node_id); node;
       node = node.parent()) {
    if (areas.find(node.node_id()) != areas.end())
      return true;
  }

  return false;
}

EventTableModel::AreaCounts EventTableModel::CountUnacknowledgedByArea(
    std::span<const scada::NodeId> areas) const {
  AreaCounts counts;
  counts.per_area.assign(areas.size(), 0);

  // A source sits in the same area for every event it raised, and a flood is
  // thousands of events from a handful of sources, so the containment chain
  // is walked once per source rather than once per event (task 721).
  std::map<scada::NodeId, int> area_of_source;  // -1: under none of `areas`
  auto area_index_of = [&](const scada::NodeId& source) {
    auto [entry, inserted] = area_of_source.try_emplace(source, -1);
    if (!inserted)
      return entry->second;
    // Collect the source's containment chain, then attribute it to its area
    // (top-level areas are siblings, so at most one matches).
    ItemIds chain{source};
    for (auto node = node_service_.GetNode(source); node; node = node.parent())
      chain.insert(node.node_id());
    for (size_t i = 0; i < areas.size(); ++i) {
      if (chain.contains(areas[i])) {
        entry->second = static_cast<int>(i);
        break;
      }
    }
    return entry->second;
  };

  auto account = [&](const scada::Event& event) {
    // Ignore the active area filter — the sidebar's counts stay meaningful
    // for every area while one of them is filtering the rows — but respect
    // the other filters, so the counts match what selecting an area would
    // show.
    if (event.acked || !PassesFilters(event, /*include_area_filter=*/false))
      return;
    ++counts.total;
    if (const int area = area_index_of(event.source_node_id); area >= 0)
      ++counts.per_area[area];
  };

  std::set<scada::EventId> current_ids;
  for (const scada::Event& event : current_event_model_.events()) {
    current_ids.insert(event.event_id);
    account(event);
  }
  for (const scada::Event& event : local_event_model_.events())
    account(event);
  if (!current_events_) {
    // Historical copies of live events dedupe by id, as in RefilterNow().
    for (const scada::Event& event : historical_event_model_.events()) {
      if (!current_ids.contains(event.event_id))
        account(event);
    }
  }

  return counts;
}

void EventTableModel::AddRows(EventType type,
                              std::span<const scada::Event* const> events) {
  // One rebuild at most, then every lookup and append below is O(1).
  EnsureRowIndex();

  std::vector<const scada::Event*> added_events;

  for (auto* event : events) {
    int index = FindRow(*event);
    if (index == -1) {
      if (IsEventShown(*event))
        added_events.emplace_back(event);
    } else {
      // Update row data.
      auto& row = rows_[index];
      scada::base::Check(row.type == type);
      row.Update(node_service_);
      // The row holds this pointer already, so nothing was added — but the
      // event behind it may have been acknowledged since
      // (MoveOccurrenceToHistory re-adds an acked copy through here), so the
      // row is recounted rather than assumed unchanged.
      RecountRowUnacked(index);
      NotifyItemsChanged(index, 1);
    }
  }

  if (grouped_) {
    // Fold each arrival into the group it repeats, so a chattering source bumps
    // one row's count instead of growing the list. Rows stay live here rather
    // than waiting for a rebuild: a flood is when a rebuild per batch would
    // cost the most, and the event pointers are only valid for this call.
    for (auto* event : added_events) {
      const int index = FindAlarmGroupRow(type, *event);
      if (index == -1) {
        const int first = static_cast<int>(rows_.size());
        NotifyItemsAdding(first, 1);
        Row row{type, *event};
        row.Update(node_service_);
        unacked_count_ += row.unacked;
        rows_.emplace_back(std::move(row));
        IndexRow(first);
        InvalidateOccurrenceOffsets();
        NotifyItemsAdded(first, 1);
        continue;
      }

      Row& row = rows_[index];
      // The newest occurrence represents the group (see GroupRepeatedEvents),
      // so an arrival that is newer takes over and the previous one becomes a
      // repeat.
      if (row.event->time <= event->time) {
        row.repeats.push_back(row.event);
        row.event = event;
        row.Update(node_service_);
      } else {
        row.repeats.push_back(event);
      }
      // The row keeps its position; only the new pointer needs registering.
      occurrence_rows_[event] = index;
      RecountRowUnacked(index);
      InvalidateOccurrenceOffsets();
      NotifyItemsChanged(index, 1);
    }
    return;
  }

  if (!added_events.empty()) {
    int first = static_cast<int>(rows_.size());
    NotifyItemsAdding(first, added_events.size());
    rows_.reserve(rows_.size() + added_events.size());
    for (auto* event : added_events) {
      Row row{type, *event};
      row.Update(node_service_);
      unacked_count_ += row.unacked;
      rows_.emplace_back(std::move(row));
      IndexRow(static_cast<int>(rows_.size()) - 1);
    }
    InvalidateOccurrenceOffsets();
    NotifyItemsAdded(first, added_events.size());
  }
}

int EventTableModel::FindOccurrenceRow(const scada::Event& event) const {
  EnsureRowIndex();
  auto found = occurrence_rows_.find(&event);
  return found == occurrence_rows_.end() ? -1 : found->second;
}

int EventTableModel::FindAlarmGroupRow(EventType type,
                                       const scada::Event& event) const {
  EnsureRowIndex();
  auto found = alarm_group_rows_.find({type, events::AlarmKeyOf(event)});
  return found == alarm_group_rows_.end() ? -1 : found->second;
}

bool EventTableModel::RemoveOccurrence(int index, const scada::Event& event) {
  Row& row = rows_[index];
  if (row.repeats.empty())
    return false;

  if (row.event == &event) {
    // The occurrence on display is going away; the newest of the rest takes
    // over so the row keeps showing the latest state of the alarm.
    auto newest = row.repeats.begin();
    for (auto i = row.repeats.begin(); i != row.repeats.end(); ++i) {
      if ((*newest)->time < (*i)->time)
        newest = i;
    }
    row.event = *newest;
    row.repeats.erase(newest);
    row.Update(node_service_);
  } else {
    std::erase(row.repeats, &event);
  }

  // The row keeps its index and its alarm; only the departed pointer must go.
  if (row_index_valid_)
    occurrence_rows_.erase(&event);
  RecountRowUnacked(index);
  InvalidateOccurrenceOffsets();

  NotifyItemsChanged(index, 1);
  return true;
}

void EventTableModel::MoveOccurrenceToHistory(const scada::Event& event) {
  scada::Event acked_event = event;
  acked_event.acked = true;
  const scada::Event& historical_event =
      historical_event_model_.AddEvent(std::move(acked_event));
  const scada::Event* event_ptr = &historical_event;
  AddRows(HISTORICAL_EVENT, {&event_ptr, 1});
}

void EventTableModel::RecountRowUnacked(int index) {
  Row& row = rows_[index];
  const int before = row.unacked;
  row.RecountUnacked();
  unacked_count_ += row.unacked - before;
}

void EventTableModel::RecountAllUnacked() {
  unacked_count_ = 0;
  for (const Row& row : rows_)
    unacked_count_ += row.unacked;
}

// Still a walk, and deliberately: the footer asks for this on demand, not per
// notification, and `max_severity` is not a quantity a running tally can
// maintain — a maximum cannot be decremented when the row holding it goes. Its
// `unacknowledged` is therefore an independent second opinion on
// CountUnacknowledged(), which is what the tests cross-check.
EventTableModel::AlarmSummary EventTableModel::GetAlarmSummary() const {
  AlarmSummary summary;
  auto account = [&summary](const scada::Event& event) {
    if (event.acked)
      return;
    ++summary.unacknowledged;
    summary.max_severity = std::max(summary.max_severity, event.severity);
  };
  for (const Row& row : rows_) {
    account(*row.event);
    for (const scada::Event* repeat : row.repeats)
      account(*repeat);
  }
  return summary;
}

void EventTableModel::RegroupIfFloodChanged() {
  if (events::IsAlarmFlood(CountUnacknowledged()) != grouped_)
    RefilterNow();
}

void EventTableModel::AppendRows(Rows& rows,
                                 EventType type,
                                 std::span<const scada::Event* const> events,
                                 bool grouped) const {
  if (!grouped) {
    for (const scada::Event* event : events) {
      Row row{type, *event};
      row.Update(node_service_);
      rows.emplace_back(std::move(row));
    }
    return;
  }

  for (events::EventGroup& group : events::GroupRepeatedEvents(events)) {
    Row row{type, *group.representative};
    row.repeats = std::move(group.repeats);
    // The constructor counted the representative alone; the repeats just
    // arrived. `rows` is a local being built, so the model's tally is set by
    // the caller (RefilterNow) once the rows are installed.
    row.RecountUnacked();
    row.Update(node_service_);
    rows.emplace_back(std::move(row));
  }
}

void EventTableModel::RemoveRows(int first, int count) {
  scada::base::Check(count > 0);
  NotifyItemsRemoving(first, count);
  for (int i = first; i < first + count; ++i)
    unacked_count_ -= rows_[i].unacked;
  rows_.erase(rows_.begin() + first, rows_.begin() + (first + count));
  // Every row behind the gap moved.
  InvalidateRowIndex();
  InvalidateOccurrenceOffsets();
  NotifyItemsRemoved(first, count);
}

void EventTableModel::UpdateAffectedRows(const scada::NodeId& node_id) {
  // TODO: check only visible rows
  int min_index = -1, max_index = -1;
  int index = 0;
  for (auto& row : rows_) {
    if (row.IsAffected(node_id)) {
      if (min_index == -1)
        min_index = index;
      max_index = index;
    }
    ++index;
  }
  if (min_index != -1)
    NotifyItemsChanged(min_index, max_index - min_index + 1);
}

void EventTableModel::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  UpdateAffectedRows(node_id);
}

void EventTableModel::OnModelChanged(const scada::ModelChangeEvent& event) {
  UpdateAffectedRows(event.node_id);
}

void EventTableModel::OnCurrentEvents(
    std::span<const scada::Event* const> events) {
  std::vector<const scada::Event*> partitioned_events(events.begin(),
                                                      events.end());
  auto first_unacked = std::stable_partition(
      partitioned_events.begin(), partitioned_events.end(),
      [](const scada::Event* event) { return event->acked; });

  // Remove acked. An acknowledged occurrence may be collapsed inside a group
  // rather than be a row of its own, so drop that one occurrence and keep the
  // row when others remain. This cannot be deferred to a rebuild: the storage
  // extracts an acknowledged event before notifying, so its pointer dies with
  // this call and must not stay in a group.
  for (auto i = partitioned_events.begin(); i != first_unacked; ++i) {
    auto& event = **i;
    int index = FindOccurrenceRow(event);
    if (index == -1)
      continue;

    if (RemoveOccurrence(index, event)) {
      // The row survives. In the journal the acknowledged occurrence still
      // belongs in the history, exactly as a whole-row acknowledgement moves it
      // there.
      if (!current_events_)
        MoveOccurrenceToHistory(event);
      continue;
    }

    AckRows(index, 1);
  }

  // Filter and add unacked.
  {
    std::vector<const scada::Event*> filtered_events;
    filtered_events.reserve(partitioned_events.end() - first_unacked);
    std::copy_if(
        first_unacked, partitioned_events.end(),
        std::back_inserter(filtered_events),
        [this](const scada::Event* event) { return IsEventShown(*event); });
    AddRows(CURRENT_EVENT, filtered_events);
  }

  RegroupIfFloodChanged();
}

void EventTableModel::AckRows(int first, int count) {
  scada::base::Check(count > 0);

  if (current_events_) {
    RemoveRows(first, count);

  } else {
    for (int i = 0; i < count; ++i) {
      // Convert to historical.
      Row& row = rows_[first + i];
      if (row.type == CURRENT_EVENT) {
        // Every occurrence the row stands for has to be copied across: the
        // storage pointers behind the collapsed repeats die with this
        // notification, and dropping them would lose them from the journal and
        // leave the row counting events that no longer exist.
        std::vector<const scada::Event*> repeats;
        repeats.swap(row.repeats);

        auto acked_event = *row.event;
        acked_event.acked = true;
        const scada::Event& historical_event =
            historical_event_model_.AddEvent(std::move(acked_event));
        row.type = HISTORICAL_EVENT;
        row.event = &historical_event;
        row.Update(node_service_);

        for (const scada::Event* repeat : repeats) {
          auto acked_repeat = *repeat;
          acked_repeat.acked = true;
          row.repeats.push_back(
              &historical_event_model_.AddEvent(std::move(acked_repeat)));
        }
        // The row now holds the historical copies' pointers, not the live
        // ones; the occurrence count is unchanged, but all of them are
        // acknowledged now, so the row leaves the backlog.
        RecountRowUnacked(first + i);
        InvalidateRowIndex();
      }
    }
    NotifyItemsChanged(first, count);
  }
}

void EventTableModel::OnLocalEvent(const scada::Event& event) {
  if (event.acked) {
    // LocalEvents destroys the event right after this notification, so — as
    // with the storage above — the occurrence has to leave the row now.
    const int index = FindOccurrenceRow(event);
    if (index != -1 && !RemoveOccurrence(index, event))
      RemoveRows(index, 1);
  } else {
    auto* event_ptr = &event;
    AddRows(LOCAL_EVENT, {&event_ptr, 1});
  }

  RegroupIfFloodChanged();
}

bool EventTableModel::AddFilteredItem(const scada::NodeId& item) {
  if (!filter_node_ids_.insert(item).second)
    return false;

  Refilter();
  return true;
}

bool EventTableModel::RemoveFilteredItem(const scada::NodeId& item) {
  if (!filter_node_ids_.erase(item))
    return false;

  Refilter();
  return true;
}

void EventTableModel::Refilter() {
  refilter_delay_timer_.StartRepeating(
      300ms, std::bind_front(&EventTableModel::RefilterNow, this));
}

void EventTableModel::SetSeverityMin(unsigned severity) {
  if (severity_min_ == severity)
    return;

  severity_min_ = severity;
  RefilterNow();
}

void EventTableModel::SetAuditOnly(bool value) {
  if (audit_only_ == value)
    return;
  audit_only_ = value;
  RefilterNow();
}

void EventTableModel::SetUnacknowledgedOnly(bool value) {
  if (unacknowledged_only_ == value)
    return;

  unacknowledged_only_ = value;
  RefilterNow();
}

void EventTableModel::RefilterNow() {
  refilter_delay_timer_.Stop();

  if (!rows_.empty()) {
    int count = static_cast<int>(rows_.size());
    NotifyItemsRemoving(0, count);
    rows_.clear();
    unacked_count_ = 0;
    InvalidateRowIndex();
    InvalidateOccurrenceOffsets();
    NotifyItemsRemoved(0, count);
  }

  std::vector<const scada::Event*> local_events;
  for (const scada::Event& event : local_event_model_.events())
    local_events.push_back(&event);

  std::vector<const scada::Event*> current_events;
  for (const scada::Event& event : current_event_model_.events()) {
    if (IsEventShown(event))
      current_events.push_back(&event);
  }

  std::vector<const scada::Event*> historical_events;
  if (!current_events_) {
    // The current (live) surface takes precedence over the history record: an
    // unacknowledged alarm that is both live and already in the read history
    // is one event, not two rows. Dedupe by event id — ids are server-issued
    // and unique, so a historical row with a live counterpart is the same
    // record.
    std::set<scada::EventId> current_ids;
    for (const scada::Event* event : current_events)
      current_ids.insert(event->event_id);

    for (const scada::Event& event : historical_event_model_.events()) {
      if (IsEventShown(event) && !current_ids.contains(event.event_id))
        historical_events.push_back(&event);
    }
  }

  // Group exactly while the operator is buried, judged on the same signal as
  // the shell's flood pill: the actionable (unacknowledged) backlog. Acking the
  // backlog back below the threshold expands the rows again on the next
  // rebuild, so grouping is a property of the situation, not a mode to leave
  // on.
  int unacknowledged = 0;
  for (const auto* events_span :
       {&local_events, &current_events, &historical_events}) {
    for (const scada::Event* event : *events_span) {
      if (!event->acked)
        ++unacknowledged;
    }
  }
  grouped_ = events::IsAlarmFlood(unacknowledged);

  std::vector<Row> rows;
  AppendRows(rows, LOCAL_EVENT, local_events, grouped_);
  AppendRows(rows, CURRENT_EVENT, current_events, grouped_);
  if (!current_events_)
    AppendRows(rows, HISTORICAL_EVENT, historical_events, grouped_);

  if (!rows.empty()) {
    int count = static_cast<int>(rows.size());
    NotifyItemsAdding(0, count);
    rows_ = std::move(rows);
    // After the assignment, never before it: `NotifyItemsAdding` above runs
    // observers, and an observer that queries the model (Qt's own views ask
    // for a row count inside `beginInsertRows`) would rebuild the tables from
    // the *old* rows_ and mark them valid, leaving them stale for the rows
    // that arrive on the next line.
    InvalidateRowIndex();
    InvalidateOccurrenceOffsets();
    RecountAllUnacked();
    NotifyItemsAdded(0, count);
  }
}

void EventTableModel::Update() {
  if (lock_update_) {
    pending_update_ = true;
    return;
  }

  pending_update_ = false;

  if (!rows_.empty()) {
    int count = static_cast<int>(rows_.size());
    NotifyItemsRemoving(0, count);
    rows_.clear();
    unacked_count_ = 0;
    InvalidateRowIndex();
    InvalidateOccurrenceOffsets();
    NotifyItemsRemoved(0, count);
  }

  if (!current_events_) {
    historical_event_model_.Update();
  }

  RefilterNow();
}

void EventTableModel::CancelRequest() {
  historical_event_model_.CancelRequest();
}

int EventTableModel::group_count_at(int row) const {
  return 1 + static_cast<int>(rows_[row].repeats.size());
}

void EventTableModel::AcknowledgeRows(std::span<const int> rows) {
  // Resolve every target before acknowledging any of them. Acknowledging
  // notifies, and a notification removes the occurrence from its row, can drop
  // the row entirely, and can regroup the whole list once the backlog leaves
  // flood — so both the row indices and any pointer into a row go stale
  // partway through. Event ids survive all of it.
  struct Target {
    EventType type;
    scada::EventId event_id;
  };
  std::vector<Target> targets;

  for (int row : rows) {
    const Row& r = rows_[row];
    // Every occurrence the row stands for: a collapsed row hides its repeats,
    // so acknowledging only the one on display would clear what the operator
    // can see and leave the rest of the count behind it.
    targets.push_back({r.type, r.event->event_id});
    for (const scada::Event* repeat : r.repeats)
      targets.push_back({r.type, repeat->event_id});
  }

  for (const Target& target : targets) {
    switch (target.type) {
      case CURRENT_EVENT:
        // Event state comes from the server; it may already have been acked
        // concurrently.
        current_event_model_.Ack(target.event_id);
        break;

      case HISTORICAL_EVENT:
        // Do nothing.
        break;

      case LOCAL_EVENT:
        local_event_model_.Ack(target.event_id);
        break;

      default:
        scada::base::NotReached();
    }
  }
}

void EventTableModel::LockUpdate() {
  scada::base::Check(!lock_update_);
  lock_update_ = true;
}

void EventTableModel::UnlockUpdate() {
  scada::base::Check(lock_update_);
  lock_update_ = false;
  if (pending_update_)
    Update();
}

std::u16string EventTableModel::MakeTitle() const {
  std::u16string title;
  if (current_events_) {
    title = Translate("Current Events");
  } else {
    switch (historical_event_model_.time_range().type) {
      case scada::RelativeTimeRange::Type::Day:
        title = Translate("Event Journal for Day");
        break;
      case scada::RelativeTimeRange::Type::Week:
        title = Translate("Event Journal for Week");
        break;
      case scada::RelativeTimeRange::Type::Month:
        title = Translate("Event Journal for Month");
        break;
      case scada::RelativeTimeRange::Type::Custom:
      default:
        title = Translate("Event Journal");  // TODO: Format time range.
        break;
    }
  }

  if (severity_min_ || !filter_node_ids_.empty())
    title += u" (" + Translate("Filter") + u")";

  return title;
}

bool EventTableModel::IsWorking() const {
  if (current_events_)
    return current_event_model_.working();
  else
    return historical_event_model_.working();
}

int EventTableModel::CompareCells(int row1, int row2, int column_id) {
  const auto& event1 = *rows_[row1].event;
  const auto& event2 = *rows_[row2].event;

  switch (column_id) {
    case EventColumnTime:
      return Compare(event1.time, event2.time);
    case EventColumnAckTime:
      return Compare(event1.acknowledged_time, event2.acknowledged_time);
    case EventColumnUnacked:
      // Pending (unacknowledged) rows order together; ties keep their
      // relative order via the stable sort.
      return Compare(event1.acked, event2.acked);
    case EventColumnSeverity:
      // Compare the severity itself, not its cell text: the text sorts
      // lexically, which already misordered "100" against "80" and now also
      // carries the band name in front of the number.
      return Compare(event1.severity, event2.severity);
    default:
      return scada::aui::TableModel::CompareCells(row1, row2, column_id);
  }
}
