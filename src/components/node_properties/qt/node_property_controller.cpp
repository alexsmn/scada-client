#include "client/components/node_properties/qt/node_property_controller.h"

#include "client/components/node_properties/node_property_model.h"
#include "client/controller_factory.h"
#include "client/qt/tree_model_adapter.h"
#include "client/window_definition.h"

#include <QTreeView>

const float ROW_HEIGHT = 22.0f;

// NodePropertyController

REGISTER_CONTROLLER(NodePropertyController, ID_PROPERTY_VIEW);

NodePropertyController::NodePropertyController(const ControllerContext& context)
    : Controller(context) {
}

NodePropertyController::~NodePropertyController() {
}

UiView* NodePropertyController::Init(const WindowDefinition& definition) {
  std::vector<scada::NodeId> node_ids;
  const WindowItem* window_item = definition.FindItem("Item");
  if (window_item) {
    std::string path = window_item->GetString("path");
    auto node_id = scada::NodeId::FromString(path);
    if (!node_id.is_null())
      node_ids.emplace_back(std::move(node_id));
  }

  model_ = std::make_unique<NodePropertyModel>(
      PropertyContext{node_service_, task_manager_, node_management_service_},
      std::move(node_ids));

  model_adapter_ = std::make_unique<TreeModelAdapter>(*model_);
  model_adapter_->set_editable_column_ids({1});

  view_ = new QTreeView;
  view_->setModel(model_adapter_.get());

  return view_;
}
