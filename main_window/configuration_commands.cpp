#include "configuration_commands.h"

#include "base/awaitable.h"
#include "base/awaitable_promise.h"
#include "base/u16format.h"
#include "resources/common_resources.h"
#include "components/limits/limit_dialog.h"
#include "components/write/write_dialog.h"
#include "controller/command_registry.h"
#include "controller/selection_model.h"
#include "core/selection_command_context.h"
#include "events/local_event_util.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"
#include "scada/session_service.h"
#include "scada/status_exception.h"
#include "services/task_manager.h"

namespace {

Awaitable<void> ReportMethodCallResultAsync(std::shared_ptr<Executor> executor,
                                            promise<> call_promise,
                                            std::u16string title,
                                            LocalEvents& local_events,
                                            const Profile& profile) {
  scada::Status status = scada::StatusCode::Good;
  try {
    co_await AwaitPromise(NetExecutorAdapter{std::move(executor)},
                          std::move(call_promise));
  } catch (...) {
    status = scada::GetExceptionStatus(std::current_exception());
  }

  ReportRequestResult(title, status, local_events, profile);
}

}  // namespace

void ConfigurationCommands::Register() {
  selection_commands_.AddCommand(
      {.command_id = ID_WRITE,
       .execute_handler =
           [this](const SelectionCommandContext& context) {
             ExecuteWriteDialog(context.dialog_service,
                                WriteContext{executor_, timed_data_service_,
                                             context.selection.node().node_id(),
                                             profile_, false});
           },
       .enabled_handler =
           [](const SelectionCommandContext& context) {
             // TODO: Use `scada::AttributeId::UserWriteMask` when available.
             // Allow writing to all variables. Except for data items: check
             // an output channel is present.
             auto node = context.selection.node();
             return !IsInstanceOf(node, data_items::id::DataItemType) ||
                    !node[data_items::id::DataItemType_Output]
                         .value()
                         .is_null();
           },
       .available_handler =
           [this](const SelectionCommandContext& context) {
             return session_service_.HasPrivilege(scada::Privilege::Control) &&
                    context.selection.node().node_class() ==
                        scada::NodeClass::Variable;
           }});

  selection_commands_.AddCommand(
      {.command_id = ID_WRITE_MANUAL,
       .execute_handler =
           [this](const SelectionCommandContext& context) {
             ExecuteWriteDialog(context.dialog_service,
                                WriteContext{executor_, timed_data_service_,
                                             context.selection.node().node_id(),
                                             profile_, true});
           },
       .available_handler =
           [this](const SelectionCommandContext& context) {
             return session_service_.HasPrivilege(scada::Privilege::Control) &&
                    IsInstanceOf(context.selection.node(),
                                 data_items::id::DataItemType);
           }});

  selection_commands_.AddCommand(
      {.command_id = ID_UNLOCK_ITEM,
       .execute_handler =
           [this](const SelectionCommandContext& context) {
             const auto& node = context.selection.node();
             task_manager_.PostTask(
                 u16format(L"Снятие блокировки с {}",
                                    node.display_name()),
                 [node] {
                   return node.scada_node().call(
                       data_items::id::DataItemType_Unlock);
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

  selection_commands_.AddCommand(
      {.command_id = ID_EDIT_LIMITS,
       .execute_handler =
           [this](const SelectionCommandContext& context) {
             ShowLimitsDialog(context.dialog_service,
                              {context.selection.node(), task_manager_});
           },
       .available_handler =
           [this](const SelectionCommandContext& context) {
             return session_service_.HasPrivilege(scada::Privilege::Control) &&
                    IsInstanceOf(context.selection.node(),
                                 data_items::id::AnalogItemType);
           }});

  // TODO: Rename contants.
  RegisterEnableDeviceCommand(ID_ITEM_ENABLE, true);
  RegisterEnableDeviceCommand(ID_ITEM_DISABLE, false);

  // TODO: Rename contants.
  RegisterMethodCommand(ID_DEV1_REFR, devices::id::DeviceType_Interrogate);
  RegisterMethodCommand(ID_DEV1_SYNC, devices::id::DeviceType_SyncClock);
}

void ConfigurationCommands::RegisterMethodCommand(
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

void ConfigurationCommands::CallMethod(
    const NodeRef& node,
    const scada::NodeId& method_id,
    const std::vector<scada::Variant>& arguments) {
  auto call_promise = node.scada_node().call_packed(method_id, arguments);
  CoSpawn(executor_,
          [executor = executor_, call_promise = std::move(call_promise),
           title = ToString16(node.display_name()),
           &local_events = local_events_,
           &profile = profile_]() mutable -> Awaitable<void> {
            co_await ReportMethodCallResultAsync(
                std::move(executor), std::move(call_promise), std::move(title),
                local_events, profile);
          });
}

void ConfigurationCommands::RegisterEnableDeviceCommand(unsigned command_id,
                                                        bool enable) {
  selection_commands_.AddCommand(
      {.command_id = command_id,
       .execute_handler =
           [this, enable](const SelectionCommandContext& context) {
             task_manager_.PostUpdateTask(
                 context.selection.node().node_id(), /*attrs=*/{}, /*props=*/
                 {{devices::id::DeviceType_Disabled, !enable}});
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
