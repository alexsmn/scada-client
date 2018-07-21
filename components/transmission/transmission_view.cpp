#include "components/transmission/transmission_view.h"

#include "common/node_id_util.h"
#include "common/node_service.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "components/transmission/transmission_model.h"
#include "controller_factory.h"
#include "controls/grid.h"
#include "remote/session_proxy.h"
#include "services/task_manager.h"
#include "window_definition.h"

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

  grid_.reset(new Grid(*model_, *model_, column_model_));

#if defined(UI_VIEWS)
  grid_->SetRowHeadersVisible(true);
  grid_->set_allow_row_select(true);
  grid_->SetRowHeaderWidth(15);
#endif

  return grid_->CreateParentIfNecessary();
}

void TransmissionView::DeleteSelection() {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure))
    return;

#if defined(UI_VIEWS)
  ui::GridRange range = grid_->GetSelectionRange();
  if (range.empty())
    return;

  for (int i = range.row(); i <= range.last_row(); i++)
    task_manager_.PostDeleteTask(model_->row(i).transmission.node_id());
#endif
}

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
