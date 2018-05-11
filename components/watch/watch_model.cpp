#include "components/watch/watch_model.h"

#include "base/format_time.h"
#include "base/strings/sys_string_conversions.h"
#include "common/node_util.h"
#include "core/event_service.h"
#include "core/monitored_item.h"

#include <fstream>

static const int kMaxLines = 10000;

// WatchModel

WatchModel::WatchModel(WatchModelContext&& context)
    : WatchModelContext{std::move(context)} {}

void WatchModel::OnEvent(const scada::Status& status,
                         const scada::Event& event) {
  if (!status) {
    scada::Event event;
    event.message = L"Подписка прервана. Возможно, устройство было удалено.";
    AddLine(event);

  } else if (!paused_) {
    AddLine(event);
  }
}

void WatchModel::AddLine(const scada::Event& event) {
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
    monitored_item_ =
        device_.CreateMonitoredItem(scada::AttributeId::EventNotifier);
    monitored_item_->set_event_handler(
        [this](const scada::Status& status, const scada::Event& event) {
          OnEvent(status, event);
        });
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
      cell.text = GetDisplayName(node_service_, event.node_id);
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
