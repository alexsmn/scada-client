#include "views/controllers/node_property_controller.h"

#include "client_session.h"
#include "controller_factory.h"
#include "window_definition.h"
#include "common/scada_node_ids.h"
#include "models/node_property_model.h"
#include "ui/views/controls/property_view/property_view.h"
#include "common/node_service.h"
#include "common/node_id_util.h"

const float ROW_HEIGHT = 22.0f;

// NodePropertyController

REGISTER_CONTROLLER(NodePropertyController, ID_NEW_PROPERTY_VIEW);

NodePropertyController::NodePropertyController(const ControllerContext& context)
    : Controller{context} {
}

NodePropertyController::~NodePropertyController() {
}

views::View* NodePropertyController::Init(const WindowDefinition& definition) {
  NodePropertyModel::Nodes nodes;

  const WindowItem* window_item = definition.FindItem("Item");
  if (window_item) {
    std::string path = window_item->GetString("path");
    auto node_id = NodeIdFromScadaString(path);
    if (auto node = g_node_service->GetNode(node_id))
      nodes.emplace_back(node);
  }

  model_.reset(new NodePropertyModel(*g_node_service, task_manager_, std::move(nodes)));
  view_.reset(new views::PropertyView(*model_));
  view_->SetColumnWidth(0, 150);
  view_->SetColumnWidth(1, 200);

  return view_->CreateParentIfNecessary();
}
