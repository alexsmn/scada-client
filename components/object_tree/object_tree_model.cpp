#include "components/object_tree/object_tree_model.h"

#include "core/standard_node_ids.h"
#include "services/profile.h"

ObjectTreeModel::ObjectTreeModel(ObjectTreeModelContext&& context)
    : ObjectTreeModelContext{std::move(context)},
      ConfigurationTreeModel{ObjectTreeModelContext::node_service_,
                             ObjectTreeModelContext::task_manager_,
                             ObjectTreeModelContext::root_,
                             {scada::id::Organizes},
                             {}} {
  Blinker::Start();
}

int ObjectTreeModel::GetColumnCount() const {
  return 2;
}

base::string16 ObjectTreeModel::GetColumnText(int column_id) const {
  return column_id == 0 ? L"Имя" : L"Значение";
}

int ObjectTreeModel::GetColumnPreferredSize(int column_id) const {
  return column_id == 0 ? 200 : 0;
}

base::string16 ObjectTreeModel::GetText(void* node, int column_id) {
  if (column_id == 1) {
    auto* timed_data = GetTimedData(node);
    if (!timed_data)
      return base::string16();
    return timed_data->GetCurrentString(FORMAT_DEFAULT);
  } else
    return ConfigurationTreeModel::GetText(node, column_id);
}

SkColor ObjectTreeModel::GetTextColor(void* node, int column_id) {
  if (column_id == 1) {
    auto* timed_data = GetTimedData(node);
    if (!timed_data || timed_data->current().qualifier.general_bad())
      return profile_.bad_value_color;
  }
  return ConfigurationTreeModel::GetTextColor(node, column_id);
}

SkColor ObjectTreeModel::GetBackgroundColor(void* node, int column_id) {
  if (column_id == 1 && Blinker::GetState()) {
    auto* timed_data = GetTimedData(node);
    if (timed_data && timed_data->alerting())
      return SK_ColorYELLOW;
  }
  return ConfigurationTreeModel::GetBackgroundColor(node, column_id);
}

void ObjectTreeModel::OnBlink(bool state) {
  for (auto& p : visible_nodes_data_) {
    if (p.second.alerting())
      TreeNodeChanged(p.first);
  }
}

const rt::TimedDataSpec* ObjectTreeModel::GetTimedData(void* node) const {
  if (!node)
    return nullptr;
  auto i = visible_nodes_data_.find(static_cast<ConfigurationTreeNode*>(node));
  if (i == visible_nodes_data_.end())
    return nullptr;
  return &i->second;
}

void ObjectTreeModel::SetNodeVisible(ConfigurationTreeNode& node,
                                     bool visible) {
  if (visible) {
    const auto& data_node = node.data_node();
    if (data_node.fetched() &&
        data_node.node_class() != scada::NodeClass::Variable)
      return;

    rt::TimedDataSpec& spec = visible_nodes_data_[&node];

    spec.property_change_handler = [this,
                                    &node](const rt::PropertySet& properties) {
      const auto& data_node = node.data_node();
      if (data_node.fetched() &&
          data_node.node_class() != scada::NodeClass::Variable) {
        visible_nodes_data_.erase(&node);
        TreeNodeChanged(&node);
        return;
      }

      if (properties.is_current_changed())
        TreeNodeChanged(&node);
    };

    spec.event_change_handler = [this, &node] { TreeNodeChanged(&node); };
    spec.Connect(timed_data_service_, node.data_node());
    TreeNodeChanged(&node);

  } else {
    visible_nodes_data_.erase(&node);
  }
}
