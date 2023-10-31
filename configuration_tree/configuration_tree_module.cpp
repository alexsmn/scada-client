#include "configuration_tree/configuration_tree_module.h"

#include "configuration_tree/configuration_tree_component.h"
#include "configuration_tree/hardware/hardware_tree_view.h"
#include "configuration_tree/nodes/nodes_view.h"
#include "configuration_tree/objects/object_tree_view.h"
#include "controller/controller_registry.h"
#include "profile/profile.h"

ConfigurationTreeModule::ConfigurationTreeModule(
    ConfigurationTreeModuleContext&& context)
    : ConfigurationTreeModuleContext{std::move(context)} {
  controller_registry_.AddControllerFactory(
      kObjectTreeWindowInfo, [this](const ControllerContext& context) {
        return std::make_unique<ObjectTreeView>(context);
      });

  controller_registry_.AddControllerFactory(
      kHardwareTreeWindowInfo, [this](const ControllerContext& context) {
        return std::make_unique<HardwareTreeView>(context);
      });

  controller_registry_.AddControllerFactory(
      kNodesWindowInfo, [this](const ControllerContext& context) {
        return std::make_unique<NodesView>(context);
      });

  profile_.RegisterWriter([this](Profile& profile) {
    // TODO: Add writers.
  });
}
