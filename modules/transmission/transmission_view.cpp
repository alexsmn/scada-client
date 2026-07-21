#include "modules/transmission/transmission_view.h"

#include "aui/grid.h"
#include "aui/models/header_model.h"
#include "aui/translation.h"
#include "model/node_id_util.h"
#include "modules/transmission/transmission_model.h"
#include "node_service/node_service.h"
#include "profile/window_definition.h"
#include "remote/session_proxy.h"
#include "resources/common_resources.h"
#include "services/task_manager.h"

TransmissionView::TransmissionView(const ControllerContext& context)
    : ControllerContext{context},
      model_{std::make_shared<TransmissionModel>(context.executor_,
                                                 context.node_service_,
                                                 context.task_manager_)},
      column_model_{std::make_shared<scada::aui::ColumnHeaderModel>()} {}

TransmissionView::~TransmissionView() {}

std::unique_ptr<UiView> TransmissionView::Init(
    const WindowDefinition& definition) {
  if (const WindowItem* item = definition.FindItem("Item")) {
    auto path = item->GetString("path");
    auto device_id = NodeIdFromScadaString(path);
    model_->Init(node_service_.GetNode(device_id));
  }

  const scada::aui::TableColumn columns[] = {
      {0, Translate("Object"), 250, scada::aui::TableColumn::LEFT},
      {1, Translate("Address"), 100, scada::aui::TableColumn::RIGHT},
  };
  column_model_->SetColumns(std::size(columns), columns);

  grid_ = new scada::aui::Grid{model_, model_, column_model_};

  grid_->SetRowHeaderVisible(true);
  grid_->SetRowHeaderWidth(15);
  grid_->SetExpandAllowed(true);

  // Selecting a rule publishes it on the shell selection, so the
  // Transmission-rule inspector fills and node-scoped commands enable.
  grid_->SetSelectionChangeHandler([this] { OnSelectionChanged(); });
  selection_.multiple_handler = [this] {
    NodeIdSet node_ids;
    for (int row_index : grid_->GetSelectedRows())
      node_ids.insert(model_->row(row_index).transmission.node_id());
    return node_ids;
  };

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

void TransmissionView::OnSelectionChanged() {
  const auto rows = grid_->GetSelectedRows();
  if (rows.empty()) {
    selection_.Clear();
  } else if (rows.size() == 1) {
    selection_.SelectNode(model_->row(rows.front()).transmission);
  } else {
    selection_.SelectMultiple();
  }
}

CommandHandler* TransmissionView::GetCommandHandler(unsigned command_id) {
  return command_registry_.GetCommandHandler(command_id);
}

ContentsModel* TransmissionView::GetContentsModel() {
  return model_.get();
}
