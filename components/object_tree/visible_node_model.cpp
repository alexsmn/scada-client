#include "components/object_tree/visible_node_model.h"

#include "components/configuration_tree/configuration_tree_model.h"
#include "services/profile.h"

VisibleNodeModel::VisibleNodeModel(TimedDataService& timed_data_service,
                                   Profile& profile,
                                   BlinkerManager& blinker_manager,
                                   NodeChangeHandler node_change_handler)
    : Blinker{blinker_manager},
      timed_data_service_{timed_data_service},
      profile_{profile},
      node_change_handler_{std::move(node_change_handler)} {
  Blinker::Start();
}

void VisibleNodeModel::OnBlink(bool state) {
  // TODO: Track list of alerting nodes.
  for (auto& p : visible_nodes_data_) {
    if (p.second->IsAlerting())
      node_change_handler_(p.first);
  }
}

void VisibleNodeModel::SetNodeVisible(ConfigurationTreeNode& tree_node,
                                      bool visible) {
  if (visible) {
    const auto& node = tree_node.node();
    if (node.fetched() && node.node_class() != scada::NodeClass::Variable)
      return;

    auto& node_data = visible_nodes_data_[&tree_node];
    assert(!node_data);

    node_data = CreateNodeData(tree_node);
    node_change_handler_(&tree_node);

  } else {
    visible_nodes_data_.erase(&tree_node);
  }
}

std::wstring VisibleNodeModel::GetText(void* tree_node) {
  auto* node_data = GetNodeData(tree_node);
  if (!node_data)
    return std::wstring();
  return node_data->GetText();
}

SkColor VisibleNodeModel::GetTextColor(void* tree_node) {
  auto* node_data = GetNodeData(tree_node);
  if (!node_data || node_data->IsBad())
    return profile_.bad_value_color;

  return SK_ColorBLACK;
}

SkColor VisibleNodeModel::GetBackgroundColor(void* tree_node) {
  if (Blinker::GetState()) {
    auto* node_data = GetNodeData(tree_node);
    if (node_data && node_data->IsAlerting())
      return profile_.alarm_color;
  }

  return SK_ColorTRANSPARENT;
}

const VisibleNodeModel::NodeData* VisibleNodeModel::GetNodeData(
    void* tree_node) const {
  if (!tree_node)
    return nullptr;

  auto i =
      visible_nodes_data_.find(static_cast<ConfigurationTreeNode*>(tree_node));
  if (i == visible_nodes_data_.end())
    return nullptr;

  return i->second.get();
}

std::unique_ptr<VisibleNodeModel::NodeData> VisibleNodeModel::CreateNodeData(
    ConfigurationTreeNode& tree_node) {
  return std::make_unique<NodeDataImpl>(
      timed_data_service_, tree_node.node(),
      [this, &tree_node] { node_change_handler_(&tree_node); });
}

// VisibleNodeModel::NodeDataImpl

VisibleNodeModel::NodeDataImpl::NodeDataImpl(
    TimedDataService& timed_data_service,
    const NodeRef& node,
    ChangeHandler change_handler) {
  spec_.property_change_handler =
      [change_handler](const PropertySet& properties) {
        if (properties.is_current_changed())
          change_handler();
      };

  spec_.event_change_handler = change_handler;

  spec_.Connect(timed_data_service, node);
}

std::wstring VisibleNodeModel::NodeDataImpl::GetText() const {
  return spec_.GetCurrentString(FORMAT_DEFAULT);
}

bool VisibleNodeModel::NodeDataImpl::IsBad() const {
  return spec_.current().qualifier.general_bad();
}

bool VisibleNodeModel::NodeDataImpl::IsAlerting() const {
  return spec_.alerting();
}
