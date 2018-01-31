#pragma once

#include "components/configuration_tree/configuration_tree_model.h"

class TimedDataService;

class HardwareTreeModel : public ConfigurationTreeModel {
 public:
  HardwareTreeModel(scada::ViewService& view_service,
                    NodeService& node_service,
                    TimedDataService& timed_data_service);
  virtual ~HardwareTreeModel();

  TimedDataService& timed_data_service() { return timed_data_service_; }

 protected:
  // ConfigurationTreeModel
  virtual std::unique_ptr<ConfigurationTreeNode> CreateNode(
      const NodeRef& data_node) override;

 private:
  TimedDataService& timed_data_service_;

  class DeviceTreeNode;
};
