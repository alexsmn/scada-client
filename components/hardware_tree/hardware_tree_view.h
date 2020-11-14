#pragma once

#include "common_resources.h"
#include "components/configuration_tree/configuration_tree_view.h"
#include "components/hardware_tree/hardware_tree_model.h"
#include "model/scada_node_ids.h"

class HardwareTreeView : public ConfigurationTreeView {
 public:
  explicit HardwareTreeView(const ControllerContext& context)
      : ConfigurationTreeView{context,
                              *new HardwareTreeModel(HardwareTreeModelContext{
                                  context.node_service_,
                                  context.task_manager_,
                                  context.timed_data_service_,
                              })} {}
};