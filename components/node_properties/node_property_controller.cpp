#include "components/node_properties/node_property_controller.h"

#include "common/node_id_util.h"
#include "common/node_service.h"
#include "common/scada_node_ids.h"
#include "components/node_properties/node_property_model.h"
#include "controller_factory.h"
#include "controls/property_tree_model.h"
#include "controls/tree.h"
#include "window_definition.h"

const WindowInfo kWindowInfo = {ID_NEW_PROPERTY_VIEW,
                                "NewProps",
                                L"Параметры",
                                WIN_DISALLOW_NEW | WIN_REQUIRES_ADMIN,
                                200,
                                400};

REGISTER_CONTROLLER(NodePropertyController, kWindowInfo);

// NodePropertyController

NodePropertyController::NodePropertyController(const ControllerContext& context)
    : Controller{context} {}

NodePropertyController::~NodePropertyController() {}

UiView* NodePropertyController::Init(const WindowDefinition& definition) {
  NodeRef node;

  if (const WindowItem* window_item = definition.FindItem("Item")) {
    auto path = window_item->GetString("path");
    auto node_id = NodeIdFromScadaString(path);
    node = node_service_.GetNode(node_id);
  }

  selection().SelectNode(node);

  property_model_ = std::make_unique<NodePropertyModel>(
      PropertyContext{node_service_, task_manager_, dialog_service_},
      std::move(node));
  tree_model_ = std::make_unique<PropertyTreeModel>(*property_model_);
  tree_view_ = std::make_unique<Tree>(*tree_model_);

  /*tree_view_->SetColumnWidth(0, 150);
  tree_view_->SetColumnWidth(1, 200);*/

  tree_view_->SetHeaderVisible(true);
  tree_view_->SetRowHeight(21);

  tree_view_->SetCompareHandler([this](void* left, void* right) {
    auto& left_node = *static_cast<PropertyTreeModel::Node*>(left);
    auto& right_node = *static_cast<PropertyTreeModel::Node*>(right);
    auto* left_group = left_node.AsGroup();
    auto* right_group = right_node.AsGroup();
    if (left_group && right_group)
      return left_group->index - right_group->index;
    const auto& left_text = left_node.GetText(0);
    const auto& right_text = right_node.GetText(0);
    return left_text.compare(right_text);
  });

#if defined(UI_QT)
  tree_view_->setEditTriggers(QAbstractItemView::EditTrigger::AnyKeyPressed |
                              QAbstractItemView::EditTrigger::DoubleClicked |
                              QAbstractItemView::EditTrigger::EditKeyPressed |
                              QAbstractItemView::EditTrigger::SelectedClicked);
  tree_view_->setColumnWidth(0, 200);
  tree_view_->setAlternatingRowColors(true);
  tree_view_->expandAll();
#endif

  return tree_view_.get();
}

void NodePropertyController::Save(WindowDefinition& definition) {
  definition.AddItem("Item").SetString(
      "path", NodeIdToScadaString(property_model_->node().node_id()));
}
