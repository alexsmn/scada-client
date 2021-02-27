#include "components/configuration_tree/configuration_tree_view.h"

#include "base/strings/sys_string_conversions.h"
#include "client_utils.h"
#include "common_resources.h"
#include "components/configuration_tree/configuration_tree_drop_handler.h"
#include "components/configuration_tree/configuration_tree_model.h"
#include "controller_delegate.h"
#include "controls/tree.h"
#include "core/node_management_service.h"
#include "core/session_service.h"
#include "model/scada_node_ids.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "services/dialog_service.h"
#include "views/item_drag_data.h"

#if defined(UI_VIEWS)
#include "ui/views/widget/widget.h"
#endif

ConfigurationTreeView::ConfigurationTreeView(
    const ControllerContext& context,
    ConfigurationTreeModel& model,
    ConfigurationTreeDropHandler& drop_handler)
    : ControllerContext{context}, model_{&model}, drop_handler_{&drop_handler} {
  model_->Init();

  tree_view_.reset(new Tree(*model_));
  tree_view_->LoadIcons(IDB_ITEMS, 16, UiColorRGB(255, 0, 255));
  tree_view_->SetRootVisible(true);
  tree_view_->SetSorted(true);

  tree_view_->SetFocusHandler([this] { controller_delegate_.Focus(); });

  tree_view_->SetSelectionChangedHandler([this] {
    auto selection_size = tree_view_->GetSelectionSize();
    if (selection_size == 0)
      selection_.SelectNode(model_->root_node());
    else if (selection_size == 1) {
      auto* node =
          static_cast<ConfigurationTreeNode*>(tree_view_->GetSelectedNode());
      selection_.SelectNode(node ? node->node() : nullptr);
    } else
      selection_.SelectMultiple();
  });

  tree_view_->SetDoubleClickHandler([this] {
    const auto& node = selection_.node();
    if (node)
      controller_delegate_.ExecuteDefaultNodeCommand(node);
  });

  tree_view_->SetCompareHandler([](void* left, void* right) {
    const auto& a = static_cast<ConfigurationTreeNode*>(left)->node();
    const auto& b = static_cast<ConfigurationTreeNode*>(right)->node();
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
  });

#if defined(UI_VIEWS)
  tree_view_->SetDragHandler([this](void* node) { StartDrag(node); });
#endif

  tree_view_->SetContextMenuHandler([this](const UiPoint& point) {
    controller_delegate_.ShowPopupMenu(IDR_ITEM_POPUP, point, true);
  });
}

ConfigurationTreeView::~ConfigurationTreeView() {}

UiView* ConfigurationTreeView::Init(const WindowDefinition& definition) {
  if (auto* state = definition.FindItem("State"))
    tree_view_->RestoreState(state->attributes);

  return tree_view_.get();
}

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
  context.title = GetFullDisplayName(tree_node->node());
  context.node_ids =
      GetVariableNodeIds(tree_view().GetOrderedNodes(tree_node, false));
  return std::move(context);
}

#if defined(UI_VIEWS)
void ConfigurationTreeView::StartDrag(void* node) {
  ConfigurationTreeNode* cfg_node = static_cast<ConfigurationTreeNode*>(node);
  if (!cfg_node->node())
    return;

  scada::NodeId item_id = cfg_node->node().node_id();
  if (item_id.is_null())
    return;

  ui::OSExchangeData data;
  ItemDragData item_data(item_id);
  item_data.Save(data);

  if (auto* widget = tree_view_->GetWidget()) {
    widget->RunShellDrag(data,
                         DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK);
  }
}

bool ConfigurationTreeView::CanDrop(const ui::OSExchangeData& data) {
  return session_service_.HasPrivilege(scada::Privilege::Configure) &&
         data.HasCustomFormat(ItemDragData::GetCustomFormat());
}

void ConfigurationTreeView::OnDragEntered(const ui::DropTargetEvent& event) {
  assert(dragging_item_id_.is_null());

  ItemDragData item_data;
  if (item_data.Load(event.data()))
    dragging_item_id_ = item_data.item_id();
}

int ConfigurationTreeView::OnDragUpdated(const ui::DropTargetEvent& event) {
  ConfigurationTreeNode* target_node = reinterpret_cast<ConfigurationTreeNode*>(
      tree_view_->GetNodeAt(event.location()));
  int result = drop_handler_->GetDropAction(dragging_item_id_, target_node,
                                            drop_action_);
  if (result == ui::DragDropTypes::DRAG_NONE) {
    target_node = nullptr;
    drop_action_ = nullptr;
  }
  tree_view_->SetDropTargetNode(target_node);
  return result;
}

void ConfigurationTreeView::OnDragDone() {
  dragging_item_id_ = scada::NodeId();
  drop_action_ = nullptr;
  tree_view_->SetDropTargetNode(NULL);
}

int ConfigurationTreeView::OnPerformDrop(const ui::DropTargetEvent& event) {
  assert(!dragging_item_id_.is_null());

  auto drop_action = std::move(drop_action_);

  drop_action_ = nullptr;
  dragging_item_id_ = scada::NodeId();
  tree_view_->SetDropTargetNode(NULL);

  return drop_action();
}
#endif
