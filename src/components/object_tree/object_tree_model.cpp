#include "components/object_tree/object_tree_model.h"

#include "services/profile.h"
#include "core/standard_node_ids.h"

ObjectTreeModel::ObjectTreeModel(NodeRefService& node_service, scada::NodeId root_id,
                                 TimedDataService& timed_data_service, Profile& profile)
    : ConfigurationTreeModel{node_service, std::move(root_id), {OpcUaId_Organizes, OpcUaId_HasComponent}},
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
