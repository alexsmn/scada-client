#include "configuration/configuration_module.h"

#include "aui/translation.h"
#include "configuration/devices/hardware_tree_view.h"
#include "configuration/nodes/nodes_view.h"
#include "configuration/objects/object_tree_view.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_registry.h"
#include "profile/profile.h"
#include "resources/common_resources.h"

namespace {

static constexpr WindowInfo kObjectTreeWindowInfo = {
    ID_OBJECT_VIEW, "Struct", u"Objects", WIN_SING, 200, 400, 0};

static constexpr WindowInfo kHardwareTreeWindowInfo = {
    ID_HARDWARE_VIEW, "Subsystems", u"Subsystems", WIN_SING, 200, 400};

static constexpr WindowInfo kNodesWindowInfo = {
    ID_NODES_VIEW, "Nodes", u"Nodes", WIN_SING | WIN_REQUIRES_ADMIN,
    200,           400,     0};

}  // namespace

ConfigurationModule::ConfigurationModule(ConfigurationModuleContext&& context)
    : ConfigurationModuleContext{std::move(context)} {
  controller_registry_.AddControllerFactory(
      kObjectTreeWindowInfo,
      [f = node_service_tree_factory_](const ControllerContext& context) {
        return std::make_unique<ObjectTreeView>(context, f);
      });

  controller_registry_.AddControllerFactory(
      kHardwareTreeWindowInfo,
      [f = node_service_tree_factory_](const ControllerContext& context) {
        return std::make_unique<HardwareTreeView>(context, f);
      });

  controller_registry_.AddControllerFactory(
      kNodesWindowInfo,
      [f = node_service_tree_factory_](const ControllerContext& context) {
        return std::make_unique<NodesView>(context, f);
      });

  ui_command_registry_.AddMenuItem({.menu_id = MainMenuId::More,
                                    .order = 100,
                                    .command_id = ID_OBJECT_VIEW,
                                    .title = Translate("Items"),
                                    .checkable = true});
  ui_command_registry_.AddMenuItem({.menu_id = MainMenuId::More,
                                    .order = 150,
                                    .command_id = ID_HARDWARE_VIEW,
                                    .title = Translate("Hardware"),
                                    .checkable = true});
  ui_command_registry_.AddMenuItem({.menu_id = MainMenuId::More,
                                    .order = 200,
                                    .command_id = ID_NODES_VIEW,
                                    .title = Translate("Nodes"),
                                    .checkable = true,
                                    .separator_before = true,
                                    .admin_only = true});
  ui_command_registry_.AddMenuItem({.menu_id = MainMenuId::More,
                                    .order = 210,
                                    .command_id = ID_TS_FORMATS_VIEW,
                                    .title = Translate("Formats"),
                                    .checkable = true,
                                    .admin_only = true});
  ui_command_registry_.AddMenuItem({.menu_id = MainMenuId::More,
                                    .order = 220,
                                    .command_id = ID_SIMULATION_ITEMS_VIEW,
                                    .title = Translate("Simulated Signals"),
                                    .checkable = true,
                                    .admin_only = true});
  ui_command_registry_.AddMenuItem({.menu_id = MainMenuId::More,
                                    .order = 230,
                                    .command_id = ID_USERS_VIEW,
                                    .title = Translate("Users"),
                                    .checkable = true,
                                    .admin_only = true});
  ui_command_registry_.AddMenuItem({.menu_id = MainMenuId::More,
                                    .order = 240,
                                    .command_id = ID_HISTORICAL_DB_VIEW,
                                    .title = Translate("Databases"),
                                    .checkable = true,
                                    .admin_only = true});

  profile_.RegisterWriter([](Profile& profile) {
    // TODO: Add writers.
  });
}
