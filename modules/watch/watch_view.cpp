#include "modules/watch/watch_view.h"

#include "aui/dialog_service.h"
#include "aui/table.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/u16format.h"
#include "controller/controller_delegate.h"
#include "model/node_id_util.h"
#include "modules/watch/watch_model.h"
#include "modules/watch/watch_model_builder.h"
#include "net/net_executor_adapter.h"
#include "node_service/node_service.h"
#include "profile/window_definition.h"
#include "resources/common_resources.h"

#include <boost/date_time/posix_time/posix_time.hpp>

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

WatchView::~WatchView() = default;

void WatchView::Save(WindowDefinition& definition) {
  WindowItem& item = definition.AddItem("Item");
  item.SetString("path", NodeIdToScadaString(model_->device().node_id()));
  SaveTimeRange(definition, model_->time_range());
}

std::u16string WatchView::MakeTitle() const {
  std::u16string title = ToString16(model_->device().display_name());
  if (model_->paused())
    title += u" [Pause]";
  return title;
}

std::unique_ptr<UiView> WatchView::Init(const WindowDefinition& definition) {
  const scada::aui::TableColumn columns[] = {
      {0, Translate("Time"), 100, scada::aui::TableColumn::LEFT,
       scada::aui::TableColumn::DataType::DateTime},
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
  });

  table_->SetContextMenuHandler([this](const scada::aui::Point& point) {
    // Cross-platform AUI menu model (Windows, macOS, Wt) instead of the
    // Windows-only `IDR_LOG_POPUP` resource menu.
    controller_delegate_.ShowPopupMenu(&watch_menu_model_.model(),
                                       /*resource_id=*/0, point, true);
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
      Command{ID_SAVE_AS}.set_execute_handler([this] { SaveLog(); }));

  command_registry_.AddCommand(
      Command{ID_CLEAR_ALL}.set_execute_handler([this] { model_->Clear(); }));

  return std::unique_ptr<UiView>{table_->CreateParentIfNecessary()};
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

TimeRange WatchView::GetTimeRange() const {
  return model_->time_range();
}

void WatchView::SetTimeRange(const TimeRange& time_range) {
  model_->SetTimeRange(time_range);
}
