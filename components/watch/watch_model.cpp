#include "components/watch/watch_model.h"

#include "base/format_time.h"
#include "base/strings/sys_string_conversions.h"
#include "core/event_service.h"
#include "core/monitored_item_service.h"
#include "model/devices_node_ids.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"

#include <fstream>

static const int kMaxLines = 10000;

// WatchModel

WatchModel::WatchModel(WatchModelContext&& context)
    : WatchModelContext{std::move(context)} {}

void WatchModel::OnEvent(const scada::Event& event) {
  if (!paused_)
    AddLine(event);
}

void WatchModel::OnError(const scada::Status& status) {
  scada::Event event;
  event.message = L"Подписка прервана. Возможно, устройство было удалено.";
  AddLine(event);
}

void WatchModel::AddLine(const scada::Event& event) {
  int index = static_cast<int>(events_.size());
  NotifyItemsAdding(index, 1);
  events_.push_back(event);
  NotifyItemsAdded(index, 1);

  if (events_.size() > kMaxLines) {
    int delete_count = static_cast<int>(events_.size()) - kMaxLines;
    NotifyItemsRemoving(0, delete_count);
    events_.erase(events_.begin(), events_.begin() + delete_count);
    NotifyItemsRemoved(0, delete_count);
  }
}

void WatchModel::SetDevice(NodeRef device) {
  if (device_ == device)
    return;

  monitored_item_.reset();

  device_ = std::move(device);
  if (!device_)
    return;

  monitored_item_ =
      node_service_.GetNode(scada::id::Server)
          .CreateMonitoredItem(
              scada::AttributeId::EventNotifier,
              scada::MonitoringParameters{}.set_filter(
                  scada::EventFilter{}
                      .add_of_type(devices::id::DeviceWatchEventType)
                      .add_child_of(device_.node_id())));
  if (!monitored_item_)
    return OnError(scada::StatusCode::Bad);

  // FIXME: Captures |this|. No sync.
  monitored_item_->Subscribe(
      [this](const scada::Status& status, const std::any& event) {
        if (!status)
          return OnError(status);
        auto* system_event = std::any_cast<scada::Event>(&event);
        assert(system_event);
        if (system_event)
          OnEvent(*system_event);
      });
}

void WatchModel::SaveLog(const std::filesystem::path& path) {
  std::ofstream str(path);
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

  // TODO: Unify with GetEventColors().
  if (event.severity >= scada::kSeverityCritical) {
    cell.cell_color = SkColorSetRGB(248, 105, 107);
  } else if (event.severity >= scada::kSeverityWarning) {
    cell.cell_color = SkColorSetRGB(255, 235, 132);
  }

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
  NotifyItemsRemoving(0, row_count);
  events_.clear();
  NotifyItemsRemoved(0, row_count);
}
