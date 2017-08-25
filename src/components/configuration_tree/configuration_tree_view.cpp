#include "components/configuration_tree/configuration_tree_view.h"

#include "base/strings/sys_string_conversions.h"
#include "client_utils.h"
#include "common_resources.h"
#include "controller_factory.h"
#include "controls/tree.h"
#include "components/configuration_tree/configuration_tree_model.h"
#include "views/item_drag_data.h"
#include "common/scada_node_ids.h"
#include "core/node_management_service.h"
#include "core/session_service.h"
#include "ui/base/models/sorted_tree_model.h"
#include "common/node_ref_util.h"

#if defined(UI_VIEWS)
#include "ui/views/widget/widget.h"
#endif

class TypesView : public ConfigurationTreeView {
 public:
  explicit TypesView(const ControllerContext& context)
      : ConfigurationTreeView(context, std::make_unique<ConfigurationTreeModel>(
            context.node_service_, OpcUaId_TypesFolder,
            std::vector<scada::NodeId>{OpcUaId_HierarchicalReferences})) {
  }
};

// REGISTER_CONTROLLER(TypesView, ID_TYPES_VIEW);

class NodesView : public ConfigurationTreeView {
 public:
  explicit NodesView(const ControllerContext& context)
      : ConfigurationTreeView(context, std::make_unique<ConfigurationTreeModel>(
            context.node_service_, OpcUaId_RootFolder,
            std::vector<scada::NodeId>{OpcUaId_HierarchicalReferences})) {
  }
};

REGISTER_CONTROLLER(NodesView, ID_NODES_VIEW);

ConfigurationTreeView::ConfigurationTreeView(const ControllerContext& context, std::unique_ptr<ConfigurationTreeModel> model)
    : Controller(context),
      model_(std::move(model)) {
  // sorted_model_ = std::make_unique<ui::SortedTreeModel>(*model_);

  tree_view_.reset(new Tree(*model_));
  tree_view_->LoadIcons(IDB_ITEMS, 16, UiColorRGB(255, 0, 255));
  tree_view_->SetRootVisible(true);

#if defined(UI_VIEWS)
  tree_view_->SetShowChecks(model_->AreChecksVisible());
#endif

  tree_view_->SetSelectionChangedHandler([this] {
    auto selection_size = tree_view_->GetSelectionSize();
    if (selection_size == 0)
      selection().SelectNode(model_->root_node());
    else if (selection_size == 1) {
      auto* node = static_cast<ConfigurationTreeNode*>(tree_view_->GetSelectedNode());
      selection().SelectNode(node->data_node());
    } else
      selection().SelectMultiple();
  });

  tree_view_->SetDoubleClickHandler([this] {
    if (const auto& node = selection().node())
      controller_delegate_.ExecuteDefaultItemCommand(node);
  });

  tree_view_->SetCompareHandler([this](void* left, void* right) {
    auto& a = static_cast<ConfigurationTreeNode*>(left)->data_node();
    auto& b = static_cast<ConfigurationTreeNode*>(right)->data_node();
    if (!!a != !!b)
      return !!a < !!b ? -1 : 1;
    if (!a.fetched() || !b.fetched()) {
      return base::SysNativeMBToWide(a.browse_name()).compare(
             base::SysNativeMBToWide(b.browse_name()));
    }
    const auto& ta = a.type_definition().id();
    const auto& tb = b.type_definition().id();
    bool fa = a.node_class() != scada::NodeClass::Variable;
    bool fb = b.node_class() != scada::NodeClass::Variable;
    if (fa != fb)
      return fa < fb ? 1 : -1;
    if (ta != tb)
      return ta < tb ? -1 : 1;
    return base::SysNativeMBToWide(a.browse_name()).compare(
           base::SysNativeMBToWide(b.browse_name()));
  });

#if defined(UI_VIEWS)
  tree_view_->SetDragHandler([this](void* node) { StartDrag(node); });
#endif

  /*tree_view_->SetContextMenuHandler([this](const UiPoint& point) {
    controller_delegate_.ShowPopupMenu(IDR_ITEM_POPUP, point, true);
  });*/
}

ConfigurationTreeView::~ConfigurationTreeView() {
}

UiView* ConfigurationTreeView::Init(const WindowDefinition& definition) {
  return tree_view_.get();
}

CommandHandler* ConfigurationTreeView::GetCommandHandler(unsigned command_id) {
  switch (command_id) {
    case ID_DELETE: {
      if (!session_service_.IsAdministrator())
        return NULL;
    
      if (selection().node())
        return this;
    }
  }

  return __super::GetCommandHandler(command_id);
}

