#include "components/events/event_table_model.h"

#include "base/format_time.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/utils.h"
#include "base/excel.h"
#include "common_resources.h"
#include "common/scada_node_ids.h"
#include "common/event_manager.h"
#include "core/data_value.h"
#include "ui/base/models/grid_range.h"
#include "common/node_ref_service.h"
#include "common/node_ref_format.h"
#include "common/node_ref_util.h"
#include "translation.h"

const base::char16 kLocalEventSource[] = L"Локальное событие";

struct EventTableModel::RowsComparer {
  bool operator()(const Row& left, const Row& right) const {
    // desc by time
    if (left.event->time != right.event->time)
      return left.event->time > right.event->time;
    // asct by severity
    if (left.event->severity != right.event->severity)
      return left.event->severity < right.event->severity;
    // asc by source
    if (left.event->node_id != right.event->node_id)
      return left.event->node_id < right.event->node_id;
    // TODO: sort by object name
    return left.event < right.event;
  }
};

static void GetEventColors(const scada::Event& event, SkColor& text_color,
                           SkColor& back_color) {
  if (!event.acked) {
    back_color = SkColorSetRGB(99, 190, 123);
  } else if (event.severity >= scada::kSeverityCritical) {
    back_color = SkColorSetRGB(248, 105, 107);
  } else if (event.severity >= scada::kSeverityWarning) {
    back_color = SkColorSetRGB(255, 235, 132);
  }
}

EventTableModel::EventTableModel(EventTableModelContext&& context)
    : EventTableModelContext(std::move(context)),
      mode_(ID_CURRENT_EVENTS) {
  local_events_.observers().AddObserver(this);
  event_manager_.AddObserver(*this);
  node_service_.AddObserver(*this);
}

EventTableModel::~EventTableModel() {
  CancelRequest();

  node_service_.RemoveObserver(*this);
  event_manager_.RemoveObserver(*this);
  local_events_.observers().RemoveObserver(this);
}

void EventTableModel::Init(Mode mode, const TimeRange& range, ItemIds filter_items) {
  mode_ = mode;
  time_range_ = range;
  filter_node_ids_ = std::move(filter_items);
  Update();
}

int EventTableModel::GetRowCount() {
  return static_cast<int>(rows_.size());
}

