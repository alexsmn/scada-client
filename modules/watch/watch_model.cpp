#include "modules/watch/watch_model.h"

#include "aui/translation.h"
#include "base/format_time.h"
#include "base/utf_convert.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"

#include <algorithm>
#include <fstream>

namespace {

static const int kHighWaterMarkLines = 10000;
static const int kLowWaterMarkLines = 9000;

scada::Time GetEventTime(const WatchModel::Row& row) {
  return row.event.time;
}

// Whether `event` belongs in `mode`. The frame trace narrows the same stream to
// the lines the drivers marked as protocol traffic.
bool IsVisibleInMode(const WatchModel::Row& row, WatchMode mode) {
  if (mode == WatchMode::kLog)
    return true;
  // A structured frame is traffic by construction. Without one, fall back to
  // the message marker so a server that predates DeviceFrameEventType still
  // produces a usable trace.
  if (row.frame)
    return true;
  return IsProtocolTraffic(ClassifyDeviceLogLine(row.event.message.text));
}

}  // namespace

// WatchModel

WatchModel::WatchModel(WatchModelContext&& context)
    : WatchModelContext{std::move(context)} {}

void WatchModel::OnEvent(const scada::Event& event) {
  if (!paused_)
    AddLine(Row{.event = event});
}

void WatchModel::OnDeviceFrame(const scada::DeviceFrameEvent& event) {
  if (!paused_)
    AddLine(Row{.event = event.base, .frame = event.frame});
}

void WatchModel::OnError(const scada::Status& status) {
  scada::Event event;
  event.message =
      Translate("Subscription interrupted. The device may have been deleted.");
  AddLine(Row{.event = std::move(event)});
}

void WatchModel::AddLine(Row row) {
  const scada::Event& event = row.event;
  // Events originate from the server (or local error lines); a null time is
  // tolerated by the ordered insert below.

  // Event time never changes.
  auto same_time_events =
      std::ranges::equal_range(events_, event.time, std::less{}, &GetEventTime);

  for (Row& r : same_time_events) {
    scada::Event& e = r.event;
    if (e.event_id == event.event_id) {
      if (e.receive_time < event.receive_time ||
          (e.receive_time == event.receive_time && e != event)) {
        r = row;
        int index = static_cast<int>(&r - events_.data());
        NotifyItemsChanged(index, 1);
      }
      return;
    }
  }

  int index = std::ranges::upper_bound(events_, event.time, std::less{},
                                       &GetEventTime) -
              events_.begin();

  // `visible_` holds indices into `events_`, so an insert shifts every index at
  // or after it. Done before the row notification so the model is consistent
  // when the view reads back.
  for (int& i : visible_) {
    if (i >= index)
      ++i;
  }

  const bool visible = IsVisibleInMode(row, mode_);
  int visible_index = 0;
  if (visible) {
    visible_index = static_cast<int>(
        std::lower_bound(visible_.begin(), visible_.end(), index) -
        visible_.begin());
    NotifyItemsAdding(visible_index, 1);
  }

  events_.emplace(events_.begin() + index, std::move(row));

  if (visible) {
    visible_.insert(visible_.begin() + visible_index, index);
    NotifyItemsAdded(visible_index, 1);
  }

  if (events_.size() > kHighWaterMarkLines) {
    int delete_count = static_cast<int>(events_.size()) - kLowWaterMarkLines;
    // Only the *visible* rows among the trimmed ones are rows as far as the
    // view is concerned.
    int visible_deleted = static_cast<int>(
        std::lower_bound(visible_.begin(), visible_.end(), delete_count) -
        visible_.begin());
    if (visible_deleted > 0)
      NotifyItemsRemoving(0, visible_deleted);
    events_.erase(events_.begin(), events_.begin() + delete_count);
    visible_.erase(visible_.begin(), visible_.begin() + visible_deleted);
    for (int& i : visible_)
      i -= delete_count;
    if (visible_deleted > 0)
      NotifyItemsRemoved(0, visible_deleted);
  }
}

void WatchModel::RebuildVisible() {
  visible_.clear();
  for (int i = 0; i < static_cast<int>(events_.size()); ++i) {
    if (IsVisibleInMode(events_[i], mode_))
      visible_.push_back(i);
  }
}

const WatchModel::Row& WatchModel::VisibleRow(int row) const {
  return events_[visible_[row]];
}

