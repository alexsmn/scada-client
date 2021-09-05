#include "components/watch/watch_view.h"

#include "common_resources.h"
#include "components/watch/watch_model.h"
#include "controller_delegate.h"
#include "controls/table.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "remote/session_proxy.h"
#include "services/dialog_service.h"
#include "window_definition.h"

// WatchView

WatchView::WatchView(const ControllerContext& context)
    : ControllerContext{context},
      model_{std::make_shared<WatchModel>(
          WatchModelContext{context.node_service_})} {}

WatchView::~WatchView() {
  model_->observers().RemoveObserver(this);
}

void WatchView::Save(WindowDefinition& definition) {
  WindowItem& item = definition.AddItem("Item");
  item.SetString("path", NodeIdToScadaString(model_->device().node_id()));
}

std::wstring WatchView::MakeTitle() const {
  std::wstring title = ToString16(model_->device().display_name());
  if (model_->paused())
    title += L" [Пауза]";
  return title;
}

UiView* WatchView::Init(const WindowDefinition& definition) {
  const ui::TableColumn columns[] = {
      {0, L"Время", 100, ui::TableColumn::LEFT,
       ui::TableColumn::DataType::DateTime},
      {1, L"Устройство", 100, ui::TableColumn::LEFT},
      {2, L"Событие", 400, ui::TableColumn::LEFT},
  };

  if (const WindowItem* item = definition.FindItem("Item")) {
    auto path = item->GetString("path");
    auto device_id = NodeIdFromScadaString(path);
    model_->SetDevice(node_service_.GetNode(device_id));
  }

  table_ = new Table(model_, {columns, columns + _countof(columns)});

  table_->SetSelectionChangeHandler([this] {
    auto_scroll_ = table_->GetCurrentRow() == model_->GetRowCount() - 1;
  });

  table_->SetContextMenuHandler([this](const UiPoint& point) {
    controller_delegate_.ShowPopupMenu(IDR_LOG_POPUP, point, true);
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

  return table_->CreateParentIfNecessary();
}

void WatchView::SaveLog() {
  SYSTEMTIME time;
  GetLocalTime(&time);

  std::wstring name = base::StringPrintf(
      L"%04d%02d%02d_%02d%02d%02d.log", time.wYear, time.wMonth, time.wDay,
      time.wHour, time.wMinute, time.wSecond);

  auto path = dialog_service_.SelectSaveFile({L"Сохранить как", name});
  if (!path.empty())
    model_->SaveLog(path);
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
