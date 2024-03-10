#pragma once

#include "configuration/tree/configuration_tree_model.h"

#include <memory>

class Executor;
class NodeService;
class TimedDataService;

struct HardwareTreeModelContext {
  const std::shared_ptr<Executor> executor_;
  NodeService& node_service_;
  TimedDataService& timed_data_service_;
};

class HardwareTreeModel : public ConfigurationTreeModel {
 public:
  explicit HardwareTreeModel(HardwareTreeModelContext&& context);
  virtual ~HardwareTreeModel();

  TimedDataService& timed_data_service() { return timed_data_service_; }

 protected:
  // ConfigurationTreeModel
  virtual std::unique_ptr<ConfigurationTreeNode> CreateTreeNode(
      const scada::NodeId& reference_type_id,
      bool forward_reference,
      const NodeRef& node) override;

 private:
  TimedDataService& timed_data_service_;

  class DeviceTreeNode;
};
