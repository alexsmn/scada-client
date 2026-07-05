#include "create/create_module.h"

#include "aui/translation.h"
#include "controller/action.h"
#include "controller/action_manager.h"
#include "controller/command_ui_registry.h"
#include "create/opened_view_create_command.h"
#include "main_window/opened_view/opened_view_command_registry.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/history_node_ids.h"
#include "model/security_node_ids.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "resources/common_resources.h"

namespace {

const scada::NodeId kNewCommandTypeIds[] = {
    data_items::id::DataGroupType,
    data_items::id::DiscreteItemType,
    data_items::id::AnalogItemType,
    security::id::UserType,
    history::id::HistoricalDatabaseType,
    data_items::id::SimulationSignalType,
    devices::id::Iec60870DeviceType,
    devices::id::Iec61850DeviceType,
    devices::id::Iec61850RcbType,
    devices::id::ModbusLinkType,
    devices::id::ModbusDeviceType,
    data_items::id::TsFormatType,
    devices::id::ModbusTransmissionItemType,
    devices::id::Iec60870TransmissionItemType,
    devices::id::Iec61850TransmissionItemType,
};

class NodeActionTitle {
 public:
  NodeActionTitle(ActionManager& action_manager,
                  unsigned command_id,
                  NodeRef node)
      : action_manager_{action_manager},
        command_id_{command_id},
        node_{std::move(node)},
        node_semantic_changed_connection_{
            node_.SubscribeNodeSemanticChanged([this](const scada::NodeId&) {
              action_manager_.NotifyActionChanged(command_id_,
                                                  ActionChangeMask::Title);
            })} {}

  std::u16string GetTitle() const { return ToString16(node_.display_name()); }

 private:
  ActionManager& action_manager_;
  const unsigned command_id_;
  const NodeRef node_;

  boost::signals2::scoped_connection node_semantic_changed_connection_;
};

Action MakeNodeAction(ActionManager& action_manager,
                      unsigned command_id,
                      CommandCategory category,
                      NodeRef node) {
  auto title = std::make_shared<NodeActionTitle>(action_manager, command_id,
                                                 std::move(node));
  return Action{.command_id_ = command_id,
                .category_ = category,
                .title_provider_ = [title] { return title->GetTitle(); }};
}

}  // namespace

scada::NodeId GetCreateCommandTypeId(unsigned command_id) {
  if (command_id < ID_NEW) {
    return scada::NodeId();
  }

  auto index = command_id - ID_NEW;
  if (index >= std::size(kNewCommandTypeIds)) {
    return scada::NodeId();
  }

  return kNewCommandTypeIds[index];
}

CreateModule::CreateModule(CreateModuleContext&& context)
    : CreateModuleContext{std::move(context)} {
  ui_command_registry_.AddAction(
      Action{.command_id_ = ID_ADD_MULTIPLE_ITEMS,
             .category_ = CATEGORY_CREATE,
             .title_ = Translate("Multiple Create...")});
  ui_command_registry_.AddAction(
      Action{.command_id_ = ID_NEW_SERVICE_ITEMS,
             .category_ = CATEGORY_CREATE,
             .title_ = Translate("Service Items...")});
  ui_command_registry_.AddAction(
      Action{.command_id_ = ID_NEW_IEC60870_LINK101,
             .category_ = CATEGORY_CREATE,
             .title_ = Translate("IEC 60870-101 Link")});
  ui_command_registry_.AddAction(
      Action{.command_id_ = ID_NEW_IEC60870_LINK104,
             .category_ = CATEGORY_CREATE,
             .title_ = Translate("IEC 60870-104 Link")});

  for (size_t i = 0; i < std::size(kNewCommandTypeIds); ++i) {
    ui_command_registry_.AddAction(MakeNodeAction(
        ui_command_registry_.action_manager(), ID_NEW + i, CATEGORY_CREATE,
        node_service_.GetNode(kNewCommandTypeIds[i])));
  }

  opened_view_commands_.AddFactory(
      [](const OpenedViewCommandFactoryContext& context) {
        return std::make_unique<OpenedViewCreateCommand>(
            OpenedViewCreateCommandContext{
                .executor_ = context.executor_,
                .dialog_service_ = context.dialog_service_,
                .session_service_ = context.session_service_,
                .node_service_ = context.node_service_,
                .task_manager_ = context.task_manager_,
                .local_events_ = context.local_events_,
                .profile_ = context.profile_,
                .create_tree_ = context.create_tree_,
                .controller_ = context.controller_,
                .created_node_handler_ = context.created_node_handler_});
      });
}
