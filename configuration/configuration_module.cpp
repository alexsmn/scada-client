#include "configuration/configuration_module.h"

#include "configuration/devices/hardware_tree_view.h"
#include "configuration/nodes/nodes_view.h"
#include "configuration/objects/object_tree_view.h"
#include "controller/controller_registry.h"
#include "profile/profile.h"

namespace {

static constexpr WindowInfo kObjectTreeWindowInfo = {
    ID_OBJECT_VIEW, "Struct", u"Объекты", WIN_SING, 200, 400, 0};

static constexpr WindowInfo kHardwareTreeWindowInfo = {
    ID_HARDWARE_VIEW, "Subsystems", u"Оборудование", WIN_SING, 200, 400};

static constexpr WindowInfo kNodesWindowInfo = {
    ID_NODES_VIEW, "Nodes", u"Узлы", WIN_SING | WIN_REQUIRES_ADMIN,
    200,           400,     0};

}  // namespace

ConfigurationModule::ConfigurationModule(ConfigurationModuleContext&& context)
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
