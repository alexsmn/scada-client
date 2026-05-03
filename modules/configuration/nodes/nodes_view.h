#pragma once

#include "configuration/configuration_module.h"
#include "configuration/tree/configuration_tree_view.h"

struct ControllerContext;

class NodesView : public ConfigurationTreeView {
 public:
  NodesView(const ControllerContext& context,
            const NodeServiceTreeFactory& node_service_tree_factory);

 private:
  static std::shared_ptr<ConfigurationTreeModel> CreateConfigurationTreeModel(
      const ControllerContext& context,
      const NodeServiceTreeFactory& node_service_tree_factory);

  static std::unique_ptr<ConfigurationTreeDropHandler>
  CreateConfigurationTreeDropHandler(const ControllerContext& context);
};
