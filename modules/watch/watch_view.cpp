#include "modules/watch/watch_view.h"

#include "aui/dialog_service.h"
#include "aui/table.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/u16format.h"
#include "base/utf_convert.h"
#include "controller/controller_delegate.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "modules/watch/frame_decode.h"
#include "modules/watch/frame_decode_tree_model.h"
#include "modules/watch/watch_model.h"
#include "modules/watch/watch_model_builder.h"
#include "net/net_executor_adapter.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "profile/window_definition.h"
#include "resources/common_resources.h"
#include "services/frame_capture_registry.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <optional>
#include <string>

#if defined(UI_QT)
#include "modules/watch/qt/frame_decode_pane.h"
#include "modules/watch/qt/watch_filter_bar.h"
#include "node_properties/device_address_map.h"

#include <QHBoxLayout>
#include <QPointer>
#include <QSplitter>
#include <QVBoxLayout>

#include <charconv>
#endif

namespace {

Awaitable<void> SaveLogAsync(AnyExecutor executor,
                             DialogService& dialog_service,
                             std::shared_ptr<WatchModel> model,
                             std::u16string name) {
  auto path =
      co_await dialog_service.SelectSaveFile({Translate("Save As"), name});
  model->SaveLog(path);
  co_return;
}

}  // namespace

// WatchView

WatchView::WatchView(const ControllerContext& context)
    : ControllerContext{context},
      model_{WatchModelBuilder{executor_, context.node_service_}
                 .CreateWatchModel()} {}

WatchView::~WatchView() {
  // A capture must not outlive the view that armed it: the operator has closed
  // the only thing that was reading the frames.
  if (capture_armed_)
    SetCaptureArmed(false);
}

// The device's arming switch lives in its address space
// (devices::id::DeviceType_FrameCapture), so this is an ordinary value write.
void WatchView::SetCaptureArmed(bool armed) {
  if (capture_armed_ == armed)
    return;
  capture_armed_ = armed;

  const NodeRef device = model_->device();
  frame_capture_registry_.SetArmed(
      device.node_id(), ToString16(device.display_name()), armed);

  if (NodeRef capture = device[scada::devices::id::DeviceType_FrameCapture]) {
    // Fire and forget. A server too old to know the variable answers
    // Bad_WrongNodeId, and an operator who opened a trace must not get a modal
    // over it — the trace is simply empty, which is the visible symptom
    // anyway.
    CoSpawn(executor_, [node = capture.scada_node(),
                        armed]() mutable -> Awaitable<void> {
      co_await node.write_value(armed);
      co_return;
    });
  }
}

void WatchView::Save(WindowDefinition& definition) {
  WindowItem& item = definition.AddItem("Item");
  item.SetString("path", NodeIdToScadaString(model_->device().node_id()));
  SaveTimeRange(definition, model_->time_range());
}

std::u16string WatchView::MakeTitle() const {
  std::u16string title = ToString16(model_->device().display_name());
  // The mode is part of the view's identity: a frame trace and a device log
  // are answering different questions about the same device.
  if (model_->mode() == WatchMode::kFrameTrace)
    title += u" \u2014 " + Translate("Frame trace");
  if (model_->paused())
    title += u" [Pause]";
  return title;
}