DeviceLogDirection WatchModel::DirectionOf(const Row& row) const {
  if (row.frame) {
    switch (row.frame->direction) {
      case scada::DeviceFrame::kInbound:
        return DeviceLogDirection::kInbound;
      case scada::DeviceFrame::kOutbound:
        return DeviceLogDirection::kOutbound;
      default:
        return DeviceLogDirection::kNone;
    }
  }
  return ClassifyDeviceLogLine(row.event.message.text).direction;
}

void WatchModel::SetMode(WatchMode mode) {
  if (mode_ == mode)
    return;

  // Switching mode changes which rows exist, not their contents. Notify the
  // removal of the old row set and the arrival of the new one rather than
  // trying to diff two filters — the log is bounded, and a mode switch is a
  // deliberate operator action, not something that happens per event.
  int old_rows = GetRowCount();
  if (old_rows > 0)
    NotifyItemsRemoving(0, old_rows);
  mode_ = mode;
  RebuildVisible();
  if (old_rows > 0)
    NotifyItemsRemoved(0, old_rows);

  int new_rows = GetRowCount();
  if (new_rows > 0) {
    NotifyItemsAdding(0, new_rows);
    NotifyItemsAdded(0, new_rows);
  }
}

void WatchModel::SetDevice(NodeRef device) {
  if (device_ == device) {
    return;
  }

  device_ = std::move(device);

  Clear();

  event_source_.Start(device_.node_id(),
                      scada::ToTimeRangeWithOpenRange(
                          time_range_, /*now=*/scada::Now()),
                      /*delegate=*/*this);
}

void WatchModel::SetTimeRange(const scada::RelativeTimeRange& time_range) {
  if (time_range_ == time_range) {
    return;
  }

  time_range_ = time_range;

  Clear();

  event_source_.Start(device_.node_id(),
                      scada::ToTimeRangeWithOpenRange(
                          time_range_, /*now=*/scada::Now()),
                      /*delegate=*/*this);
}

void WatchModel::SaveLog(const std::filesystem::path& path) {
  std::ofstream str(path);
  for (int i = 0; i < GetRowCount(); ++i) {
    for (int j = 0; j < 7; j++) {
      std::string text = UtfConvert<char>(GetCellText(i, j));
      if (j)
        str << '\t';
      str << text;
    }
    str << '\n';
  }
}

int WatchModel::GetRowCount() {
  return static_cast<int>(visible_.size());
}

void WatchModel::GetCell(scada::aui::TableCell& cell) {
  const Row& row = VisibleRow(cell.row);
  const scada::Event& event = row.event;

  // TODO: Unify with GetEventColors().
  if (event.severity >= scada::kSeverityCritical) {
    cell.cell_color = scada::aui::Rgba{248, 105, 107};
  } else if (event.severity >= scada::kSeverityWarning) {
    cell.cell_color = scada::aui::Rgba{255, 235, 132};
  }

  switch (cell.column_id) {
    case 0:
      cell.text = UtfConvert<char16_t>(
          FormatTime(event.time, TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
      break;

    case 1:
      if (event.source_node_id.is_null())
        break;
      cell.text = GetDisplayName(node_service_, event.source_node_id).text;
      if (cell.text.empty())
        cell.text = u"?";
      break;

    case 2: {
      // The direction marker is chrome: it is shown in its own column (3), so
      // the message reads without it in both modes.
      cell.text = std::u16string{ClassifyDeviceLogLine(event.message.text).text};
      break;
    }

    case 3:
      switch (DirectionOf(row)) {
        case DeviceLogDirection::kInbound:
          cell.text = Translate("RX");
          break;
        case DeviceLogDirection::kOutbound:
          cell.text = Translate("TX");
          break;
        case DeviceLogDirection::kNone:
          break;
      }
      break;

    // The decoded frame columns. Blank unless the server sent structured frame
    // data — an ordinary log line has none, and neither does any server older
    // than DeviceFrameEventType. Zero means "not applicable" rather than a real
    // value: IOA 0 is not a valid object address and type/cause 0 are unused.
    case 4:
      if (row.frame && row.frame->type_id != 0)
        cell.text = UtfConvert<char16_t>(std::to_string(row.frame->type_id));
      break;

    case 5:
      if (row.frame && row.frame->cause != 0)
        cell.text = UtfConvert<char16_t>(std::to_string(row.frame->cause));
      break;

    case 6:
      if (row.frame && row.frame->object_address != 0) {
        cell.text =
            UtfConvert<char16_t>(std::to_string(row.frame->object_address));
      }
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
  visible_.clear();
  NotifyItemsRemoved(0, row_count);
}