void EventTableModel::GetCell(ui::TableCell& cell) {
  const Row& row = rows_[cell.row];
  const scada::Event& event = *row.event;

  GetEventColors(event, cell.text_color, cell.cell_color);

  switch (cell.column_id) {
    case EventColumnTime:
      cell.text = base::SysNativeMBToWide(FormatTime(event.time,
          TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
      break;
    case EventColumnSeverity:
  		cell.text = base::IntToString16(event.severity);
      break;
    case EventColumnItem:
      if (row.node)
        cell.text = ToString16(row.node.display_name());
      else if (row.type == LOCAL_EVENT)
        cell.text = kLocalEventSource;
      break;
    case EventColumnMessage:
      cell.text = event.message;
      break;
    case EventColumnSource:
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
      cell.text = base::SysNativeMBToWide(FormatTime(event.acknowledged_time,
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
      DCHECK(i->type == CURRENT_EVENT || i->type == LOCAL_EVENT);
      return i - rows_.begin();
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

  return false;
}

int EventTableModel::GetInsertIndex(EventType type, const scada::Event& event) {
  if (!IsEventShown(event))
    return -1;

  Row row(type, event);
  Rows::iterator i = std::upper_bound(rows_.begin(), rows_.end(), row, RowsComparer());
  return i != rows_.end() ? static_cast<int>(i - rows_.begin()) :
                            static_cast<int>(rows_.size());
}

void EventTableModel::AddRow(EventType type, const scada::Event& event) {
  int index = FindRow(event);
  if (index == -1) {
    index = GetInsertIndex(type, event);
    if (index != -1) {
      Row row{type, event};
      row.node = node_service_.GetNode(event.node_id);
      row.user = node_service_.GetNode(event.user_id);
      row.acknowledged_user = node_service_.GetNode(event.acknowledged_user_id);
      rows_.insert(rows_.begin() + index, std::move(row));
      NotifyItemsAdded(index, 1);
    }
  } else {
    // Update row data.
    assert(rows_[index].type == type);
    NotifyItemsChanged(index, 1);
  }
}

void EventTableModel::RemoveRows(int first, int count) {
  rows_.erase(rows_.begin() + first, rows_.begin() + (first + count));
  NotifyItemsRemoved(first, count);
}

void EventTableModel::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  // TODO: check only visible rows
  int index = 0;
  for (Rows::const_iterator i = rows_.begin(); i != rows_.end(); ++i, ++index) {
    const Row& row = *i;
    if (node_id == row.event->node_id)
      NotifyItemsChanged(index, 1);
  }
}

void EventTableModel::OnEventReported(const scada::Event& event) {
  if (IsEventShown(event))
    AddRow(CURRENT_EVENT, event);
}

void EventTableModel::OnEventAcknowledged(const scada::Event& event) {
  int index = FindRow(event);
  if (index != -1)
    AckRows(index, 1);
}

void EventTableModel::OnAllEventsAcknowledged() {
  AckRows(0, static_cast<int>(rows_.size()));
}

void EventTableModel::AckRows(int first, int count) {
  if (mode_ == ID_CURRENT_EVENTS) {
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
    AddRow(LOCAL_EVENT, event);
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
  refilter_delay_timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(300),
      this, &EventTableModel::RefilterNow);
}

void EventTableModel::SetSeverityMin(unsigned severity) {
  if (severity_min_ == severity)
    return;

  severity_min_ = severity;
  RefilterNow();
}

void EventTableModel::RefilterNow() {
  refilter_delay_timer_.Stop();

  rows_.clear();

  const LocalEvents::Events& local_events = local_events_.events();
  for (LocalEvents::Events::const_iterator i = local_events.begin();
                                           i != local_events.end(); ++i) {
    const scada::Event& event = **i;
    rows_.push_back(Row(LOCAL_EVENT, event));
  }

  const auto& events = event_manager_.unacked_events();
  for (auto i = events.begin(); i != events.end(); i++) {
    const scada::Event& event = i->second;
    if (IsEventShown(event))
      rows_.push_back(Row(CURRENT_EVENT, event));
  }

  if (mode_ == ID_CURRENT_EVENTS) {

  } else {
    for (EventContainer::iterator i = historical_events_.begin();
                                  i != historical_events_.end(); ++i) {
      const scada::Event& event = *i;
      if (IsEventShown(event))
        rows_.push_back(Row(HISTORICAL_EVENT, event));
    }
  }

  std::sort(rows_.begin(), rows_.end(), RowsComparer());
  // Mustn't be RangeChanged as rows are reordered.
  NotifyModelChanged();
}

void EventTableModel::Update() {
  if (lock_update_) {
    pending_update_ = true;
    return;
  }

  pending_update_ = false;

  CancelRequest();

  rows_.clear();
  historical_events_.clear();

  if (mode_ == ID_CURRENT_EVENTS) {

  } else {
    base::Time from, to;
    switch (mode_) {
      case ID_TIME_RANGE_DAY:
        from = base::Time::Now().LocalMidnight();
        break;
      case ID_TIME_RANGE_WEEK: {
        base::Time cur = base::Time::Now().LocalMidnight();
        base::Time::Exploded ts = {0};
        cur.LocalExplode(&ts);
        // We need to start day of week from Monday instead of Sunday.
        unsigned day_of_week = (ts.day_of_week + 6) % 7;
        from = cur - base::TimeDelta::FromDays(day_of_week);
        break;
      }
      case ID_TIME_RANGE_MONTH: {
        base::Time::Exploded ts;
        base::Time::Now().LocalMidnight().LocalExplode(&ts);
        ts.day_of_month = 1;
        from = base::Time::FromLocalExploded(ts);
        break;
      }
      case ID_TIME_RANGE_CUSTOM:
        from = time_range().start.time;
        if (time_range().start.date_only)
          from = from.LocalMidnight();
        to = time_range().end.time;
        if (time_range().end.date_only)
          to = to.LocalMidnight() + base::TimeDelta::FromDays(1);
        if (from >= to) {
          to = base::Time::Now();
          from = to - base::TimeDelta::FromHours(1);
        }
        break;
      default:
        assert(false);
        break;
    }

    LOG(INFO) << "Query events from " << FormatTime(from).c_str();

    request_running_ = true;

    auto runner = base::ThreadTaskRunnerHandle::Get();
    auto weak_ptr = weak_factory_.GetWeakPtr();
    history_service_.HistoryRead({scada::id::RootFolder, scada::AttributeId::EventNotifier}, from, to, {{scada::Event::ACKED}},
        [this, runner, weak_ptr](scada::Status status, scada::QueryValuesResults values, scada::QueryEventsResults events) {
          runner->PostTask(FROM_HERE, base::Bind(&EventTableModel::OnQueryEventsCompleted,
              weak_ptr, base::Passed(std::move(status)), base::Passed(std::move(events))));
        });
  }

  RefilterNow();
}

void EventTableModel::CancelRequest() {
  weak_factory_.InvalidateWeakPtrs();
  request_running_ = false;
}

void EventTableModel::SetMode(unsigned mode) {
  if (mode != ID_TIME_RANGE_CUSTOM && mode_ == mode)
    return;

  mode_ = mode;

  Update();
}

void EventTableModel::OnQueryEventsCompleted(scada::Status status, scada::QueryEventsResults events) {
  DCHECK(request_running_);

  if (events)
    historical_events_.assign(events->begin(), events->end());
  else
    historical_events_.clear();

  request_running_ = false;
  RefilterNow();
}

void EventTableModel::AcknowledgeRow(int row) {
  Row& r = rows_[row];
  switch (r.type) {
    case CURRENT_EVENT:
      assert(!r.event->acked);
      event_manager_.AcknowledgeEvent(r.event->acknowledge_id);
      break;

    case HISTORICAL_EVENT:
      // Do nothing.
      break;

    case LOCAL_EVENT:
      local_events_.AcknowledgeEvent(r.event->acknowledge_id);
      break;

    default:
      DCHECK(false);
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

base::string16 EventTableModel::MakeTitle() const {
  base::string16 title;
  switch (mode_) {
    case ID_TIME_RANGE_DAY:
      title = L"Журнал событий за день";
      break;
    case ID_TIME_RANGE_WEEK:
      title = L"Журнал событий за неделю";
      break;
    case ID_TIME_RANGE_MONTH:
      title = L"Журнал событий за месяц";
      break;
    case ID_TIME_RANGE_CUSTOM:
      title = L"Журнал событий";	// TODO: Format time range.
      break;
    default:
      title = L"Текущие события";
      break;
  }

  if (severity_min_ || !filter_node_ids_.empty())
    title += L" (Фильтр)";

  return title;
}

bool EventTableModel::IsWorking() const {
  if (mode_ == ID_CURRENT_EVENTS)
    return event_manager_.is_acking();
  else
    return request_running_;
}
