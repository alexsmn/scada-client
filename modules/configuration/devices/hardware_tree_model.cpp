#include "hardware_tree_model.h"

#include "aui/severity_colors.h"
#include "configuration/devices/device_state_color.h"
#include "configuration/tree/node_service_tree_impl.h"
#include "model/devices_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "services/device_state_notifier.h"

namespace {

bool IsHardwareDeviceNode(const NodeRef& node) {
  return IsInstanceOf(node, scada::devices::id::DeviceType) ||
         IsInstanceOf(node, scada::devices::id::ModbusDeviceType) ||
         IsInstanceOf(node, scada::devices::id::Iec60870DeviceType) ||
         IsInstanceOf(node, scada::devices::id::Iec61850DeviceType);
}

}  // namespace

// HardwareTreeModel::DeviceTreeNode

class HardwareTreeModel::DeviceTreeNode : public ConfigurationTreeNode {
 public:
  DeviceTreeNode(HardwareTreeModel& model,
                 scada::NodeId reference_type_id,
                 bool forward_reference,
                 const NodeRef& node);

  // ConfigurationTreeNode
  virtual void OnModelChanged() override;

  // TreeNode
  virtual int GetIcon() const override;

  // The device's connection state: the live DeviceStateNotifier value, falling
  // back to the node's Value-attribute snapshot (DeviceStateFromNode) while the
  // notifier still reads Unknown (right after load, and in the headless
  // capture, where monitored values are not delivered).
  DeviceState device_state() const;

  std::optional<DeviceState> GetDeviceStateForTesting() const;

 private:
  void UpdateNotifier();

  std::unique_ptr<DeviceStateNotifier> device_state_notifier_;
};

HardwareTreeModel::DeviceTreeNode::DeviceTreeNode(
    HardwareTreeModel& model,
    scada::NodeId reference_type_id,
    bool forward_reference,
    const NodeRef& node)
    : ConfigurationTreeNode{model, std::move(reference_type_id),
                            forward_reference, node} {
  UpdateNotifier();
}

int HardwareTreeModel::DeviceTreeNode::GetIcon() const {
  if (IsHardwareDeviceNode(node())) {
    switch (device_state()) {
      case DeviceState::Disabled:
        return IMAGE_DEVICE_DISABLED;
      case DeviceState::Offline:
        return IMAGE_DEVICE_STOPPED;
      case DeviceState::Online:
        return IMAGE_DEVICE_RUNNING;
      case DeviceState::Unknown:
      default:
        return IMAGE_DEVICE;
    }
  }

  return ConfigurationTreeNode::GetIcon();
}

DeviceState HardwareTreeModel::DeviceTreeNode::device_state() const {
  if (!IsHardwareDeviceNode(node()))
    return DeviceState::Unknown;
  DeviceState state = device_state_notifier_
                          ? device_state_notifier_->device_state()
                          : DeviceState::Unknown;
  if (state == DeviceState::Unknown)
    state = DeviceStateFromNode(node());
  return state;
}

std::optional<DeviceState>
HardwareTreeModel::DeviceTreeNode::GetDeviceStateForTesting() const {
  if (!device_state_notifier_)
    return std::nullopt;
  return device_state_notifier_->device_state();
}

void HardwareTreeModel::DeviceTreeNode::OnModelChanged() {
  ConfigurationTreeNode::OnModelChanged();
  UpdateNotifier();
}

void HardwareTreeModel::DeviceTreeNode::UpdateNotifier() {
  if (!device_state_notifier_ && node().fetched() &&
      IsHardwareDeviceNode(node())) {
    auto& model = static_cast<HardwareTreeModel&>(this->model());
    device_state_notifier_ = std::make_unique<DeviceStateNotifier>(
        model.timed_data_service(), this->node(), [this] { Changed(); });
  }
}

// HardwareTreeModel

HardwareTreeModel::HardwareTreeModel(HardwareTreeModelContext&& context)
    : ConfigurationTreeModel{::ConfigurationTreeModelContext{
          .executor_ = context.executor_,
          .node_service_tree_ =
              context.node_service_tree_factory_(NodeServiceTreeImplContext{
                  .executor_ = context.executor_,
                  .node_service_ = context.node_service_,
                  .root_node_ = context.node_service_.GetNode(
                      scada::devices::id::Devices),
                  .reference_filter_ = {{scada::id::Organizes, true},
                                        {scada::id::HasComponent, true}},
                  .type_definition_ids_ =
                      {scada::devices::id::DeviceType,
                       scada::devices::id::LinkType,
                       scada::devices::id::ModbusLinkType,
                       scada::devices::id::ModbusDeviceType,
                       scada::devices::id::Iec60870LinkType,
                       scada::devices::id::Iec60870DeviceType,
                       scada::devices::id::Iec61850DeviceType,
                       scada::devices::id::Iec61850LogicalNodeType,
                       scada::devices::id::Iec61850ConfigurableObjectType,
                       scada::devices::id::Iec61850DataVariableType,
                       scada::devices::id::Iec61850ControlObjectType,
                       scada::devices::id::TransmissionItemType}}),
      }},
      timed_data_service_{context.timed_data_service_} {}

HardwareTreeModel::~HardwareTreeModel() {}

std::optional<DeviceState> HardwareTreeModel::GetDeviceStateForTesting(
    void* tree_node) const {
  auto* device_tree_node = dynamic_cast<DeviceTreeNode*>(
      static_cast<ConfigurationTreeNode*>(tree_node));
  return device_tree_node ? device_tree_node->GetDeviceStateForTesting()
                          : std::nullopt;
}

std::optional<scada::aui::Color> HardwareTreeModel::GetStatusColor(
    void* tree_node) {
  auto* device_tree_node = dynamic_cast<DeviceTreeNode*>(
      static_cast<ConfigurationTreeNode*>(tree_node));
  if (!device_tree_node)
    return std::nullopt;
  std::optional<scada::aui::Quality> quality =
      DeviceStateQuality(device_tree_node->device_state());
  if (!quality)
    return std::nullopt;  // Unknown state: no dot.
  return scada::aui::QualityColor(*quality);
}

std::unique_ptr<ConfigurationTreeNode> HardwareTreeModel::CreateTreeNode(
    const scada::NodeId& reference_type_id,
    bool forward_reference,
    const NodeRef& node) {
  return std::make_unique<DeviceTreeNode>(*this, reference_type_id,
                                          forward_reference, node);
}