std::unique_ptr<UiView> WatchView::Init(const WindowDefinition& definition) {
  const scada::aui::TableColumn columns[] = {
      {0, Translate("Time"), 100, scada::aui::TableColumn::LEFT,
       scada::aui::TableColumn::DataType::DateTime},
      // Direction sits beside the time, as in the frame-trace mockup. It is
      // present in both modes rather than swapped in with the mode, because
      // aui::Table fixes its columns at construction — and because knowing
      // which log lines are protocol traffic is useful in the log too. It is
      // simply blank for unmarked lines.
      {3, Translate("Dir"), 50, scada::aui::TableColumn::LEFT},
      // Decoded frame fields, blank for ordinary log lines. Permanent for the
      // same reason as Dir: aui::Table fixes its columns at construction, so
      // they cannot be swapped in with the mode.
      {4, Translate("Type ID"), 70, scada::aui::TableColumn::RIGHT},
      {5, Translate("Cause"), 60, scada::aui::TableColumn::RIGHT},
      {6, Translate("IOA"), 70, scada::aui::TableColumn::RIGHT},
      {7, Translate("Fmt"), 45, scada::aui::TableColumn::LEFT},
      {8, Translate("N(S)/N(R)"), 90, scada::aui::TableColumn::RIGHT},
      {1, Translate("Device"), 100, scada::aui::TableColumn::LEFT},
      {2, Translate("Event"), 400, scada::aui::TableColumn::LEFT},
  };

  if (const WindowItem* item = definition.FindItem("Item")) {
    auto path = item->GetString("path");
    auto device_id = NodeIdFromScadaString(path);
    model_->SetDevice(node_service_.GetNode(device_id));
  }

  if (auto time_range = RestoreTimeRange(definition)) {
    model_->SetTimeRange(*time_range);
  }

  table_ =
      new scada::aui::Table(model_, {columns, columns + std::size(columns)});

  table_->SetSelectionChangeHandler([this] {
    auto_scroll_ = table_->GetCurrentRow() == model_->GetRowCount() - 1;
    refresh_decode_pane_();
  });

  table_->SetContextMenuHandler([this](const scada::aui::Point& point) {
    // Cross-platform AUI menu model (Windows, macOS, Wt) instead of the
    // Windows-only `IDR_LOG_POPUP` resource menu.
    controller_delegate_.ShowPopupMenu(&watch_menu_model_.model(), point, true);
  });

  // Must be after |table_| is bound.
  items_added_connection_ = model_->SubscribeItemsAdded(
      [this](int first, int count) { OnItemsAdded(first, count); });

  command_registry_.AddCommand(
      Command{ID_PAUSE}
          .set_execute_handler([this] {
            model_->set_paused(!model_->paused());
            controller_delegate_.SetTitle(MakeTitle());
          })
          .set_checked_handler([this] { return model_->paused(); }));

  command_registry_.AddCommand(
      Command{ID_WATCH_FRAME_TRACE}
          .set_execute_handler([this] { ToggleFrameTrace(); })
          .set_checked_handler([this] {
            return model_->mode() == WatchMode::kFrameTrace;
          }));

  command_registry_.AddCommand(
      Command{ID_SAVE_AS}.set_execute_handler([this] { SaveLog(); }));

  command_registry_.AddCommand(Command{ID_CLEAR_ALL}.set_execute_handler(
      [this] {
        model_->Clear();
        refresh_decode_pane_();
      }));

#if defined(UI_QT)
  return CreateFrameTraceLayout();
#else
  return std::unique_ptr<UiView>{table_->CreateParentIfNecessary()};
#endif
}

#if defined(UI_QT)

// The trace beside the decode pane, as in
// docs/ui-mockups/screens/device-protocol-trace.html: the selected frame's
// octets and its decoded field tree. The pane belongs to the frame trace, so
// it is hidden in the ordinary device log — a permanently empty inspector
// would be a regression for the common case, which is reading log lines.
//
// Composition is Qt-only, the house pattern for this (see table_view.cpp,
// event_view.cpp): aui has no cross-platform splitter, and the Wt frontend
// keeps the bare trace it has today.
std::unique_ptr<UiView> WatchView::CreateFrameTraceLayout() {
  auto* pane = new FrameDecodePane;

  refresh_decode_pane_ = [this, pane] {
    const bool tracing = model_->mode() == WatchMode::kFrameTrace;
    pane->setVisible(tracing);
    // Here rather than in ToggleFrameTrace so every route into the trace —
    // toggling it, and the first selection made in it — arms the browse.
    if (tracing)
      EnsureAddressMap(pane);

    const int row = table_->GetCurrentRow();
    const WatchModel::Row* selected = model_->FindVisibleRow(row);
    if (!selected || !selected->frame) {
      pane->Clear();
      return;
    }
    pane->ShowFrame(MakeDecodeHeader(row, *selected->frame), *selected->frame);
  };
  refresh_decode_pane_();

  auto* splitter = new QSplitter{Qt::Horizontal};
  splitter->addWidget(table_);
  splitter->addWidget(pane);
  // The trace is the subject; the pane is detail about one row of it.
  splitter->setStretchFactor(0, 1);
  splitter->setStretchFactor(1, 0);

  auto* container = new QWidget;
  auto* layout = new QVBoxLayout{container};
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  // Shown in both modes, because the model filters in both: a control that
  // keeps applying while hidden is a trap, and text-filtering a device log is
  // useful in itself.
  layout->addWidget(CreateWatchFilterBar([this](WatchFilter filter) {
    model_->SetFilter(std::move(filter));
    // Filtering changes which row is selected, and so what the pane shows.
    refresh_decode_pane_();
  }));
  layout->addWidget(splitter, 1);
  return std::unique_ptr<UiView>{container};
}

