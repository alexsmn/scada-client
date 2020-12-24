#include "components/object_tree/visible_nodes.h"

#include "components/configuration_tree/configuration_tree_model.h"
#include "services/profile.h"

VisibleNodes::VisibleNodes(TimedDataService& timed_data_service,
                           Profile& profile,
                           NodeChangeHandler node_change_handler)
    : timed_data_service_{timed_data_service},
      profile_{profile},
      node_change_handler_{std::move(node_change_handler)} {
  Blinker::Start();
}

void VisibleNodes::OnBlink(bool state) {
  for (auto& p : visible_nodes_data_) {
    if (p.second.alerting())
      node_change_handler_(p.first);
  }
}

void VisibleNodes::SetNodeVisible(ConfigurationTreeNode& tree_node,
                                  bool visible) {
  if (visible) {
    const auto& node = tree_node.node();
    if (node.fetched() && node.node_class() != scada::NodeClass::Variable)
      return;

    auto& spec = visible_nodes_data_[&tree_node];
    spec = MakeTimedDataSpec(tree_node);
    node_change_handler_(&tree_node);

  } else {
    visible_nodes_data_.erase(&tree_node);
  }
}

std::wstring VisibleNodes::GetText(void* tree_node) {
  auto* timed_data = GetTimedData(tree_node);
  if (!timed_data)
    return std::wstring();
  return timed_data->GetCurrentString(FORMAT_DEFAULT);
}

SkColor VisibleNodes::GetTextColor(void* tree_node) {
  auto* timed_data = GetTimedData(tree_node);
  if (!timed_data || timed_data->current().qualifier.general_bad())
    return profile_.bad_value_color;

  return SK_ColorBLACK;
}

SkColor VisibleNodes::GetBackgroundColor(void* tree_node) {
  if (Blinker::GetState()) {
    auto* timed_data = GetTimedData(tree_node);
    if (timed_data && timed_data->alerting())
      return profile_.alarm_color;
  }

  return SK_ColorTRANSPARENT;
}

const TimedDataSpec* VisibleNodes::GetTimedData(void* tree_node) const {
  if (!tree_node)
    return nullptr;

  auto i =
      visible_nodes_data_.find(static_cast<ConfigurationTreeNode*>(tree_node));
  if (i == visible_nodes_data_.end())
    return nullptr;

  return &i->second;
}

TimedDataSpec VisibleNodes::MakeTimedDataSpec(
    ConfigurationTreeNode& tree_node) {
  TimedDataSpec spec;

  spec.property_change_handler = [this,
                                  &tree_node](const PropertySet& properties) {
    const auto& node = tree_node.node();
    if (node.fetched() && node.node_class() != scada::NodeClass::Variable) {
      auto& copied_this = *this;
      auto& copied_tree_node = tree_node;
      // WARNING: This line removes the current lambda.
      visible_nodes_data_.erase(&tree_node);
      copied_this.node_change_handler_(&copied_tree_node);
      return;
    }

    if (properties.is_current_changed())
      node_change_handler_(&tree_node);
  };

  spec.event_change_handler = [this, &tree_node] {
    node_change_handler_(&tree_node);
  };

  spec.Connect(timed_data_service_, tree_node.node());

  return spec;
}
