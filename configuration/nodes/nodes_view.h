#pragma once

#include "configuration/tree/configuration_tree_view.h"

struct ControllerContext;

class NodesView : public ConfigurationTreeView {
 public:
  explicit NodesView(const ControllerContext& context);

 private:
  static std::shared_ptr<ConfigurationTreeModel> CreateConfigurationTreeModel(
      const ControllerContext& context);

  static std::unique_ptr<ConfigurationTreeDropHandler>
  CreateConfigurationTreeDropHandler(const ControllerContext& context);
};
