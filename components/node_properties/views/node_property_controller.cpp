#include "components/node_properties/views/node_property_controller.h"

#include "common/node_id_util.h"
#include "common/node_service.h"
#include "common/scada_node_ids.h"
#include "components/node_properties/node_property_model.h"
#include "controller_factory.h"
#include "ui/views/controls/property_view/property_view.h"
#include "window_definition.h"

const float ROW_HEIGHT = 22.0f;

// NodePropertyController

REGISTER_CONTROLLER(NodePropertyController, ID_NEW_PROPERTY_VIEW);

NodePropertyController::NodePropertyController(const ControllerContext& context)
    : Controller{context} {}

NodePropertyController::~NodePropertyController() {}

views::View* NodePropertyController::Init(const WindowDefinition& definition) {
  NodePropertyModel::Nodes nodes;

  const WindowItem* window_item = definition.FindItem("Item");
  if (window_item) {
    std::string path = window_item->GetString("path");
    auto node_id = NodeIdFromScadaString(path);
    if (auto node = node_service_.GetNode(node_id))
      nodes.emplace_back(node);
  }

  model_ = std::make_unique<NodePropertyModel>(node_service_, task_manager_,
                                               std::move(nodes));
  view_.reset(new views::PropertyView(*model_));
  view_->SetColumnWidth(0, 150);
  view_->SetColumnWidth(1, 200);

  return view_->CreateParentIfNecessary();
}
