#include "components/configuration_tree/configuration_tree_view.h"

#include "common_resources.h"
#include "components/configuration_tree/configuration_tree_drop_handler.h"
#include "components/configuration_tree/configuration_tree_model.h"
#include "controller_delegate.h"
#include "controls/tree.h"
#include "item_drag_data.h"
#include "node_service/node_util.h"
#include "window_definition.h"

#if defined(UI_VIEWS)
#include "components/configuration_tree/views/configuration_tree_drag_drop_controller_views.h"
#endif

namespace {

int CompareNodes(const NodeRef& a, const NodeRef& b) {
  if (!!a != !!b)
    return !!a < !!b ? 1 : -1;
  if (a.fetched() != b.fetched())
    return a.fetched() < b.fetched() ? 1 : -1;
  const auto& ta = a.type_definition().node_id();
  const auto& tb = b.type_definition().node_id();
  bool fa = a.node_class() != scada::NodeClass::Variable;
  bool fb = b.node_class() != scada::NodeClass::Variable;
  if (fa != fb)
    return fa < fb ? 1 : -1;
  if (ta != tb)
    return ta < tb ? -1 : 1;
  return ToString16(a.display_name()).compare(ToString16(b.display_name()));
}

}  // namespace

ConfigurationTreeView::ConfigurationTreeView(
    const ControllerContext& context,
    std::shared_ptr<ConfigurationTreeModel> model,
    std::unique_ptr<ConfigurationTreeDropHandler> drop_handler)
    : ControllerContext{context},
      model_{std::move(model)},
      drop_handler_{std::move(drop_handler)} {
  tree_view_ = new Tree{model_};
  tree_view_->LoadIcons(IDB_ITEMS, 16, UiColorRGB(255, 0, 255));
  tree_view_->SetRootVisible(true);
  tree_view_->SetSorted(true);

  tree_view_->SetFocusHandler([this] { controller_delegate_.Focus(); });

  tree_view_->SetSelectionChangedHandler([this] { UpdateSelection(); });

  tree_view_->SetDoubleClickHandler([this] {
    const auto& node = selection_.node();
    if (node)
      controller_delegate_.ExecuteDefaultNodeCommand(node);
  });

  tree_view_->SetCompareHandler([](void* left, void* right) {
    return CompareNodes(
        static_cast<const ConfigurationTreeNode*>(left)->node(),
        static_cast<const ConfigurationTreeNode*>(right)->node());
  });

  tree_view_->SetDragHandler(
      {std::string{ItemDragData::kMimeType}},
      [this](const std::vector<void*>& nodes) { return GetDragData(nodes); });

  tree_view_->SetDropHandler([this](int drop_action, const DragData& drag_data,
                                    void* target_node) {
    DropAction action;
    auto* target_tree_node = static_cast<ConfigurationTreeNode*>(target_node);
    drop_handler_->GetDropAction(drag_data, target_tree_node, action);
    return action;
  });

#if defined(UI_VIEWS)
  drag_drop_controller_ =
      std::make_unique<ConfigurationTreeDragDropControllerViews>(
          *tree_view_, *drop_handler_, session_service_);

  tree_view_->SetDragHandler(
      [drop_controller = drag_drop_controller_.get()](void* node) {
        drop_controller->StartDrag(node);
      });
#endif

  tree_view_->SetContextMenuHandler([this](const aui::Point& point) {
    controller_delegate_.ShowPopupMenu(IDR_ITEM_POPUP, point, true);
  });
}

ConfigurationTreeView::~ConfigurationTreeView() = default;

UiView* ConfigurationTreeView::Init(const WindowDefinition& definition) {
  if (auto* state = definition.FindItem("State"))
    tree_view_->RestoreState(state->attributes);

  return tree_view_;
}

#if defined(UI_VIEWS)
views::DropController* ConfigurationTreeView::GetDropController() {
  return drag_drop_controller_.get();
}
#endif

void ConfigurationTreeView::Save(WindowDefinition& definition) {
  definition.AddItem("State").attributes = tree_view_->SaveState();
}

void ConfigurationTreeView::OnViewNodeCreated(const NodeRef& node) {
  // Select a first tree node.
  auto* tree_node = model_->FindFirstTreeNode(node.node_id());
  if (tree_node)
    tree_view().SelectNode(tree_node);
}

// Must keep |nodes| order.
std::vector<scada::NodeId> ConfigurationTreeView::GetVariableNodeIds(
    const std::vector<void*>& nodes) const {
  std::vector<scada::NodeId> node_ids;
  for (auto* node : nodes) {
    auto& n = *static_cast<ConfigurationTreeNode*>(node);
    if (n.node().node_class() == scada::NodeClass::Variable)
      node_ids.emplace_back(n.node().node_id());
  }
  return node_ids;
}

std::optional<OpenContext> ConfigurationTreeView::GetOpenContext() const {
  auto* tree_node =
      static_cast<ConfigurationTreeNode*>(tree_view().GetSelectedNode());
  if (!tree_node)
    tree_node = static_cast<ConfigurationTreeNode*>(model().GetRoot());
  if (!tree_node)
    return std::nullopt;

  OpenContext context;
  context.node = tree_node->node();
  return context;
}

void ConfigurationTreeView::UpdateSelection() {
  auto selection_size = tree_view_->GetSelectionSize();
  if (selection_size == 0)
    selection_.SelectNode(model_->root_node());
  else if (selection_size == 1) {
    auto* node =
        static_cast<ConfigurationTreeNode*>(tree_view_->GetSelectedNode());
    selection_.SelectNode(node ? node->node() : nullptr);
  } else
    selection_.SelectMultiple();
}

DragData ConfigurationTreeView::GetDragData(
    const std::vector<void*>& nodes) const {
  if (nodes.empty())
    return {};

  auto* tree_node = static_cast<ConfigurationTreeNode*>(nodes.front());
  auto node_id = tree_node->node().node_id();
  if (node_id.is_null())
    return {};

  DragData drag_data;
  ItemDragData{std::move(node_id)}.Save(drag_data);
  return drag_data;
}
