#pragma once

#include "client/components/configuration_tree/configuration_tree_model.h"

class TimedDataService;

class HardwareTreeModel : public ConfigurationTreeModel {
 public:
  HardwareTreeModel(NodeRefService& node_service, TimedDataService& timed_data_service);
  virtual ~HardwareTreeModel();

  TimedDataService& timed_data_service() { return timed_data_service_; }

 protected:
  // ConfigurationTreeModel
  virtual std::unique_ptr<ConfigurationTreeNode> CreateNode(const NodeRef& data_node) override;

 private:
  TimedDataService& timed_data_service_;

  class DeviceTreeNode;
};
