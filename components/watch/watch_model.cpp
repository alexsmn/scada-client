#include "components/watch/watch_model.h"

#include "aui/translation.h"
#include "base/format_time.h"
#include "base/utf_convert.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"

#include <fstream>

namespace {

static const int kHighWaterMarkLines = 10000;
static const int kLowWaterMarkLines = 9000;

scada::DateTime GetEventTime(const scada::Event& event) {
  return event.time;
}

}  // namespace

// WatchModel

WatchModel::WatchModel(WatchModelContext&& context)
    : WatchModelContext{std::move(context)} {}

void WatchModel::OnEvent(const scada::Event& event) {
  if (!paused_)
    AddLine(event);
}

void WatchModel::OnError(const scada::Status& status) {
  scada::Event event;
  event.message = Translate("Subscription interrupted. The device may have been deleted.");
  AddLine(event);
}

void WatchModel::AddLine(const scada::Event& event) {
  assert(!event.time.is_null());

  // Event time never changes.
  auto same_time_events =
      std::ranges::equal_range(events_, event.time, std::less{}, &GetEventTime);

  for (scada::Event& e : same_time_events) {
    if (e.event_id == event.event_id) {
      if (e.receive_time < event.receive_time ||
          (e.receive_time == event.receive_time && e != event)) {
        e = event;
        int index = static_cast<int>(&e - events_.data());
        NotifyItemsChanged(index, 1);
      }
      return;
    }
  }

  int index = std::ranges::upper_bound(events_, event.time, std::less{},
                                       &GetEventTime) -
              events_.begin();
  NotifyItemsAdding(index, 1);
  events_.emplace(events_.begin() + index, event);
  NotifyItemsAdded(index, 1);

  if (events_.size() > kHighWaterMarkLines) {
    int delete_count = static_cast<int>(events_.size()) - kLowWaterMarkLines;
    NotifyItemsRemoving(0, delete_count);
    events_.erase(events_.begin(), events_.begin() + delete_count);
    NotifyItemsRemoved(0, delete_count);
  }
}

void WatchModel::SetDevice(NodeRef device) {
  if (device_ == device) {
    return;
  }

  device_ = std::move(device);

  Clear();

  event_source_.Start(
      device_.node_id(),
      ToDateTimeRangeWithOpenRange(time_range_, /*now=*/base::Time::Now()),
      /*delegate=*/*this);
}

void WatchModel::SetTimeRange(const TimeRange& time_range) {
  if (time_range_ == time_range) {
    return;
  }

  time_range_ = time_range;

  Clear();

  event_source_.Start(
      device_.node_id(),
      ToDateTimeRangeWithOpenRange(time_range_, /*now=*/base::Time::Now()),
      /*delegate=*/*this);
}

void WatchModel::SaveLog(const std::filesystem::path& path) {
  std::ofstream str(path);
  for (int i = 0; i < GetRowCount(); ++i) {
    for (int j = 0; j < 3; j++) {
      std::string text = UtfConvert<char>(GetCellText(i, j));
      if (j)
        str << '\t';
      str << text.c_str();
    }
    str << '\n';
  }
}

int WatchModel::GetRowCount() {
  return static_cast<int>(events_.size());
}

void WatchModel::GetCell(aui::TableCell& cell) {
  const scada::Event& event = events_[cell.row];

  // TODO: Unify with GetEventColors().
  if (event.severity >= scada::kSeverityCritical) {
    cell.cell_color = aui::Rgba{248, 105, 107};
  } else if (event.severity >= scada::kSeverityWarning) {
    cell.cell_color = aui::Rgba{255, 235, 132};
  }

  switch (cell.column_id) {
    case 0:
      cell.text = UtfConvert<char16_t>(
          FormatTime(event.time, TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
      break;

    case 1:
      if (event.node_id.is_null())
        break;
      cell.text = GetDisplayName(node_service_, event.node_id);
      if (cell.text.empty())
        cell.text = u"?";
      break;

    case 2:
      cell.text = event.message;
      break;
  }
}

void WatchModel::Clear() {
  int row_count = GetRowCount();
  if (row_count == 0) {
    return;
  }

  NotifyItemsRemoving(0, row_count);
  events_.clear();
  NotifyItemsRemoved(0, row_count);
}
