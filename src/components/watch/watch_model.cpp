#include "client/components/watch/watch_model.h"

#include <fstream>

#include "base/format_time.h"
#include "base/strings/sys_string_conversions.h"
#include "core/monitored_item.h"
#include "core/monitored_item_service.h"

static const int kMaxLines = 1000;

// WatchModel

WatchModel::WatchModel(scada::MonitoredItemService& monitored_item_service)
    : monitored_item_service_(monitored_item_service) {
}

void WatchModel::OnEvent(const scada::Event& event) {
  if (paused_)
    return;

  int index = static_cast<int>(events_.size());
  events_.push_back(event);
  NotifyItemsAdded(index, 1);

  if (events_.size() > kMaxLines) {
    int delete_count = static_cast<int>(events_.size()) - kMaxLines;
    events_.erase(events_.begin(), events_.begin() + delete_count);
    NotifyItemsRemoved(0, delete_count);
  }
}

void WatchModel::SetDevice(NodeRef device) {
  if (device_ == device)
    return;

  monitored_item_.reset();

  device_ = std::move(device);

  if (device_) {
    monitored_item_ = monitored_item_service_.CreateMonitoredItem(device_.id(), OpcUa_Attributes_EventNotifier);
    monitored_item_->set_event_handler([this](const scada::Event& event) { OnEvent(event); });
    monitored_item_->Subscribe();
  }
}

void WatchModel::SaveLog(const base::FilePath& path) {
  std::ofstream str(path.value().c_str());
  for (int i = 0; i < GetRowCount(); ++i) {
    for (int j = 0; j < 3; j++) {
      std::string text = base::SysWideToNativeMB(GetCellText(i, j));
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

void WatchModel::GetCell(ui::TableCell& cell) {
  const scada::Event& event = events_[cell.row];

  switch (cell.column_id) {
    case 0:
      cell.text = base::SysNativeMBToWide(
          FormatTime(event.time, TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
      break;
      
    case 1:
      if (event.node_id.is_null())
        break;
      cell.text = base::SysNativeMBToWide(device_.browse_name());
      if (cell.text.empty())
        cell.text = L"?";
      break;
      
    case 2:
      cell.text = event.message;
      break;
  }
}

void WatchModel::Clear() {
  int row_count = GetRowCount();
  events_.clear();
  NotifyItemsRemoved(0, row_count);
}
