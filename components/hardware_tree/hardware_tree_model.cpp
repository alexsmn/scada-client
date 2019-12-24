#include "hardware_tree_model.h"

#include "common/node_ref.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "model/scada_node_ids.h"
#include "services/device_state_notifier.h"

// HardwareTreeModel::DeviceTreeNode

class HardwareTreeModel::DeviceTreeNode : public ConfigurationTreeNode {
 public:
  DeviceTreeNode(HardwareTreeModel& model, const NodeRef& data_node);

  // ConfigurationTreeNode
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;

  // TreeNode
  virtual int GetIcon() const override;

 private:
  void UpdateNotifier();

  std::unique_ptr<DeviceStateNotifier> device_state_notifier_;
};

HardwareTreeModel::DeviceTreeNode::DeviceTreeNode(HardwareTreeModel& model,
                                                  const NodeRef& data_node)
    : ConfigurationTreeNode{model, data_node} {
  UpdateNotifier();
}

int HardwareTreeModel::DeviceTreeNode::GetIcon() const {
  if (IsInstanceOf(data_node(), id::DeviceType)) {
    auto device_state = device_state_notifier_
                            ? device_state_notifier_->device_state()
                            : DEVICE_STATE_UNKNOWN;
    switch (device_state) {
      case DEVICE_STATE_DISABLED:
        return IMAGE_DEVICE_DISABLED;
      case DEVICE_STATE_OFFLINE:
        return IMAGE_DEVICE_STOPPED;
      case DEVICE_STATE_ONLINE:
        return IMAGE_DEVICE_RUNNING;
      case DEVICE_STATE_UNKNOWN:
      default:
        return IMAGE_DEVICE;
    }
  }

  return ConfigurationTreeNode::GetIcon();
}

void HardwareTreeModel::DeviceTreeNode::OnModelChanged(
    const scada::ModelChangeEvent& event) {
  ConfigurationTreeNode::OnModelChanged(event);
  UpdateNotifier();
}

void HardwareTreeModel::DeviceTreeNode::UpdateNotifier() {
  if (!device_state_notifier_ && data_node().fetched() &&
      IsInstanceOf(data_node(), id::DeviceType)) {
    auto& model = static_cast<HardwareTreeModel&>(this->model());
    device_state_notifier_ = std::make_unique<DeviceStateNotifier>(
        model.timed_data_service(), this->data_node(), [this] { Changed(); });
  }
}

// HardwareTreeModel

HardwareTreeModel::HardwareTreeModel(NodeService& node_service,
                                     TaskManager& task_manager,
                                     TimedDataService& timed_data_service)
    : ConfigurationTreeModel{node_service,
                             task_manager,
                             node_service.GetNode(id::Devices),
                             {scada::id::Organizes, scada::id::HasComponent},
                             {id::DeviceType, id::Iec61850LogicalNodeType,
                              id::Iec61850ConfigurableObjectType,
                              id::Iec61850DataVariableType,
                              id::Iec61850ControlObjectType}},
      timed_data_service_{timed_data_service} {}

HardwareTreeModel::~HardwareTreeModel() {}

std::unique_ptr<ConfigurationTreeNode> HardwareTreeModel::CreateNode(
    const NodeRef& data_node) {
  return std::make_unique<DeviceTreeNode>(*this, data_node);
}
