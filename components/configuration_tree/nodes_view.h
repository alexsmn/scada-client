#pragma once

#include "components/configuration_tree/configuration_tree_drop_handler.h"
#include "components/configuration_tree/configuration_tree_model.h"
#include "components/configuration_tree/configuration_tree_view.h"
#include "node_service/node_service.h"

class NodesView : public ConfigurationTreeView {
 public:
  explicit NodesView(const ControllerContext& context)
      : ConfigurationTreeView{
            context,
            *new ConfigurationTreeModel{ConfigurationTreeModelContext{
                context.node_service_,
                context.node_service_.GetNode(scada::id::RootFolder),
                {{scada::id::HierarchicalReferences, true}},
                {},
            }},
            *new ConfigurationTreeDropHandler{
                ConfigurationTreeDropHandlerContext{
                    context.node_service_,
                    context.task_manager_,
                }},
        } {}
};
