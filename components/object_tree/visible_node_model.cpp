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
  for (auto& p : nodes_) {
    if (p.second->IsAlerting())
      node_change_handler_(p.first);
  }
}

void VisibleNodeModel::SetNode(void* tree_node,
                               std::unique_ptr<VisibleNode> node) {
  if (node) {
    node->change_handler_ = [this, tree_node] {
      node_change_handler_(tree_node);
    };

    nodes_[tree_node] = std::move(node);

    node_change_handler_(tree_node);

  } else {
    nodes_.erase(tree_node);
  }
}

std::wstring VisibleNodeModel::GetText(void* tree_node) {
  auto* node = GetNode(tree_node);
  if (!node)
    return std::wstring();
  return node->GetText();
}

SkColor VisibleNodeModel::GetTextColor(void* tree_node) {
  auto* node = GetNode(tree_node);
  if (!node || node->IsBad())
    return profile_.bad_value_color;

  return SK_ColorBLACK;
}

SkColor VisibleNodeModel::GetBackgroundColor(void* tree_node) {
  if (Blinker::GetState()) {
    auto* node = GetNode(tree_node);
    if (node && node->IsAlerting())
      return profile_.alarm_color;
  }

  return SK_ColorTRANSPARENT;
}

const VisibleNode* VisibleNodeModel::GetNode(void* tree_node) const {
  if (!tree_node)
    return nullptr;

  auto i = nodes_.find(static_cast<ConfigurationTreeNode*>(tree_node));
  if (i == nodes_.end())
    return nullptr;

  return i->second.get();
}

// VisibleNode

void VisibleNode::NotifyChanged() {
  if (change_handler_)
    change_handler_();
}

// ObjectVisibleNode

ObjectVisibleNode::ObjectVisibleNode(TimedDataService& timed_data_service,
                                     const NodeRef& node) {
  spec_.property_change_handler = [this](const PropertySet& properties) {
    if (properties.is_current_changed())
      NotifyChanged();
  };

  spec_.event_change_handler = [this] { NotifyChanged(); };

  spec_.Connect(timed_data_service, node);
}

std::wstring ObjectVisibleNode::GetText() const {
  return spec_.GetCurrentString(FORMAT_DEFAULT);
}

bool ObjectVisibleNode::IsBad() const {
  return spec_.current().qualifier.general_bad();
}

bool ObjectVisibleNode::IsAlerting() const {
  return spec_.alerting();
}
