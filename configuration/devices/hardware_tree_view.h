#pragma once

#include "resources/common_resources.h"
#include "configuration/devices/hardware_tree_model.h"
#include "configuration/tree/configuration_tree_drop_handler.h"
#include "configuration/tree/configuration_tree_view.h"
#include "model/scada_node_ids.h"

class HardwareTreeView : public ConfigurationTreeView {
 public:
  HardwareTreeView(const ControllerContext& context,
                   const NodeServiceTreeFactory& node_service_tree_factory)
      : ConfigurationTreeView{
            context,
            CreateConfigurationTreeModel(context, node_service_tree_factory),
            CreateTreeDropHandler(context)} {}

 private:
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
            context.node_service_,
            context.task_manager_,
            context.create_tree_,
        });
  }
};
