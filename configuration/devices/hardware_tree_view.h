#pragma once

#include "aui/tree.h"
#include "resources/common_resources.h"
#include "configuration/devices/hardware_tree_model.h"
#include "configuration/tree/configuration_tree_drop_handler.h"
#include "configuration/tree/configuration_tree_view.h"
#include "model/devices_node_ids.h"
#include "model/scada_node_ids.h"
#include "node_service/node_util.h"

#include <optional>
#include <string>
#include <vector>

struct HardwareTreeDeviceForTesting {
  std::string protocol;
  std::u16string label;
  bool active = false;
  std::string state;
};

class HardwareTreeView : public ConfigurationTreeView {
 public:
  HardwareTreeView(const ControllerContext& context,
                   const NodeServiceTreeFactory& node_service_tree_factory)
      : ConfigurationTreeView{
            context,
            CreateConfigurationTreeModel(context, node_service_tree_factory),
            CreateTreeDropHandler(context)} {}

  std::vector<HardwareTreeDeviceForTesting> GetExpandedDevicesForTesting() {
    auto* root = model().root();
    std::vector<HardwareTreeDeviceForTesting> devices;
    if (!root)
      return devices;

    auto expand_tree = [&](auto& self, ConfigurationTreeNode& current) -> void {
      if (current.CanFetchMore())
        current.FetchMore();

      tree_view().ExpandNode(&current);
      const auto& node = current.node();
      if (node.fetched()) {
        std::optional<std::string> protocol;
        if (IsInstanceOf(node, devices::id::ModbusDeviceType))
          protocol = "MODBUS";
        else if (IsInstanceOf(node, devices::id::Iec60870DeviceType))
          protocol = "IEC60870";
        else if (IsInstanceOf(node, devices::id::Iec61850DeviceType))
          protocol = "IEC61850";

        if (protocol) {
          auto device_state = model().GetDeviceStateForTesting(&current);
          devices.emplace_back(HardwareTreeDeviceForTesting{
              .protocol = *protocol,
              .label = model().GetText(&current, /*column_id=*/0),
              .active = device_state == DeviceState::Online,
              .state = device_state ? ToString(*device_state) : "Unknown"});
        }
      }

      for (int i = 0; i < current.GetChildCount(); ++i)
        self(self, current.GetChild(i));
    };

    expand_tree(expand_tree, *root);
    return devices;
  }

 private:
  HardwareTreeModel& model() {
    return static_cast<HardwareTreeModel&>(ConfigurationTreeView::model());
  }

  static std::shared_ptr<ConfigurationTreeModel> CreateConfigurationTreeModel(
      const ControllerContext& context,
      const NodeServiceTreeFactory& node_service_tree_factory) {
    auto model = std::make_shared<HardwareTreeModel>(HardwareTreeModelContext{
        .executor_ = context.executor_,
        .node_service_ = context.node_service_,
        .timed_data_service_ = context.timed_data_service_,
        .node_service_tree_factory_ = node_service_tree_factory});
    model->Init();
    return model;
  }

  static std::unique_ptr<ConfigurationTreeDropHandler> CreateTreeDropHandler(
      const ControllerContext& context) {
    return std::make_unique<ConfigurationTreeDropHandler>(
        ConfigurationTreeDropHandlerContext{
            context.executor_,
            context.node_service_,
            context.task_manager_,
            context.create_tree_,
        });
  }
};
