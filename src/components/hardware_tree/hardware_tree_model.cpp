#include "components/hardware_tree/hardware_tree_model.h"

#include "common/scada_node_ids.h"
#include "services/device_state_notifier.h"
#include "common/node_ref_util.h"

// HardwareTreeModel::DeviceTreeNode

class HardwareTreeModel::DeviceTreeNode : public ConfigurationTreeNode {
 public:
  DeviceTreeNode(HardwareTreeModel& model, NodeRef data_node);

  // ConfigurationTreeNode
  virtual void OnNodeSemanticChanged() override;

  // TreeNode
  virtual int GetIcon() const;

 private:
  void UpdateNotifier();

  std::unique_ptr<DeviceStateNotifier> device_state_notifier_;
};

HardwareTreeModel::DeviceTreeNode::DeviceTreeNode(HardwareTreeModel& model, NodeRef data_node)
    : ConfigurationTreeNode{model, std::move(data_node)} {
  UpdateNotifier();
}

int HardwareTreeModel::DeviceTreeNode::GetIcon() const {
  if (!device_state_notifier_)
    return IMAGE_DEVICE;
  switch (device_state_notifier_->device_state()) {
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

void HardwareTreeModel::DeviceTreeNode::OnNodeSemanticChanged() {
  ConfigurationTreeNode::OnNodeSemanticChanged();
  UpdateNotifier();
}

void HardwareTreeModel::DeviceTreeNode::UpdateNotifier() {
  if (!device_state_notifier_ && this->data_node().fetched()) {
    auto& model = static_cast<HardwareTreeModel&>(this->model());
    device_state_notifier_ = std::make_unique<DeviceStateNotifier>(
        model.timed_data_service(), this->data_node(), [this] { Changed(); });
  }
}

// HardwareTreeModel

HardwareTreeModel::HardwareTreeModel(NodeRefService& node_service, TimedDataService& timed_data_service)
    : ConfigurationTreeModel{node_service, id::Devices, {OpcUaId_Organizes}},
      timed_data_service_{timed_data_service} {
}

HardwareTreeModel::~HardwareTreeModel() {
}

std::unique_ptr<ConfigurationTreeNode> HardwareTreeModel::CreateNode(const NodeRef& data_node) {
  return std::make_unique<DeviceTreeNode>(*this, data_node);
}
