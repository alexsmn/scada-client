#include "components/transmission/transmission_view.h"

#include "common/node_id_util.h"
#include "common/node_service.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "components/transmission/transmission_model.h"
#include "controller_factory.h"
#include "remote/session_proxy.h"
#include "services/task_manager.h"
#include "window_definition.h"

#if defined(UI_QT)
#include "controls/qt/grid.h"
#elif defined(UI_VIEWS)
#include "ui/views/controls/grid/grid_view.h"
#endif

REGISTER_CONTROLLER(TransmissionView, ID_TRANSMISSION_VIEW);

TransmissionView::TransmissionView(const ControllerContext& context)
    : Controller{context},
      model_(new TransmissionModel(context.node_service_,
                                   context.task_manager_)) {}

TransmissionView::~TransmissionView() {}

UiView* TransmissionView::Init(const WindowDefinition& definition) {
  if (const WindowItem* item = definition.FindItem("Item")) {
    std::string path = item->GetString("path");
    auto device_id = NodeIdFromScadaString(path);
    model_->SetDevice(node_service_.GetNode(device_id));
  }

  const ui::TableColumn columns[] = {
      ui::TableColumn(0, L"Объект", 250, ui::TableColumn::LEFT),
      ui::TableColumn(1, L"Адрес", 100, ui::TableColumn::RIGHT)};
  column_model_.SetColumns(_countof(columns), columns);

#if defined(UI_QT)
  grid_.reset(new Grid(*model_, *model_, column_model_));
  return grid_.get();

#elif defined(UI_VIEWS)
  grid_.reset(new views::GridView);
  grid_->SetModel(model_.get());
  grid_->SetRowModel(model_.get());
  grid_->SetColumnModel(&column_model_);
  grid_->set_controller(this);
  grid_->SetRowHeadersVisible(true);
  grid_->set_allow_row_select(true);
  grid_->SetRowHeaderWidth(15);
  return grid_->CreateParentIfNecessary();
#endif
}

#if defined(UI_VIEWS)
void TransmissionView::ShowContextMenu(gfx::Point point) {
  controller_delegate_.ShowPopupMenu(IDR_ITEM_POPUP, point, true);
}
#endif

void TransmissionView::DeleteSelection() {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure))
    return;

#if defined(UI_VIEWS)
  ui::GridRange range = grid_->GetSelectionRange();
  if (range.empty())
    return;

  for (int i = range.row(); i <= range.last_row(); i++)
    task_manager_.PostDeleteTask(model_->row(i).transmission.id());
#endif
}

void TransmissionView::WriteCell(int row, int col, LPCTSTR text) {}

#if defined(UI_VIEWS)
bool TransmissionView::CanEditCell(views::GridView& sender,
                                   int row,
                                   int column) {
  assert(row >= 0 && row < model_->GetRowCount());

  if (column == 0)
    return false;

  return true;
}

bool TransmissionView::OnGridEditCellText(views::GridView& sender,
                                          int row,
                                          int column,
                                          const base::string16& text) {
  assert(row >= 0 && row < model_->GetRowCount());

  /*	GridRange range = selection();
    for (int row = range.top; row <= range.bottom; row++)
      for (int col = range.left; col <= range.right; col++)
        WriteCell(row, col, text);*/

  int value;
  if (!Parse(text, value))
    return false;

  auto& row_item = model_->row(row);
  scada::NodeProperties properties;
  properties.emplace_back(kIecTransmitTargetInfoAddressPropTypeId,
                          static_cast<int>(value));
  task_manager_.PostUpdateTask(row_item.transmission.id(), {}, properties);

  return true;
}
#endif

CommandHandler* TransmissionView::GetCommandHandler(unsigned command_id) {
  switch (command_id) {
    case ID_DELETE:
      return this;
  }

  return __super::GetCommandHandler(command_id);
}

void TransmissionView::ExecuteCommand(unsigned command) {
  switch (command) {
    case ID_DELETE:
      DeleteSelection();
      break;
    default:
      __super::ExecuteCommand(command);
      break;
  }
}

ContentsModel* TransmissionView::GetContentsModel() {
  return model_.get();
}
