#include "components/watch/watch_view.h"

#include "aui/dialog_service.h"
#include "aui/table.h"
#include "base/strings/stringprintf.h"
#include "common_resources.h"
#include "components/watch/watch_model.h"
#include "components/watch/watch_model_builder.h"
#include "controller/controller_delegate.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "profile/window_definition.h"

// WatchView

WatchView::WatchView(const ControllerContext& context)
    : ControllerContext{context},
      model_{WatchModelBuilder{executor_, context.node_service_}
                 .CreateWatchModel()} {}

WatchView::~WatchView() {
  model_->observers().RemoveObserver(this);
}

void WatchView::Save(WindowDefinition& definition) {
  WindowItem& item = definition.AddItem("Item");
  item.SetString("path", NodeIdToScadaString(model_->device().node_id()));
  SaveTimeRange(definition, model_->time_range());
}

std::u16string WatchView::MakeTitle() const {
  std::u16string title = ToString16(model_->device().display_name());
  if (model_->paused())
    title += u" [Пауза]";
  return title;
}

std::unique_ptr<UiView> WatchView::Init(const WindowDefinition& definition) {
  const aui::TableColumn columns[] = {
      {0, u"Время", 100, aui::TableColumn::LEFT,
       aui::TableColumn::DataType::DateTime},
      {1, u"Устройство", 100, aui::TableColumn::LEFT},
      {2, u"Событие", 400, aui::TableColumn::LEFT},
  };

  if (const WindowItem* item = definition.FindItem("Item")) {
    auto path = item->GetString("path");
    auto device_id = NodeIdFromScadaString(path);
    model_->SetDevice(node_service_.GetNode(device_id));
  }

  if (auto time_range = RestoreTimeRange(definition)) {
    model_->SetTimeRange(*time_range);
  }

  table_ = new aui::Table(model_, {columns, columns + _countof(columns)});

  table_->SetSelectionChangeHandler([this] {
    auto_scroll_ = table_->GetCurrentRow() == model_->GetRowCount() - 1;
  });

  table_->SetContextMenuHandler([this](const aui::Point& point) {
    controller_delegate_.ShowPopupMenu(nullptr, IDR_LOG_POPUP, point, true);
  });

  // Must be after |table_| is bound.
  model_->observers().AddObserver(this);

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

promise<> WatchView::SaveLog() {
  SYSTEMTIME time;
  GetLocalTime(&time);

  auto name = base::StringPrintf(u"%04d%02d%02d_%02d%02d%02d.log", time.wYear,
                                 time.wMonth, time.wDay, time.wHour,
                                 time.wMinute, time.wSecond);

  return dialog_service_.SelectSaveFile({u"Сохранить как", name})
      .then([model = model_](const std::filesystem::path& path) {
        model->SaveLog(path);
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
