#include "components/events/event_table_model.h"

#include "base/excel.h"
#include "base/executor.h"
#include "base/format_time.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/utils.h"
#include "common/event_fetcher.h"
#include "common_resources.h"
#include "model/scada_node_ids.h"
#include "node_service/node_format.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "scada/data_value.h"

using namespace std::chrono_literals;

namespace {

const char16_t kLocalEventSource[] = u"Локальное событие";

void GetEventColors(const scada::Event& event,
                    aui::Color& text_color,
                    aui::Color& back_color) {
  if (!event.acked) {
    back_color = aui::Rgba{99, 190, 123};
  } else if (event.severity >= scada::kSeverityCritical) {
    back_color = aui::Rgba{248, 105, 107};
  } else if (event.severity >= scada::kSeverityWarning) {
    back_color = aui::Rgba{255, 235, 132};
  }
}

int Compare(base::Time a, base::Time b) {
  return a < b ? -1 : b < a ? 1 : 0;
}

}  // namespace

// EventTableModel::Row

void EventTableModel::Row::Update(NodeService& node_service) {
  assert(event);

  node = node_service.GetNode(event->node_id);
  user = node_service.GetNode(event->user_id);
  acknowledged_user = node_service.GetNode(event->acknowledged_user_id);

  node.Fetch(NodeFetchStatus::NodeOnly());
  user.Fetch(NodeFetchStatus::NodeOnly());
  acknowledged_user.Fetch(NodeFetchStatus::NodeOnly());
}

bool EventTableModel::Row::IsAffected(const scada::NodeId& node_id) const {
  assert(event);
  return event->node_id == node_id || event->user_id == node_id ||
         event->acknowledged_user_id == node_id;
}

// EventTableModel

EventTableModel::EventTableModel(EventTableModelContext&& context)
    : EventTableModelContext(std::move(context)) {
  local_events_.observers().AddObserver(this);
  // TODO: Use `current_event_model_`.
  node_event_provider_.AddObserver(*this);
  node_service_.Subscribe(*this);
}

EventTableModel::~EventTableModel() {
  CancelRequest();

  node_service_.Unsubscribe(*this);
  // TODO: Use `current_event_model_`.
  node_event_provider_.RemoveObserver(*this);
  local_events_.observers().RemoveObserver(this);
}

void EventTableModel::Init(const TimeRange& range, ItemIds filter_items) {
  time_range_ = range;
  filter_node_ids_ = std::move(filter_items);
  Update();
}

void EventTableModel::SetTimeRange(const TimeRange& range) {
  if (time_range_ == range)
    return;

  time_range_ = range;

  Update();
}

int EventTableModel::GetRowCount() {
  return static_cast<int>(rows_.size());
}

