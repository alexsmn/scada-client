#include "components/node_properties/node_property_controller.h"

#include "common/node_id_util.h"
#include "common/node_service.h"
#include "common/scada_node_ids.h"
#include "components/node_properties/node_property_model.h"
#include "controller_factory.h"
#include "controls/property_tree_model.h"
#include "controls/tree.h"
#include "window_definition.h"

// NodePropertyController

REGISTER_CONTROLLER(NodePropertyController, ID_NEW_PROPERTY_VIEW);

NodePropertyController::NodePropertyController(const ControllerContext& context)
    : Controller{context} {}

NodePropertyController::~NodePropertyController() {}

UiView* NodePropertyController::Init(const WindowDefinition& definition) {
  NodeRef node;

  if (const WindowItem* window_item = definition.FindItem("Item")) {
    std::string path = window_item->GetString("path");
    auto node_id = NodeIdFromScadaString(path);
    node = node_service_.GetNode(node_id);
  }

  property_model_ = std::make_unique<NodePropertyModel>(
      PropertyContext{node_service_, task_manager_}, std::move(node));
  tree_model_ = std::make_unique<PropertyTreeModel>(*property_model_);
  tree_view_ = std::make_unique<Tree>(*tree_model_);
  /*tree_view_->SetColumnWidth(0, 150);
  tree_view_->SetColumnWidth(1, 200);*/

#if defined(UI_QT)
  tree_view_->setHeaderHidden(false);
  tree_view_->setColumnWidth(0, 200);
#endif

  return tree_view_.get();
}
