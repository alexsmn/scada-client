#include "components/watch/watch_view.h"

#include "base/strings/stringprintf.h"
#include "common_resources.h"
#include "components/watch/watch_event_source_impl.h"
#include "components/watch/watch_model.h"
#include "controller_delegate.h"
#include "aui/table.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "services/dialog_service.h"
#include "window_definition.h"

namespace {

struct WatchModelHolder {
  explicit WatchModelHolder(NodeService& node_service)
      : node_service{node_service} {}

  NodeService& node_service;
  WatchEventSourceImpl event_source{WatchEventSourceImplContext{node_service}};
  WatchModel model{WatchModelContext{node_service, event_source}};
};

std::shared_ptr<WatchModel> CreateWatchModel(NodeService& node_service) {
  auto holder = std::make_shared<WatchModelHolder>(node_service);
  return std::shared_ptr<WatchModel>(holder, &holder->model);
}

}  // namespace

// WatchView

WatchView::WatchView(const ControllerContext& context)
    : ControllerContext{context},
      model_{CreateWatchModel(context.node_service_)} {}

WatchView::~WatchView() {
  model_->observers().RemoveObserver(this);
}

void WatchView::Save(WindowDefinition& definition) {
  WindowItem& item = definition.AddItem("Item");
  item.SetString("path", NodeIdToScadaString(model_->device().node_id()));
}

std::u16string WatchView::MakeTitle() const {
  std::u16string title = ToString16(model_->device().display_name());
  if (model_->paused())
    title += u" [Пауза]";
  return title;
}

UiView* WatchView::Init(const WindowDefinition& definition) {
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

  table_ = new aui::Table(model_, {columns, columns + _countof(columns)});

  table_->SetSelectionChangeHandler([this] {
    auto_scroll_ = table_->GetCurrentRow() == model_->GetRowCount() - 1;
  });

  table_->SetContextMenuHandler([this](const aui::Point& point) {
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
