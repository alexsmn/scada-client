#pragma once

#include "base/any_executor.h"

#include "aui/color.h"
#include "configuration/configuration_module.h"
#include "configuration/tree/configuration_tree_model.h"
#include "services/device_state_notifier.h"

#include <memory>
#include <optional>

class NodeService;
class TimedDataService;

struct HardwareTreeModelContext {
  const AnyExecutor executor_;
  NodeService& node_service_;
  TimedDataService& timed_data_service_;
  const NodeServiceTreeFactory& node_service_tree_factory_;
};

class HardwareTreeModel : public ConfigurationTreeModel {
 public:
  explicit HardwareTreeModel(HardwareTreeModelContext&& context);
  virtual ~HardwareTreeModel();

  TimedDataService& timed_data_service() { return timed_data_service_; }
  std::optional<DeviceState> GetDeviceStateForTesting(
      void* tree_node) const;

  // aui::TreeModel: the reshell hardware-tree status dot. Returns the
  // device-state colour (Online/Offline/Disabled) for a device node under the
  // token theme, or nullopt (no dot) for non-device nodes and the legacy theme.
  virtual std::optional<aui::Color> GetStatusColor(void* tree_node) override;

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
