#pragma once

#include "common_resources.h"
#include "components/configuration_tree/configuration_tree_drop_handler.h"
#include "components/configuration_tree/configuration_tree_view.h"
#include "components/hardware_tree/hardware_tree_model.h"
#include "model/scada_node_ids.h"

class HardwareTreeView : public ConfigurationTreeView {
 public:
  explicit HardwareTreeView(const ControllerContext& context)
      : ConfigurationTreeView{context, CreateConfigurationTreeModel(context),
                              CreateTreeDropHandler(context)} {}

 private:
  static std::shared_ptr<ConfigurationTreeModel> CreateConfigurationTreeModel(
      const ControllerContext& context) {
    auto model = std::make_shared<HardwareTreeModel>(HardwareTreeModelContext{
        .executor_ = context.executor_,
        .node_service_ = context.node_service_,
        .timed_data_service_ = context.timed_data_service_});
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
