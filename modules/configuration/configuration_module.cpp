#include "configuration/configuration_module.h"

#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/u16format.h"
#include "configuration/devices/hardware_tree_view.h"
#include "configuration/nodes/nodes_view.h"
#include "configuration/objects/object_tree_view.h"
#include "controller/action.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_registry.h"
#include "controller/selection_model.h"
#include "core/selection_command_context.h"
#include "events/local_event_util.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"
#include "profile/profile.h"
#include "resources/common_resources.h"
#include "scada/session_service.h"
#include "services/task_manager.h"

namespace {

static constexpr WindowInfo kObjectTreeWindowInfo = {
    ID_OBJECT_VIEW, "Struct", u"Objects", WIN_SING, 200, 400, 0};

static constexpr WindowInfo kHardwareTreeWindowInfo = {
    ID_HARDWARE_VIEW, "Subsystems", u"Subsystems", WIN_SING, 200, 400};

static constexpr WindowInfo kNodesWindowInfo = {
    ID_NODES_VIEW, "Nodes", u"Nodes", WIN_SING | WIN_REQUIRES_ADMIN,
    200,           400,     0};

Awaitable<void> ReportMethodCallResultAsync(AnyExecutor executor,
                                            Awaitable<scada::Status> call,
                                            std::u16string title,
                                            LocalEvents& local_events,
                                            const Profile& profile) {
  auto status = co_await std::move(call);
  ReportRequestResult(title, status, local_events, profile);
  co_return;
}

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

  ui_command_registry_.AddAction(Action{.command_id_ = ID_UNLOCK_ITEM,
                                        .category_ = CATEGORY_ITEM,
                                        .title_ = Translate("Unlock"),
                                        .image_id_ = IDB_UNLOCK});
  ui_command_registry_.AddAction(Action{.command_id_ = ID_DEV1_REFR,
                                        .category_ = CATEGORY_DEVICE,
                                        .title_ = Translate("Poll Device")});
  ui_command_registry_.AddAction(
      Action{.command_id_ = ID_DEV1_SYNC,
             .category_ = CATEGORY_DEVICE,
             .title_ = Translate("Synchronize Clock")});
  ui_command_registry_.AddAction(Action{.command_id_ = ID_ITEM_ENABLE,
                                        .category_ = CATEGORY_SPECIFIC,
                                        .title_ = Translate("Enable")});
  ui_command_registry_.AddAction(Action{.command_id_ = ID_ITEM_DISABLE,
                                        .category_ = CATEGORY_SPECIFIC,
                                        .title_ = Translate("Disable")});

  selection_commands_.AddCommand(
      {.command_id = ID_UNLOCK_ITEM,
       .execute_handler =
           [this](const SelectionCommandContext& context) {
             // `PostTask` returns a lazy awaitable — spawn it detached so the
             // task actually runs; the task manager reports completion itself.
             CoSpawn(
                 executor_,
                 [this, node = context.selection.node()]() -> Awaitable<void> {
                   (void)co_await task_manager_.PostTask(
                       u16format(L"Unlocking {}", node.display_name()),
                       [node]() -> Awaitable<scada::Status> {
                         co_return co_await node.scada_node().call(
                             data_items::id::DataItemType_Unlock);
                       });
                 });
           },
       .enabled_handler =
           [this](const SelectionCommandContext& context) {
             return context.selection
                 .node()[data_items::id::DataItemType_Locked]
                 .value()
                 .get_or(false);
           },
       .available_handler =
           [this](const SelectionCommandContext& context) {
             return session_service_.HasPrivilege(scada::Privilege::Control) &&
                    IsInstanceOf(context.selection.node(),
                                 data_items::id::DataItemType);
           }});

  // TODO: Rename constants.
  RegisterEnableDeviceCommand(ID_ITEM_ENABLE, true);
  RegisterEnableDeviceCommand(ID_ITEM_DISABLE, false);
  RegisterMethodCommand(ID_DEV1_REFR, devices::id::DeviceType_Interrogate);
  RegisterMethodCommand(ID_DEV1_SYNC, devices::id::DeviceType_SyncClock);

  profile_.RegisterWriter([](Profile& profile) {
    // TODO: Add writers.
  });
}

void ConfigurationModule::RegisterMethodCommand(
    unsigned command_id,
    const scada::NodeId& method_id) {
  selection_commands_.AddCommand(
      {.command_id = command_id,
       .execute_handler =
           [this, method_id](const SelectionCommandContext& context) {
             CallMethod(context.selection.node(), method_id, /*args=*/{});
           },
       .available_handler =
           [this](const SelectionCommandContext& context) {
             return session_service_.HasPrivilege(scada::Privilege::Control) &&
                    IsInstanceOf(context.selection.node(),
                                 data_items::id::DataItemType);
           }});
}

void ConfigurationModule::CallMethod(
    const NodeRef& node,
    const scada::NodeId& method_id,
    const std::vector<scada::Variant>& arguments) {
  auto scada_node = node.scada_node();
  CoSpawn(executor_,
          [executor = executor_, scada_node = std::move(scada_node), method_id,
           arguments, title = ToString16(node.display_name()),
           &local_events = local_events_,
           &profile = profile_]() mutable -> Awaitable<void> {
            co_await ReportMethodCallResultAsync(
                std::move(executor),
                scada_node.call_packed(method_id, std::move(arguments)),
                std::move(title), local_events, profile);
          });
}

void ConfigurationModule::RegisterEnableDeviceCommand(unsigned command_id,
                                                      bool enable) {
  selection_commands_.AddCommand(
      {.command_id = command_id,
       .execute_handler =
           [this, enable](const SelectionCommandContext& context) {
             // `PostUpdateTask` returns a lazy awaitable — spawn it detached
             // so the task actually runs; the task manager reports completion
             // itself.
             CoSpawn(executor_,
                     [this, node_id = context.selection.node().node_id(),
                      enable]() -> Awaitable<void> {
                       (void)co_await task_manager_.PostUpdateTask(
                           node_id, /*attrs=*/{}, /*props=*/
                           {{devices::id::DeviceType_Disabled, !enable}});
                     });
           },
       .enabled_handler =
           [enable](const SelectionCommandContext& context) {
             return context.selection.node()[devices::id::DeviceType_Disabled]
                        .value()
                        .get_or(false) == enable;
           },
       .available_handler =
           [this](const SelectionCommandContext& context) {
             return session_service_.HasPrivilege(
                        scada::Privilege::Configure) &&
                    context.selection.node()[devices::id::DeviceType_Disabled];
           }});
}
