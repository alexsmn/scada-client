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

// TODO: Remove.
#include <atlbase.h>

#include <atlapp.h>
#include <atldlgs.h>
#include <atlstr.h>
#include <atluser.h>
#endif

// WatchView

REGISTER_CONTROLLER(WatchView, ID_WATCH_VIEW);

WatchView::WatchView(const ControllerContext& context)
    : Controller{context},
      model_{std::make_unique<WatchModel>(WatchModelContext{
          context.node_service_, context.monitored_item_service_})} {}

WatchView::~WatchView() {
  model_->observers().RemoveObserver(this);
}

void WatchView::Save(WindowDefinition& definition) {
  WindowItem& item = definition.AddItem("Item");
  item.SetString("path", NodeIdToScadaString(model_->device_id()));
}

base::string16 WatchView::MakeTitle() const {
  base::string16 title = GetDisplayName(node_service_, model_->device_id());
  if (model_->paused())
    title += L" [Пауза]";
  return title;
}

UiView* WatchView::Init(const WindowDefinition& definition) {
  const ui::TableColumn columns[] = {
      ui::TableColumn(0, L"Время", 100, ui::TableColumn::LEFT),
      ui::TableColumn(1, L"Устройство", 100, ui::TableColumn::LEFT),
      ui::TableColumn(2, L"Событие", 400, ui::TableColumn::LEFT)};

  const WindowItem* item = definition.FindItem("Item");
  if (item) {
    std::string path = item->GetString("path");
    auto device_id = NodeIdFromScadaString(path);
    if (!device_id.is_null())
      model_->SetDeviceID(device_id);
  }

#if defined(UI_QT)
  table_.reset(new Table(*model_, {columns, columns + _countof(columns)}));
  QObject::connect(
      table_->selectionModel(), &QItemSelectionModel::currentRowChanged,
      [this](const QModelIndex& current, const QModelIndex& previous) {
        auto_scroll_ = current.row() == model_->GetRowCount() - 1;
      });

  // Must be after |table_| is bound.
  model_->observers().AddObserver(this);

  return table_.get();

#elif defined(UI_VIEWS)
  table_.reset(new views::TableView(*model_));
  table_->set_controller(this);
  table_->set_show_grid(false);
  table_->SetColumns(_countof(columns), columns);

  // Must be after |table_| is bound.
  model_->observers().AddObserver(this);

  return &table_->CreateParentIfNecessary();
#endif
}

#if defined(UI_VIEWS)
void WatchView::ShowContextMenu(gfx::Point point) {
  WTL::CMenu menu;
  menu.LoadMenu(IDR_LOG_POPUP);
  WTL::CMenuHandle popup = menu.GetSubMenu(0);
  popup.CheckMenuItem(
      ID_PAUSE, MF_BYCOMMAND | (model_->paused() ? MF_CHECKED : MF_UNCHECKED));
  ::ShowPopupMenu(dialog_service_.GetDialogOwningWindow(), popup, point, true);
}

void WatchView::OnSelectionChanged(views::TableView& sender) {
  auto_scroll_ = table_->selection_model().size() == 1 &&
                 table_->selection_model().selected_indices()[0] ==
                     model_->GetRowCount() - 1;
}
#endif

void WatchView::SaveLog() {
#if defined(UI_VIEWS)
  SYSTEMTIME time;
  GetLocalTime(&time);

  base::string16 name =
      base::StringPrintf(L"%04d%02d%02d_%02d%02d%02d", time.wYear, time.wMonth,
                         time.wDay, time.wHour, time.wMinute, time.wSecond);

  WTL::CFileDialog dlg(
      FALSE, L"*.log", name.c_str(),
      OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST,
      L"Файлы Log\0*.log\0Все файлы\0*.*\0");
  if (dlg.DoModal() != IDOK)
    return;

  model_->SaveLog(base::FilePath(dlg.m_ofn.lpstrFile));
#endif
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

#if defined(UI_QT)
  table_->selectRow(last_row);
#elif defined(UI_VIEWS)
  table_->Select(last_row, true);
#endif
}
