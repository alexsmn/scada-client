#include "configuration/configuration_module.h"

#include "configuration/configuration_tree_component.h"
#include "configuration/devices/hardware_tree_view.h"
#include "configuration/nodes/nodes_view.h"
#include "configuration/objects/object_tree_view.h"
#include "controller/controller_registry.h"
#include "profile/profile.h"

ConfigurationModule::ConfigurationModule(
    ConfigurationModuleContext&& context)
    : ConfigurationModuleContext{std::move(context)} {
  controller_registry_.AddControllerFactory(
      kObjectTreeWindowInfo, [](const ControllerContext& context) {
        return std::make_unique<ObjectTreeView>(context);
      });

  controller_registry_.AddControllerFactory(
      kHardwareTreeWindowInfo, [](const ControllerContext& context) {
        return std::make_unique<HardwareTreeView>(context);
      });

  controller_registry_.AddControllerFactory(
      kNodesWindowInfo, [](const ControllerContext& context) {
        return std::make_unique<NodesView>(context);
      });

  profile_.RegisterWriter([](Profile& profile) {
    // TODO: Add writers.
  });
}