void WatchView::EnsureAddressMap(FrameDecodePane* pane) {
  if (address_map_requested_)
    return;
  address_map_requested_ = true;

  // QPointer, like the device-parameter form's address-map preview: the browse
  // outlives a view the operator closes while it is still running.
  CoSpawn(executor_, [executor = executor_, device = model_->device(),
                      pane_ptr = QPointer<FrameDecodePane>{
                          pane}]() mutable -> Awaitable<void> {
    std::vector<AddressMapRow> rows =
        co_await BuildDeviceAddressMap(executor, std::move(device));
    if (!pane_ptr)
      co_return;

    std::vector<FrameObjectMapping> mappings;
    mappings.reserve(rows.size());
    for (const AddressMapRow& row : rows) {
      // AddressMapRow carries the address as display text; the decoder works
      // in numbers. A row whose address does not parse is dropped rather than
      // mapped to 0, which is a real IOA value.
      const std::string address = UtfConvert<char>(row.ioa);
      scada::Int32 object_address = 0;
      const auto parsed =
          std::from_chars(address.data(), address.data() + address.size(),
                          object_address);
      if (parsed.ec != std::errc{} || parsed.ptr != address.data() + address.size())
        continue;
      mappings.push_back({.object_address = object_address,
                          .signal = row.signal,
                          .node_id = row.node_id});
    }
    // Unconditionally, including an empty map: that is what tells the pane the
    // map has been read, so it can say an address is unmapped.
    pane_ptr->SetAddressMap(std::move(mappings));
    co_return;
  });
}

// The pane's title line: which frame, from when, and how big. The direction and
// time are read back out of the table rather than reformatted, so the pane
// cannot disagree with the row it is describing.
std::u16string WatchView::MakeDecodeHeader(
    int row,
    const scada::DeviceFrame& frame) const {
  std::u16string header = model_->GetCellText(row, 3);
  if (!header.empty())
    header += u" · ";
  header += model_->GetCellText(row, 0);
  header += u" · " +
            UtfConvert<char16_t>(std::to_string(frame.raw_data.size())) + u" " +
            Translate("bytes");
  return header;
}

#endif  // defined(UI_QT)

void WatchView::ToggleFrameTrace() {
  model_->SetMode(model_->mode() == WatchMode::kFrameTrace
                      ? WatchMode::kLog
                      : WatchMode::kFrameTrace);
  // Entering the trace is exactly when the frames become worth producing, and
  // leaving it is when they stop being. Here rather than in the Qt-only pane
  // wiring so the Wt frontend arms too.
  SetCaptureArmed(model_->mode() == WatchMode::kFrameTrace);
  controller_delegate_.SetTitle(MakeTitle());
  refresh_decode_pane_();
}

void WatchView::SaveLog() {
  auto time = boost::posix_time::second_clock::local_time();
  auto date = time.date();
  auto time_of_day = time.time_of_day();

  auto name = u16format(
      L"{:04}{:02}{:02}_{:02}{:02}{:02}.log", static_cast<int>(date.year()),
      static_cast<int>(date.month()), static_cast<int>(date.day()),
      static_cast<int>(time_of_day.hours()),
      static_cast<int>(time_of_day.minutes()),
      static_cast<int>(time_of_day.seconds()));

  CoSpawn(executor_, [executor = executor_, &dialog_service = dialog_service_,
                      model = model_, name = std::move(name)] {
    return SaveLogAsync(executor, dialog_service, model, name);
  });
}

CommandHandler* WatchView::GetCommandHandler(unsigned command_id) {
  return command_registry_.GetCommandHandler(command_id);
}

void WatchView::OnItemsAdded(int first, int count) {
  if (!auto_scroll_)
    return;

  auto last_row = first + count - 1;
  table_->SelectRow(last_row);
}

ExportModel::ExportData WatchView::GetExportData() {
  return TableExportData{*model_, table_->columns()};
}

scada::RelativeTimeRange WatchView::GetTimeRange() const {
  return model_->time_range();
}

void WatchView::SetTimeRange(const scada::RelativeTimeRange& time_range) {
  model_->SetTimeRange(time_range);
}
