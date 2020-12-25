#include "components/object_tree/visible_node_model.h"

#include "components/configuration_tree/configuration_tree_model.h"
#include "services/profile.h"

VisibleNodeModel::VisibleNodeModel(TimedDataService& timed_data_service,
                                   Profile& profile,
                                   NodeChangeHandler node_change_handler)
    : timed_data_service_{timed_data_service},
      profile_{profile},
      node_change_handler_{std::move(node_change_handler)} {}

VisibleNodeModel::~VisibleNodeModel() {
  for (auto& p : nodes_)
    p.second->SetChangeHandler(nullptr);
}

void VisibleNodeModel::SetNode(void* tree_node,
                               std::shared_ptr<VisibleNode> node) {
  auto i = nodes_.find(tree_node);
  if (i != nodes_.end()) {
    if (node == i->second)
      return;

    i->second->SetChangeHandler(nullptr);
    nodes_.erase(i);
  }

  if (!node)
    return;

  node->change_handler_ = [this, tree_node] {
    node_change_handler_(tree_node);
  };

  nodes_[tree_node] = std::move(node);

  node_change_handler_(tree_node);
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
  auto* node = GetNode(tree_node);
  if (node && node->IsAlerting())
    return profile_.alarm_color;

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

VisibleNode::~VisibleNode() {
  assert(!change_handler_);
}

void VisibleNode::SetChangeHandler(ChangeHandler change_handler) {
  change_handler_ = std::move(change_handler);
}

void VisibleNode::NotifyChanged() {
  if (change_handler_)
    change_handler_();
}

// ProxyVisibleNode

ProxyVisibleNode::~ProxyVisibleNode() {
  if (underlying_node_)
    underlying_node_->SetChangeHandler(nullptr);
}

void ProxyVisibleNode::SetUnderlyingNode(std::shared_ptr<VisibleNode> node) {
  if (underlying_node_ == node)
    return;

  if (underlying_node_)
    underlying_node_->SetChangeHandler(nullptr);

  underlying_node_ = std::move(node);

  if (underlying_node_) {
    underlying_node_->SetChangeHandler([weak_ptr = weak_from_this()] {
      if (auto ptr = weak_ptr.lock())
        ptr->NotifyChanged();
    });
  }
}

std::wstring ProxyVisibleNode::GetText() const {
  return underlying_node_ ? underlying_node_->GetText() : nullptr;
}

bool ProxyVisibleNode::IsBad() const {
  return underlying_node_ && underlying_node_->IsBad();
}

bool ProxyVisibleNode::IsAlerting() const {
  return underlying_node_ && underlying_node_->IsAlerting();
}

// ObjectVisibleNode

ObjectVisibleNode::ObjectVisibleNode(TimedDataService& timed_data_service,
                                     BlinkerManager& blinker_manager,
                                     NodeRef node)
    : Blinker{blinker_manager} {
  spec_.property_change_handler = [this](const PropertySet& properties) {
    if (properties.is_current_changed())
      NotifyChanged();
  };

  spec_.event_change_handler = [this] { SetAlerting(spec_.alerting()); };

  spec_.Connect(timed_data_service, node);

  SetAlerting(spec_.alerting());

  NotifyChanged();
}

void ObjectVisibleNode::OnBlink(bool state) {
  assert(alerting_);
  NotifyChanged();
}

std::wstring ObjectVisibleNode::GetText() const {
  return spec_.GetCurrentString(FORMAT_DEFAULT);
}

bool ObjectVisibleNode::IsBad() const {
  return spec_.current().qualifier.general_bad();
}

bool ObjectVisibleNode::IsAlerting() const {
  return alerting_ && Blinker::GetState();
}

void ObjectVisibleNode::SetAlerting(bool alerting) {
  if (alerting == alerting_)
    return;

  alerting_ = alerting;

  if (alerting_)
    Blinker::Start();
  else
    Blinker::Stop();

  NotifyChanged();
}