void ConfigurationTreeView::ExecuteCommand(unsigned command) {
  switch (command) {
    case ID_DELETE:
      DeleteSelection();
      break;
      
    default:
      __super::ExecuteCommand(command);
      break;
  }
}

void ConfigurationTreeView::OnViewNodeCreated(const NodeRef& node) {
  ConfigurationTreeNode* tree_node = model_->FindNode(node.id());
  if (tree_node)
    tree_view().SelectNode(tree_node);
}

void ConfigurationTreeView::DeleteSelection() {
  if (!session_service_.IsAdministrator())
    return;

  if (auto node = selection().node()) {
    base::string16 message = base::StringPrintf(
        L"Вы действительно хотите удалить %ls?",
        node.display_name().c_str());
    int choice = ShowMessageBox(dialog_service_,
        message.c_str(), L"Удаление", MB_ICONEXCLAMATION | MB_OKCANCEL);
    if (choice == IDOK)
      DeleteTreeRecordsRecursive(node, task_manager_);
  }
}

#if defined(UI_VIEWS)
void ConfigurationTreeView::StartDrag(void* node) {
  ConfigurationTreeNode* cfg_node = static_cast<ConfigurationTreeNode*>(node);
  if (!cfg_node->data_node())
    return;

  const auto& data_node = cfg_node->data_node();
  if (!data_node)
    return;

  ui::OSExchangeData data;
  ItemDragData item_data(data_node.id());
  item_data.Save(data);

  if (views::Widget* widget = tree_view_->GetWidget())
    widget->RunShellDrag(data, DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK);
}

void* ConfigurationTreeView::TestDrop(const ui::DropTargetEvent& event) const {
  /*auto* dragging_node = configuration_.GetNode(dragging_item_id_);
  if (!dragging_node)
    return NULL;*/

  ConfigurationTreeNode* node =
      reinterpret_cast<ConfigurationTreeNode*>(tree_view_->GetNodeAt(event.location()));
  if (!node)
    return NULL;

  /*auto* dragging_type = scada::GetTypeDefinition(*dragging_node);
  if (!dragging_type)
    return nullptr;

  for ( ; node; node = node->parent()) {
    if (!node->data_node())
      continue;
    auto* type = scada::GetTypeDefinition(node->data_node().node());
    if (type && scada::HasComponent(*type, *dragging_type))
      break;
  }
  if (!node)
    return nullptr;

  if (node->data_node().node_ptr() == dragging_node ||
      node->data_node().node_ptr() == scada::GetParent(*dragging_node))
    return nullptr;*/

  return node;
}

bool ConfigurationTreeView::CanDrop(const ui::OSExchangeData& data) {
  return session_service_.IsAdministrator() && data.HasCustomFormat(ItemDragData::GetCustomFormat());
}

void ConfigurationTreeView::OnDragEntered(const ui::DropTargetEvent& event) {
  assert(dragging_item_id_.is_null());

  ItemDragData item_data;
  if (item_data.Load(event.data()))
    dragging_item_id_ = item_data.item_id();
}

int ConfigurationTreeView::OnDragUpdated(const ui::DropTargetEvent& event) {
  void* node = TestDrop(event);
  tree_view_->SetDropTargetNode(node);
  if (!node)
    return ui::DragDropTypes::DRAG_NONE;
    
  return ui::DragDropTypes::DRAG_MOVE;
}

void ConfigurationTreeView::OnDragDone() {
  dragging_item_id_ = scada::NodeId();
  tree_view_->SetDropTargetNode(NULL);
}

int ConfigurationTreeView::OnPerformDrop(const ui::DropTargetEvent& event) {
  assert(!dragging_item_id_.is_null());
  auto dragging_node_id = dragging_item_id_;
  dragging_item_id_ = scada::NodeId();

  ConfigurationTreeNode* target_node =
      reinterpret_cast<ConfigurationTreeNode*>(tree_view_->drop_target_node());
  tree_view_->SetDropTargetNode(NULL);
  if (!target_node || !target_node->data_node())
    return ui::DragDropTypes::DRAG_NONE;

  /*auto* dragging_node = configuration_.GetNode(dragging_node_id);
  if (!dragging_node)
    return ui::DragDropTypes::DRAG_NONE;

  auto* parent = scada::GetParent(*dragging_node);
  if (parent) {
    node_management_service_.DeleteReference(OpcUaId_Organizes,
        parent->GetId(), dragging_node->GetId(),
        [](const scada::Status& status) {});
  }

  node_management_service_.AddReference(OpcUaId_Organizes,
        target_node->data_node().id(), dragging_node->GetId(),
        [](const scada::Status& status) {});*/

  return ui::DragDropTypes::DRAG_MOVE;
}
#endif
