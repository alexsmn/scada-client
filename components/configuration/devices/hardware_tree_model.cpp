#include "hardware_tree_model.h"

#include "configuration/tree/node_service_tree_impl.h"
#include "model/devices_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "services/device_state_notifier.h"

namespace {

bool IsHardwareDeviceNode(const NodeRef& node) {
  return IsInstanceOf(node, devices::id::DeviceType) ||
         IsInstanceOf(node, devices::id::ModbusDeviceType) ||
         IsInstanceOf(node, devices::id::Iec60870DeviceType) ||
         IsInstanceOf(node, devices::id::Iec61850DeviceType);
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
    auto device_state = device_state_notifier_
                            ? device_state_notifier_->device_state()
                            : DeviceState::Unknown;
    switch (device_state) {
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
              .root_node_ = context.node_service_.GetNode(devices::id::Devices),
              .reference_filter_ = {{scada::id::Organizes, true},
                                    {scada::id::HasComponent, true}},
              .type_definition_ids_ =
                  {devices::id::DeviceType,
                   devices::id::LinkType,
                   devices::id::ModbusLinkType,
                   devices::id::ModbusDeviceType,
                   devices::id::Iec60870LinkType,
                   devices::id::Iec60870DeviceType,
                   devices::id::Iec61850DeviceType,
                   devices::id::Iec61850LogicalNodeType,
                   devices::id::Iec61850ConfigurableObjectType,
                   devices::id::Iec61850DataVariableType,
                   devices::id::Iec61850ControlObjectType,
                   devices::id::TransmissionItemType}}),
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

std::unique_ptr<ConfigurationTreeNode> HardwareTreeModel::CreateTreeNode(
    const scada::NodeId& reference_type_id,
    bool forward_reference,
    const NodeRef& node) {
  return std::make_unique<DeviceTreeNode>(*this, reference_type_id,
                                          forward_reference, node);
}
