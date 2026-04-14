#include "components/transmission/transmission_view.h"

#include "aui/grid.h"
#include "aui/translation.h"
#include "aui/models/header_model.h"
#include "common_resources.h"
#include "components/transmission/transmission_model.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "profile/window_definition.h"
#include "remote/session_proxy.h"
#include "services/task_manager.h"

TransmissionView::TransmissionView(const ControllerContext& context)
    : ControllerContext{context},
      model_{std::make_shared<TransmissionModel>(context.node_service_,
                                                 context.task_manager_)},
      column_model_{std::make_shared<aui::ColumnHeaderModel>()} {}

TransmissionView::~TransmissionView() {}

std::unique_ptr<UiView> TransmissionView::Init(
    const WindowDefinition& definition) {
  if (const WindowItem* item = definition.FindItem("Item")) {
    auto path = item->GetString("path");
    auto device_id = NodeIdFromScadaString(path);
    model_->Init(node_service_.GetNode(device_id));
  }

  const aui::TableColumn columns[] = {
      {0, Translate("Object"), 250, aui::TableColumn::LEFT},
      {1, Translate("Address"), 100, aui::TableColumn::RIGHT},
  };
  column_model_->SetColumns(std::size(columns), columns);

  grid_ = new aui::Grid{model_, model_, column_model_};

  grid_->SetRowHeaderVisible(true);
  grid_->SetRowHeaderWidth(15);
  grid_->SetExpandAllowed(true);

  command_registry_.AddCommand(
      Command{ID_DELETE}.set_execute_handler([this] { DeleteSelection(); }));

  return std::unique_ptr<UiView>{grid_->CreateParentIfNecessary()};
}

void TransmissionView::DeleteSelection() {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure))
    return;

  for (auto row_index : grid_->GetSelectedRows())
    task_manager_.PostDeleteTask(model_->row(row_index).transmission.node_id());
}

CommandHandler* TransmissionView::GetCommandHandler(unsigned command_id) {
  return command_registry_.GetCommandHandler(command_id);
}

ContentsModel* TransmissionView::GetContentsModel() {
  return model_.get();
}
