#include "client/components/watch/watch_view.h"

#include "client/common_resources.h"
#include "client/window_definition.h"
#include "client/components/watch/watch_model.h"
#include "client/controller_factory.h"
#include "client/controls/table.h"
#include "common/node_ref_service.h"

#if defined(UI_QT)
#elif defined(UI_VIEWS)
#include "skia/ext/skia_utils_win.h"
#include "ui/gfx/point.h"
#include "client/views/client_utils_views.h"
// TODO: Remove.
#include <atlstr.h>
#include <atlbase.h>
#include <wtl/atlapp.h>
#include <wtl/atluser.h>
#include <wtl/atldlgs.h>
#endif

// WatchView

REGISTER_CONTROLLER(WatchView, ID_WATCH_VIEW);

WatchView::WatchView(const ControllerContext& context)
    : Controller(context),
      model_(std::make_unique<WatchModel>(monitored_item_service_)),
      auto_scroll_(true) {
}

WatchView::~WatchView() {
  model_->observers().RemoveObserver(this);
}

void WatchView::Save(WindowDefinition& definition) {
  WindowItem& item = definition.AddItem("Item");
  item.SetString("path", model_->device().id().ToString());
}

base::string16 WatchView::MakeTitle() const {
  base::string16 title = base::SysNativeMBToWide(model_->device().browse_name());
  if (model_->paused())
    title += L" [Пауза]";
  return title;
}

UiView* WatchView::Init(const WindowDefinition& definition) {
  const ui::TableColumn columns[] = {
      ui::TableColumn(0, L"Время", 100, ui::TableColumn::LEFT),
      ui::TableColumn(1, L"Устройство", 100, ui::TableColumn::LEFT),
      ui::TableColumn(2, L"Событие", 400, ui::TableColumn::LEFT)
  };

  const WindowItem* item = definition.FindItem("Item");
  if (item) {
    std::string path = item->GetString("path");
    auto device_id = scada::NodeId::FromString(path);
    if (!device_id.is_null())
      model_->SetDevice(node_service_.GetPartialNode(device_id));
  }

#if defined(UI_QT)
  table_.reset(new Table(*model_, { columns, columns + _countof(columns) }));
  QObject::connect(table_->selectionModel(), &QItemSelectionModel::currentRowChanged,
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
  controller_delegate_.ShowPopupMenu(IDR_LOG_POPUP, point, true);
}

void WatchView::OnSelectionChanged(views::TableView& sender) {
  auto_scroll_ =
      table_->selection_model().size() == 1 &&
      table_->selection_model().selected_indices()[0] == model_->GetRowCount() - 1;
}
#endif

void WatchView::SaveLog() {
#if defined(UI_VIEWS)
  SYSTEMTIME time;
  GetLocalTime(&time);

  base::string16 name = base::StringPrintf(L"%04d%02d%02d_%02d%02d%02d",
      time.wYear, time.wMonth, time.wDay,
      time.wHour, time.wMinute, time.wSecond);

  WTL::CFileDialog dlg(FALSE, L"*.log", name.c_str(),
      OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST,
      L"Файлы Log\0*.log\0Все файлы\0*.*\0");
  if (dlg.DoModal(static_cast<DialogServiceViews&>(dialog_service_).GetParentView()) != IDOK)
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
