#include "components/watch/watch_view.h"

#include "common/node_id_util.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "common_resources.h"
#include "components/watch/watch_model.h"
#include "controller_factory.h"
#include "controls/table.h"
#include "remote/session_proxy.h"
#include "services/dialog_service.h"
#include "window_definition.h"

#if defined(UI_VIEWS)
#include "skia/ext/skia_utils_win.h"
#include "ui/gfx/point.h"
#include "views/client_utils_views.h"
#endif

const WindowInfo kWindowInfo = {
    ID_WATCH_VIEW, "Log", L"Наблюдение", WIN_DISALLOW_NEW, 0, 0, 0};

REGISTER_CONTROLLER(WatchView, kWindowInfo);

// WatchView

WatchView::WatchView(const ControllerContext& context)
    : Controller{context},
      model_{std::make_unique<WatchModel>(
          WatchModelContext{context.node_service_})} {}

WatchView::~WatchView() {
  model_->observers().RemoveObserver(this);
}

void WatchView::Save(WindowDefinition& definition) {
  WindowItem& item = definition.AddItem("Item");
  item.SetString("path", NodeIdToScadaString(model_->device().node_id()));
}

base::string16 WatchView::MakeTitle() const {
  base::string16 title = ToString16(model_->device().display_name());
  if (model_->paused())
    title += L" [Пауза]";
  return title;
}

UiView* WatchView::Init(const WindowDefinition& definition) {
  const ui::TableColumn columns[] = {
      ui::TableColumn(0, L"Время", 100, ui::TableColumn::LEFT),
      ui::TableColumn(1, L"Устройство", 100, ui::TableColumn::LEFT),
      ui::TableColumn(2, L"Событие", 400, ui::TableColumn::LEFT)};

  if (const WindowItem* item = definition.FindItem("Item")) {
    std::string path = item->GetString("path");
    auto device_id = NodeIdFromScadaString(path);
    model_->SetDevice(node_service_.GetNode(device_id));
  }

  table_.reset(new Table(*model_, {columns, columns + _countof(columns)}));

#if defined(UI_QT)
  QObject::connect(
      table_->selectionModel(), &QItemSelectionModel::currentRowChanged,
      [this](const QModelIndex& current, const QModelIndex& previous) {
        auto_scroll_ = current.row() == model_->GetRowCount() - 1;
      });

#elif defined(UI_VIEWS)
  table_->set_controller(this);
#endif

  table_->SetContextMenuHandler([this](const UiPoint& point) {
    controller_delegate_.ShowPopupMenu(IDR_LOG_POPUP, point, true);
  });

  // Must be after |table_| is bound.
  model_->observers().AddObserver(this);

  return table_->CreateParentIfNecessary();
}

#if defined(UI_VIEWS)
void WatchView::OnSelectionChanged(views::TableView& sender) {
  auto_scroll_ = table_->selection_model().size() == 1 &&
                 table_->selection_model().selected_indices()[0] ==
                     model_->GetRowCount() - 1;
}
#endif

void WatchView::SaveLog() {
  SYSTEMTIME time;
  GetLocalTime(&time);

  base::string16 name = base::StringPrintf(
      L"%04d%02d%02d_%02d%02d%02d.log", time.wYear, time.wMonth, time.wDay,
      time.wHour, time.wMinute, time.wSecond);

  auto path = dialog_service_.SelectSaveFile(L"Сохранить как", name);
  if (!path.empty())
    model_->SaveLog(path);
}

CommandHandler* WatchView::GetCommandHandler(unsigned command_id) {
  switch (command_id) {
    case ID_PAUSE:
    case ID_SAVE_AS:
    case ID_CLEAR_ALL:
      return this;
  }

  return __super::GetCommandHandler(command_id);
}

bool WatchView::IsCommandChecked(unsigned command_id) const {
  switch (command_id) {
    case ID_PAUSE:
      return model_->paused();
    default:
      return __super::IsCommandChecked(command_id);
  }
}

void WatchView::ExecuteCommand(unsigned command) {
  switch (command) {
    case ID_PAUSE:
      model_->set_paused(!model_->paused());
      controller_delegate_.SetTitle(MakeTitle());
      break;
    case ID_SAVE_AS:
      SaveLog();
      break;
    case ID_CLEAR_ALL: {
      model_->Clear();
      break;
    }
    default:
      __super::ExecuteCommand(command);
      break;
  }
}

void WatchView::OnItemsAdded(int first, int count) {
  if (!auto_scroll_)
    return;

  auto last_row = first + count - 1;
  table_->SelectRow(last_row);
}
