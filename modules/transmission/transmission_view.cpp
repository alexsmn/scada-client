#include "modules/transmission/transmission_view.h"

#include "aui/grid.h"
#include "aui/models/header_model.h"
#include "aui/translation.h"
#include "controller/controller_delegate.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "modules/transmission/transmission_devices.h"
#include "modules/transmission/transmission_model.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "profile/window_definition.h"
#include "remote/session_proxy.h"
#include "resources/common_resources.h"
#include "services/task_manager.h"

#if defined(UI_QT)
#include "aui/severity_colors.h"
#include "modules/transmission/qt/transmission_destination_rail.h"

#include <QHBoxLayout>
#include <QWidget>
#endif

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

#if defined(UI_QT)
  // Opt-in destination rail beside the grid: every transmission-capable
  // device with its rule count, switching which device's rules the grid
  // shows. Reshell chrome only.
  if (scada::aui::GetSeverityTheme() != scada::aui::SeverityTheme::kLegacy) {
    auto* container = new QWidget;
    auto* layout = new QHBoxLayout{container};
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(
        MakeTransmissionDestinationRail(TransmissionDestinationRailContext{
            .executor = executor_,
            .browse =
                [this] {
                  return BrowseTransmissionDevices(
                      node_service_.GetNode(scada::devices::id::Devices));
                },
            .current = model_->device().node_id(),
            .current_count = [this] { return model_->GetRowCount(); },
            .model = model_.get(),
            .on_device =
                [this](const scada::NodeId& device_id) {
                  SwitchDevice(device_id);
                }}));
    layout->addWidget(grid_->CreateParentIfNecessary(), 1);
    return std::unique_ptr<UiView>{container};
  }
#endif

  return std::unique_ptr<UiView>{grid_->CreateParentIfNecessary()};
}

void TransmissionView::SwitchDevice(const scada::NodeId& device_id) {
  if (device_id == model_->device().node_id())
    return;
  selection_.Clear();
  model_->Init(node_service_.GetNode(device_id));
  controller_delegate_.SetTitle(GetFullDisplayName(model_->device()));
}

void TransmissionView::DeleteSelection() {
  if (!session_service_.HasPermission(scada::Permission::kDeleteNode))
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