void EventTableModel::GetCell(aui::TableCell& cell) {
  const Row& row = rows_[cell.row];
  const scada::Event& event = *row.event;

  GetEventColors(event, cell.text_color, cell.cell_color);

  switch (cell.column_id) {
    case EventColumnTime:
      cell.text = base::UTF8ToUTF16(FormatTime(
          event.time, TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
      break;
    case EventColumnSeverity:
      cell.text = base::NumberToString16(event.severity);
      break;
    case EventColumnItem:
      if (row.node)
        cell.text = GetFullDisplayName(row.node);
      else if (row.type == LOCAL_EVENT)
        cell.text = kLocalEventSource;
      break;
    case EventColumnMessage:
      cell.text = event.message;
      break;
    case EventColumnUser:
      if (row.user)
        cell.text = ToString16(row.user.display_name());
      break;
    case EventColumnValue:
      if (row.node)
        cell.text = FormatValue(row.node, event.value, event.qualifier);
      else
        cell.text = ToString16(event.value.get_or(scada::LocalizedText{}));
      break;
    case EventColumnAckUser:
      if (row.acknowledged_user)
        cell.text = ToString16(row.acknowledged_user.display_name());
      break;
    case EventColumnAckTime:
      cell.text = base::UTF8ToUTF16(
          FormatTime(event.acknowledged_time,
                     TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
      break;
    default:
      assert(false);
      break;
  }
}

int EventTableModel::FindRow(const scada::Event& event) const {
  for (Rows::const_iterator i = rows_.begin(); i != rows_.end(); ++i) {
    if (i->event == &event) {
      assert(i->type == CURRENT_EVENT || i->type == LOCAL_EVENT);
      return static_cast<int>(i - rows_.begin());
    }
  }
  return -1;
}

bool EventTableModel::IsEventShown(const scada::Event& event) const {
  if (event.severity < severity_min_)
    return false;

  if (filter_node_ids_.empty())
    return true;

  // Check item is in filter.
  if (filter_node_ids_.find(event.node_id) != filter_node_ids_.end())
    return true;

  // Check any containing node is in filter.
  for (auto node = node_service_.GetNode(event.node_id); node;
       node = node.parent()) {
    if (filter_node_ids_.find(node.node_id()) != filter_node_ids_.end())
      return true;
  }

  return false;
}

void EventTableModel::AddRows(EventType type,
                              base::span<const scada::Event* const> events) {
  std::vector<const scada::Event*> added_events;

  for (auto* event : events) {
    int index = FindRow(*event);
    if (index == -1) {
      if (IsEventShown(*event))
        added_events.emplace_back(event);
    } else {
      // Update row data.
      auto& row = rows_[index];
      assert(row.type == type);
      row.Update(node_service_);
      NotifyItemsChanged(index, 1);
    }
  }

  if (!added_events.empty()) {
    int first = static_cast<int>(rows_.size());
    NotifyItemsAdding(first, added_events.size());
    rows_.reserve(rows_.size() + added_events.size());
    for (auto* event : added_events) {
      Row row{type, *event};
      row.Update(node_service_);
      rows_.emplace_back(std::move(row));
    }
    NotifyItemsAdded(first, added_events.size());
  }
}

void EventTableModel::RemoveRows(int first, int count) {
  NotifyItemsRemoving(first, count);
  rows_.erase(rows_.begin() + first, rows_.begin() + (first + count));
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

void EventTableModel::OnEvents(base::span<const scada::Event* const> events) {
  std::vector<const scada::Event*> partitioned_events(events.begin(),
                                                      events.end());
  auto first_unacked = std::stable_partition(
      partitioned_events.begin(), partitioned_events.end(),
      [](const scada::Event* event) { return event->acked; });

  // Remove acked.
  for (auto i = partitioned_events.begin(); i != first_unacked; ++i) {
    auto& event = **i;
    int index = FindRow(event);
    if (index != -1)
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
}

void EventTableModel::OnAllEventsAcknowledged() {
  AckRows(0, static_cast<int>(rows_.size()));
}

void EventTableModel::AckRows(int first, int count) {
  if (current_events_) {
    RemoveRows(first, count);

  } else {
    for (int i = 0; i < count; ++i) {
      // Convert to historical.
      Row& row = rows_[first + i];
      if (row.type == CURRENT_EVENT) {
        historical_events_.push_back(*row.event);
        scada::Event& historical_event = historical_events_.back();
        historical_event.acked = true;
        row.type = HISTORICAL_EVENT;
        row.event = &historical_event;
        row.Update(node_service_);
      }
    }
    NotifyItemsChanged(first, count);
  }
}

void EventTableModel::OnLocalEvent(const scada::Event& event) {
  if (event.acked) {
    int index = FindRow(event);
    if (index != -1)
      RemoveRows(index, 1);
  } else {
    auto* event_ptr = &event;
    AddRows(LOCAL_EVENT, {&event_ptr, 1});
  }
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

void EventTableModel::RefilterNow() {
  refilter_delay_timer_.Stop();

  if (!rows_.empty()) {
    int count = static_cast<int>(rows_.size());
    NotifyItemsRemoving(0, count);
    rows_.clear();
    NotifyItemsRemoved(0, count);
  }

  std::vector<Row> rows;

  const LocalEvents::Events& local_events = local_events_.events();
  for (auto i = local_events.begin(); i != local_events.end(); ++i) {
    const scada::Event& event = **i;
    Row row(LOCAL_EVENT, event);
    row.Update(node_service_);
    rows.emplace_back(std::move(row));
  }

  for (const scada::Event& event : current_event_model_.events()) {
    if (IsEventShown(event)) {
      Row row(CURRENT_EVENT, event);
      row.Update(node_service_);
      rows.emplace_back(std::move(row));
    }
  }

  if (!current_events_) {
    for (auto i = historical_events_.begin(); i != historical_events_.end();
         ++i) {
      const scada::Event& event = *i;
      if (IsEventShown(event)) {
        Row row(HISTORICAL_EVENT, event);
        row.Update(node_service_);
        rows.emplace_back(std::move(row));
      }
    }
  }

  if (!rows.empty()) {
    int count = static_cast<int>(rows.size());
    NotifyItemsAdding(0, count);
    rows_ = std::move(rows);
    NotifyItemsAdded(0, count);
  }
}

void EventTableModel::Update() {
  if (lock_update_) {
    pending_update_ = true;
    return;
  }

  pending_update_ = false;

  CancelRequest();

  if (!rows_.empty()) {
    int count = static_cast<int>(rows_.size());
    NotifyItemsRemoving(0, count);
    rows_.clear();
    NotifyItemsRemoved(0, count);
  }

  historical_events_.clear();

  if (!current_events_) {
    auto [from, to] = GetTimeRangeBounds(time_range_);

    LOG(INFO) << "Query events from " << FormatTime(from).c_str();

    assert(!request_running_);
    request_running_ = true;

    auto weak_ptr = weak_factory_.GetWeakPtr();
    history_service_.HistoryReadEvents(
        scada::id::Server, from, to,
        scada::EventFilter{scada::EventFilter::ACKED},
        BindExecutor(executor_, [this, weak_ptr](
                                    scada::Status status,
                                    std::vector<scada::Event> events) {
          if (weak_ptr.get())
            OnHistoryReadEventsCompleted(std::move(status), std::move(events));
        }));
  }

  RefilterNow();
}

void EventTableModel::CancelRequest() {
  weak_factory_.InvalidateWeakPtrs();
  request_running_ = false;
}

void EventTableModel::OnHistoryReadEventsCompleted(
    scada::Status&& status,
    std::vector<scada::Event>&& events) {
  assert(request_running_);
  // Only acked events were requested.
  assert(std::all_of(events.begin(), events.end(),
                     [](const scada::Event& event) { return event.acked; }));

  historical_events_.assign(std::make_move_iterator(events.begin()),
                            std::make_move_iterator(events.end()));

  request_running_ = false;
  RefilterNow();
}

void EventTableModel::AcknowledgeRow(int row) {
  Row& r = rows_[row];
  switch (r.type) {
    case CURRENT_EVENT:
      assert(!r.event->acked);
      node_event_provider_.AcknowledgeEvent(r.event->acknowledge_id);
      break;

    case HISTORICAL_EVENT:
      // Do nothing.
      break;

    case LOCAL_EVENT:
      local_events_.AcknowledgeEvent(r.event->acknowledge_id);
      break;

    default:
      assert(false);
      break;
  }
}

void EventTableModel::LockUpdate() {
  assert(!lock_update_);
  lock_update_ = true;
}

void EventTableModel::UnlockUpdate() {
  assert(lock_update_);
  lock_update_ = false;
  if (pending_update_)
    Update();
}

std::u16string EventTableModel::MakeTitle() const {
  std::u16string title;
  if (current_events_) {
    title = u"Текущие события";
  } else {
    switch (time_range_.type) {
      case TimeRange::Type::Day:
        title = u"Журнал событий за день";
        break;
      case TimeRange::Type::Week:
        title = u"Журнал событий за неделю";
        break;
      case TimeRange::Type::Month:
        title = u"Журнал событий за месяц";
        break;
      case TimeRange::Type::Custom:
      default:
        title = u"Журнал событий";  // TODO: Format time range.
        break;
    }
  }

  if (severity_min_ || !filter_node_ids_.empty())
    title += u" (Фильтр)";

  return title;
}

bool EventTableModel::IsWorking() const {
  if (current_events_)
    return current_event_model_.acking();
  else
    return request_running_;
}

int EventTableModel::CompareCells(int row1, int row2, int column_id) {
  const auto& event1 = *rows_[row1].event;
  const auto& event2 = *rows_[row2].event;

  switch (column_id) {
    case EventColumnTime:
      return Compare(event1.time, event2.time);
    case EventColumnAckTime:
      return Compare(event1.acknowledged_time, event2.acknowledged_time);
    default:
      return aui::TableModel::CompareCells(row1, row2, column_id);
  }
}
