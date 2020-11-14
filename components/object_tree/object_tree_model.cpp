#include "components/object_tree/object_tree_model.h"

#include "core/standard_node_ids.h"
#include "services/profile.h"

ObjectTreeModel::ObjectTreeModel(ObjectTreeModelContext&& context)
    : ObjectTreeModelContext{std::move(context)},
      ConfigurationTreeModel{::ConfigurationTreeModelContext{
          ObjectTreeModelContext::node_service_,
          ObjectTreeModelContext::task_manager_,
          ObjectTreeModelContext::root_,
          {{scada::id::Organizes, true}},
          {},
      }} {
  Blinker::Start();
}

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
  if (column_id == 1) {
    auto* timed_data = GetTimedData(tree_node);
    if (!timed_data)
      return std::wstring();
    return timed_data->GetCurrentString(FORMAT_DEFAULT);
  } else
    return ConfigurationTreeModel::GetText(tree_node, column_id);
}

SkColor ObjectTreeModel::GetTextColor(void* tree_node, int column_id) {
  if (column_id == 1) {
    auto* timed_data = GetTimedData(tree_node);
    if (!timed_data || timed_data->current().qualifier.general_bad())
      return profile_.bad_value_color;
  }
  return ConfigurationTreeModel::GetTextColor(tree_node, column_id);
}

SkColor ObjectTreeModel::GetBackgroundColor(void* tree_node, int column_id) {
  if (column_id == 1 && Blinker::GetState()) {
    auto* timed_data = GetTimedData(tree_node);
    if (timed_data && timed_data->alerting())
      return SK_ColorYELLOW;
  }
  return ConfigurationTreeModel::GetBackgroundColor(tree_node, column_id);
}

void ObjectTreeModel::OnBlink(bool state) {
  for (auto& p : visible_nodes_data_) {
    if (p.second.alerting())
      TreeNodeChanged(p.first);
  }
}

const TimedDataSpec* ObjectTreeModel::GetTimedData(void* tree_node) const {
  if (!tree_node)
    return nullptr;
  auto i =
      visible_nodes_data_.find(static_cast<ConfigurationTreeNode*>(tree_node));
  if (i == visible_nodes_data_.end())
    return nullptr;
  return &i->second;
}

void ObjectTreeModel::SetNodeVisible(ConfigurationTreeNode& tree_node,
                                     bool visible) {
  if (visible) {
    const auto& node = tree_node.node();
    if (node.fetched() && node.node_class() != scada::NodeClass::Variable)
      return;

    auto& spec = visible_nodes_data_[&tree_node];

    spec.property_change_handler = [this,
                                    &tree_node](const PropertySet& properties) {
      const auto& node = tree_node.node();
      if (node.fetched() && node.node_class() != scada::NodeClass::Variable) {
        auto& copied_this = *this;
        auto& copied_tree_node = tree_node;
        // WARNING: This line removes the current lambda.
        visible_nodes_data_.erase(&tree_node);
        copied_this.TreeNodeChanged(&tree_node);
        return;
      }

      if (properties.is_current_changed())
        TreeNodeChanged(&tree_node);
    };

    spec.event_change_handler = [this, &tree_node] {
      TreeNodeChanged(&tree_node);
    };
    spec.Connect(timed_data_service_, tree_node.node());
    TreeNodeChanged(&tree_node);

  } else {
    visible_nodes_data_.erase(&tree_node);
  }
}
