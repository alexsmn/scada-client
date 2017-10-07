#include "components/object_tree/object_tree_model.h"

#include "services/profile.h"
#include "core/standard_node_ids.h"

ObjectTreeModel::ObjectTreeModel(NodeRefService& node_service, scada::NodeId root_id,
                                 TimedDataService& timed_data_service, Profile& profile)
    : ConfigurationTreeModel{node_service, std::move(root_id), {scada::id::Organizes, scada::id::HasComponent}},
      timed_data_service_{timed_data_service},
      profile_(profile) {
  Blinker::Start();
}

int ObjectTreeModel::GetColumnCount() const {
  return 2;
}

base::string16 ObjectTreeModel::GetColumnText(int column_id) const {
  return column_id == 0 ? L"Объект" : L"Значение";
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
      return profile_.bad_value_color();
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

void ObjectTreeModel::OnPropertyChanged(rt::TimedDataSpec& spec, const rt::PropertySet& properties) {
  if (properties.is_current_changed()) {
    ConfigurationTreeNode& node =
        *reinterpret_cast<ConfigurationTreeNode*>(spec.param);
    TreeNodeChanged(&node);
  }
}

void ObjectTreeModel::OnEventsChanged(rt::TimedDataSpec& spec, const events::EventSet& events) {
  ConfigurationTreeNode& node =
      *reinterpret_cast<ConfigurationTreeNode*>(spec.param);
  TreeNodeChanged(&node);
}

void ObjectTreeModel::OnBlink(bool state) {
  for (auto& p : visible_nodes_) {
    auto& visible_node = p.second;
    if (visible_node.spec.alerting())
      TreeNodeChanged(p.first);
  }
}

const rt::TimedDataSpec* ObjectTreeModel::GetTimedData(void* node) const {
  if (!node)
    return nullptr;
  auto i = visible_nodes_.find(static_cast<ConfigurationTreeNode*>(node));
  if (i == visible_nodes_.end())
    return nullptr;
  auto& visible_node = i->second;
  return &visible_node.spec;
}

void ObjectTreeModel::SetNodeVisible(ConfigurationTreeNode& node, bool visible) {
  if (visible)
    ConnectVisibleNode(visible_nodes_[&node], node);
  else
    visible_nodes_.erase(&node);
}

void ObjectTreeModel::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  ConfigurationTreeModel::OnNodeSemanticChanged(node_id);

  auto* tree_node = FindNode(node_id);
  if (!tree_node)
    return;

  auto i = visible_nodes_.find(tree_node);
  if (i != visible_nodes_.end())
    ConnectVisibleNode(i->second, *tree_node);
}

void ObjectTreeModel::ConnectVisibleNode(VisibleNode& visible_node, ConfigurationTreeNode& tree_node) {
  if (visible_node.fetched)
    return;

  if (!tree_node.data_node().fetched())
    return;

  visible_node.fetched = true;

  if (tree_node.data_node().node_class() != scada::NodeClass::Variable)
    return;

  visible_node.spec.param = &tree_node;
  visible_node.spec.set_delegate(this);
  visible_node.spec.Connect(timed_data_service_, tree_node.data_node().id());
}