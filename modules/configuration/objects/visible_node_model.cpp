#include "configuration/objects/visible_node_model.h"

#include "aui/severity_colors.h"
#include "base/check.h"
#include "configuration/tree/configuration_tree_model.h"
#include "model/data_items_node_ids.h"
#include "profile/profile.h"
#include "services/data_quality.h"
#include "services/device_state_notifier.h"
#include "timed_data/timed_data_property.h"

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

bool VisibleNodeModel::HasNode(void* tree_node,
                               const std::shared_ptr<VisibleNode>& node) const {
  auto i = nodes_.find(tree_node);
  return i != nodes_.end() && i->second == node;
}

std::u16string VisibleNodeModel::GetText(void* tree_node) {
  auto* node = GetNode(tree_node);
  if (!node)
    return std::u16string();
  return node->GetText();
}

scada::aui::Color VisibleNodeModel::GetTextColor(void* tree_node) {
  auto* node = GetNode(tree_node);
  if (!node || node->IsBad())
    return profile_.bad_value_color;

  // An uncertain (stale) value is tinted like the table's value cells, so it
  // neither passes for good nor borrows the bad colour.
  if (node->GetQuality() == scada::aui::Quality::kUncertain)
    return scada::aui::QualityColor(scada::aui::Quality::kUncertain);

  return scada::aui::ColorCode::Transparent;
}

scada::aui::Color VisibleNodeModel::GetBackgroundColor(void* tree_node) {
  auto* node = GetNode(tree_node);
  if (node && node->IsAlerting())
    return profile_.alarm_color;

  return scada::aui::ColorCode::Transparent;
}

std::optional<scada::aui::StatusIndicator> VisibleNodeModel::GetStatusIndicator(
    void* tree_node) {
  const VisibleNode* node = GetNode(tree_node);
  if (!node)
    return std::nullopt;  // no live value (folder/object): no status dot

  // Only data variables that have delivered a value carry a quality: a group's
  // Value column is its device's connection state, and an unresolved row has
  // nothing to vouch for. Alarm state is deliberately absent -- the dot used to
  // turn amber for an alerting node, which read as doubtful data (backlog
  // 1120); the alarm blinks the Value cell's background instead.
  const std::optional<scada::aui::Quality> quality = node->GetQuality();
  if (!quality)
    return std::nullopt;
  return scada::aui::StatusIndicator{.quality = *quality,
                                     .label = DataQualityLabel(*quality)};
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
  scada::base::Check(!change_handler_);
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

  NotifyChanged();
}

std::u16string ProxyVisibleNode::GetText() const {
  return underlying_node_ ? underlying_node_->GetText() : std::u16string{};
}

bool ProxyVisibleNode::IsBad() const {
  return underlying_node_ && underlying_node_->IsBad();
}

bool ProxyVisibleNode::IsAlerting() const {
  return underlying_node_ && underlying_node_->IsAlerting();
}

std::optional<scada::aui::Quality> ProxyVisibleNode::GetQuality() const {
  return underlying_node_ ? underlying_node_->GetQuality() : std::nullopt;
}

// DataItemVisibleNode

DataItemVisibleNode::DataItemVisibleNode(TimedDataService& timed_data_service,
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

void DataItemVisibleNode::OnBlink(bool state) {
  scada::base::Check(alerting_);
  NotifyChanged();
}

std::u16string DataItemVisibleNode::GetText() const {
  return spec_.GetCurrentString(ValueFormat{FORMAT_DEFAULT});
}

bool DataItemVisibleNode::IsBad() const {
  return GetQuality() == scada::aui::Quality::kBad;
}

std::optional<scada::aui::Quality> DataItemVisibleNode::GetQuality() const {
  // Subscribed is not the same as delivered: until a value arrives the
  // qualifier is a default zero, which is "not bad", and DataQualityFor
  // answers std::nullopt for it rather than the good band.
  return DataQualityFor(spec_.current());
}

bool DataItemVisibleNode::IsAlerting() const {
  return alerting_ && Blinker::GetState();
}

void DataItemVisibleNode::SetAlerting(bool alerting) {
  if (alerting == alerting_)
    return;

  alerting_ = alerting;

  if (alerting_)
    Blinker::Start();
  else
    Blinker::Stop();

  NotifyChanged();
}

// DataGroupVisibleNode

DataGroupVisibleNode::DataGroupVisibleNode(TimedDataService& timed_data_service,
                                           NodeRef node)
    : timed_data_service_{timed_data_service},
      node_{node},
      model_changed_connection_{node_.SubscribeModelChanged(
          [this](const scada::ModelChangeEvent& event) {
            OnModelChanged(event);
          })} {
  UpdateDevice();
}

DataGroupVisibleNode::~DataGroupVisibleNode() = default;

std::u16string DataGroupVisibleNode::GetText() const {
  return device_state_notifier_
             ? ToLocalizedString(device_state_notifier_->device_state())
             : std::u16string{};
}

bool DataGroupVisibleNode::IsBad() const {
  return device_state_notifier_ &&
         device_state_notifier_->device_state() != DeviceState::Online;
}

void DataGroupVisibleNode::UpdateDevice() {
  auto device = node_.target(scada::data_items::id::HasDevice);

  if (device_ == device)
    return;

  device_state_notifier_.reset();
  device_fetched_connection_.disconnect();

  device_ = std::move(device);

  if (device_)
    WatchDeviceState();

  NotifyChanged();
}

void DataGroupVisibleNode::WatchDeviceState() {
  // DeviceStateNotifier requires a *fetched* node — it reads the device's node
  // id to build its component ids, and asserts residency rather than silently
  // monitoring nothing. A HasDevice target routinely is not fetched here: the
  // group lives in the Objects tree and its device lives under Devices, which
  // that tree never browses, so `target()` resolves the reference and hands
  // back a node carrying no state. Constructing the notifier on it panicked
  // the client outright, which is why no group in the screenshot fixture had
  // ever been bound to a device.
  //
  // Fetching NodeOnly is enough: the notifier addresses the runtime components
  // by constructed node id rather than by browsing them.
  if (device_.fetched()) {
    device_state_notifier_ = std::make_unique<DeviceStateNotifier>(
        timed_data_service_, device_, [this] { NotifyChanged(); });
    return;
  }

  device_fetched_connection_ =
      device_.SubscribeNodeFetched([this](const NodeFetchedEvent&) {
        if (device_state_notifier_ || !device_.fetched())
          return;
        device_state_notifier_ = std::make_unique<DeviceStateNotifier>(
            timed_data_service_, device_, [this] { NotifyChanged(); });
        NotifyChanged();
      });

  // Subscribe before starting: a synchronous service (the screenshot fixture,
  // the unit tests) completes the fetch inside this call.
  device_.StartFetch(NodeFetchStatus::NodeOnly);
}

void DataGroupVisibleNode::OnModelChanged(
    const scada::ModelChangeEvent& event) {
  if (event.verb & (scada::ModelChangeEvent::ReferenceAdded |
                    scada::ModelChangeEvent::ReferenceDeleted)) {
    UpdateDevice();
  }
}
