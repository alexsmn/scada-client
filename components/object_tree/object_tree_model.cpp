#include "components/object_tree/object_tree_model.h"

#include "components/configuration_tree/node_service_tree_impl.h"
#include "core/standard_node_ids.h"
#include "model/data_items_node_ids.h"
#include "node_service/node_util.h"

ObjectTreeModel::ObjectTreeModel(ObjectTreeModelContext&& context)
    : ObjectTreeModelContext{std::move(context)},
      ConfigurationTreeModel{::ConfigurationTreeModelContext{
          ObjectTreeModelContext::node_service_,
          std::make_unique<NodeServiceTreeImpl>(NodeServiceTreeImplContext{
              ObjectTreeModelContext::node_service_,
              ObjectTreeModelContext::root_,
              {{scada::id::Organizes, true}},
              {},
          }),
      }},
      visible_node_model_{
          timed_data_service_,
          profile_,
          [this](void* tree_node) { TreeNodeChanged(tree_node); },
      } {}

int ObjectTreeModel::GetColumnCount() const {
  return 2;
}

std::wstring ObjectTreeModel::GetColumnText(int column_id) const {
  return column_id == 0 ? L"Имя" : L"Значение";
}

int ObjectTreeModel::GetColumnPreferredSize(int column_id) const {
  return column_id == 0 ? 200 : 0;
}

std::wstring ObjectTreeModel::GetText(void* tree_node, int column_id) {
  if (column_id == 1)
    return visible_node_model_.GetText(tree_node);
  else
    return ConfigurationTreeModel::GetText(tree_node, column_id);
}

SkColor ObjectTreeModel::GetTextColor(void* tree_node, int column_id) {
  if (column_id == 1)
    return visible_node_model_.GetTextColor(tree_node);
  else
    return ConfigurationTreeModel::GetTextColor(tree_node, column_id);
}

SkColor ObjectTreeModel::GetBackgroundColor(void* tree_node, int column_id) {
  if (column_id == 1)
    return visible_node_model_.GetBackgroundColor(tree_node);
  else
    return ConfigurationTreeModel::GetBackgroundColor(tree_node, column_id);
}

void ObjectTreeModel::SetNodeVisible(void* tree_node, bool visible) {
  auto visible_node = visible ? CreateVisibleNode(tree_node) : nullptr;
  visible_node_model_.SetNode(tree_node, std::move(visible_node));
}

std::shared_ptr<VisibleNode> ObjectTreeModel::CreateVisibleNode(
    void* tree_node) {
  auto& configuration_tree_node =
      *static_cast<ConfigurationTreeNode*>(tree_node);
  auto node = configuration_tree_node.node();

  if (node.fetched())
    return CreateFetchedVisibleNode(node);

  auto proxy_visible_node = std::make_shared<ProxyVisibleNode>();
  // TODO: shared_ptr.
  node.Fetch(NodeFetchStatus::NodeOnly(),
             [this, proxy_visible_node](const NodeRef& node) {
               auto visible_node = CreateFetchedVisibleNode(node);
               proxy_visible_node->SetUnderlyingNode(std::move(visible_node));
             });

  return proxy_visible_node;
}

std::shared_ptr<VisibleNode> ObjectTreeModel::CreateFetchedVisibleNode(
    const NodeRef& node) {
  assert(node.fetched());

  if (IsInstanceOf(node, data_items::id::DataItemType)) {
    return std::make_shared<DataItemVisibleNode>(timed_data_service_,
                                                 blinker_manager_, node);

  } else if (IsInstanceOf(node, data_items::id::DataGroupType)) {
    return std::make_shared<DataGroupVisibleNode>(timed_data_service_, node);

  } else {
    return nullptr;
  }
}
