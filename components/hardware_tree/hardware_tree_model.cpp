#include "hardware_tree_model.h"

#include "components/configuration_tree/node_service_tree_impl.h"
#include "model/devices_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "services/device_state_notifier.h"

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
  if (IsInstanceOf(node(), devices::id::DeviceType)) {
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

void HardwareTreeModel::DeviceTreeNode::OnModelChanged() {
  ConfigurationTreeNode::OnModelChanged();
  UpdateNotifier();
}

void HardwareTreeModel::DeviceTreeNode::UpdateNotifier() {
  if (!device_state_notifier_ && node().fetched() &&
      IsInstanceOf(node(), devices::id::DeviceType)) {
    auto& model = static_cast<HardwareTreeModel&>(this->model());
    device_state_notifier_ = std::make_unique<DeviceStateNotifier>(
        model.timed_data_service(), this->node(), [this] { Changed(); });
  }
}

// HardwareTreeModel

HardwareTreeModel::HardwareTreeModel(HardwareTreeModelContext&& context)
    : ConfigurationTreeModel{::ConfigurationTreeModelContext{
          std::make_unique<NodeServiceTreeImpl>(NodeServiceTreeImplContext{
              context.executor_,
              context.node_service_,
              context.node_service_.GetNode(devices::id::Devices),
              {
                  {scada::id::Organizes, true},
                  {scada::id::HasComponent, true},
                  {devices::id::HasTransmissionTarget, false},
              },
              {
                  devices::id::DeviceType,
                  devices::id::Iec61850LogicalNodeType,
                  devices::id::Iec61850ConfigurableObjectType,
                  devices::id::Iec61850DataVariableType,
                  devices::id::Iec61850ControlObjectType,
                  devices::id::TransmissionItemType,
              },
          }),
      }},
      timed_data_service_{context.timed_data_service_} {}

HardwareTreeModel::~HardwareTreeModel() {}

std::unique_ptr<ConfigurationTreeNode> HardwareTreeModel::CreateTreeNode(
    const scada::NodeId& reference_type_id,
    bool forward_reference,
    const NodeRef& node) {
  return std::make_unique<DeviceTreeNode>(*this, reference_type_id,
                                          forward_reference, node);
}
