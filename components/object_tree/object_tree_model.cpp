#include "components/object_tree/object_tree_model.h"

#include "core/standard_node_ids.h"

ObjectTreeModel::ObjectTreeModel(ObjectTreeModelContext&& context)
    : ObjectTreeModelContext{std::move(context)},
      ConfigurationTreeModel{::ConfigurationTreeModelContext{
          ObjectTreeModelContext::node_service_,
          ObjectTreeModelContext::task_manager_,
          ObjectTreeModelContext::root_,
          {{scada::id::Organizes, true}},
          {},
      }},
      visible_nodes_{
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
    return visible_nodes_.GetText(tree_node);
  else
    return ConfigurationTreeModel::GetText(tree_node, column_id);
}

SkColor ObjectTreeModel::GetTextColor(void* tree_node, int column_id) {
  if (column_id == 1)
    return visible_nodes_.GetTextColor(tree_node);
  else
    return ConfigurationTreeModel::GetTextColor(tree_node, column_id);
}

SkColor ObjectTreeModel::GetBackgroundColor(void* tree_node, int column_id) {
  if (column_id == 1)
    return visible_nodes_.GetBackgroundColor(tree_node);
  else
    return ConfigurationTreeModel::GetBackgroundColor(tree_node, column_id);
}

void ObjectTreeModel::SetNodeVisible(ConfigurationTreeNode& tree_node,
                                     bool visible) {
  visible_nodes_.SetNodeVisible(tree_node, visible);
}
