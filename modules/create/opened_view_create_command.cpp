#include "create/opened_view_create_command.h"

#include "aui/dialog_service.h"
#include "base/check.h"
#include "base/u16format.h"
#include "controller/controller.h"
#include "controller/selection_model.h"
#include "create/create_module.h"
#include "events/local_event_util.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/static_types.h"
#include "modules/create_service_item/create_service_item_dialog.h"
#include "modules/multi_create/multi_create_dialog.h"
#include "net/net_executor_adapter.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_service.h"
#include "resources/common_resources.h"
#include "services/create_tree.h"
#include "services/task_manager.h"
#include "transport/transport_string.h"

OpenedViewCreateCommand::OpenedViewCreateCommand(
    OpenedViewCreateCommandContext&& context)
    : OpenedViewCreateCommandContext{std::move(context)} {}

OpenedViewCreateCommand::~OpenedViewCreateCommand() = default;

CommandHandler* OpenedViewCreateCommand::GetCommandHandler(
    unsigned command_id) {
  switch (command_id) {
    case ID_NEW_SERVICE_ITEMS:
    case ID_ADD_MULTIPLE_ITEMS:
      return CanCreateRecord(data_items::id::DiscreteItemType) ? this : nullptr;

    case ID_NEW_IEC60870_LINK101:
    case ID_NEW_IEC60870_LINK104:
      return CanCreateRecord(devices::id::Iec60870LinkType) ? this : nullptr;
  }

  if (auto node_id = GetCreateCommandTypeId(command_id); !node_id.is_null()) {
    return CanCreateRecord(node_id) ? this : nullptr;
  }

  return nullptr;
}

void OpenedViewCreateCommand::ExecuteCommand(unsigned command_id) {
  switch (command_id) {
    case ID_NEW_IEC60870_LINK101:
      CreateRecord(devices::id::Iec60870LinkType, 0);
      return;
    case ID_NEW_IEC60870_LINK104:
      CreateRecord(devices::id::Iec60870LinkType, 1);
      return;

    case ID_NEW_SERVICE_ITEMS:
      if (auto* selection_model = controller_.GetSelectionModel()) {
        ShowCreateServiceItemDialog(
            dialog_service_,
            {node_service_, task_manager_, selection_model->node().node_id()});
      }
      return;
    case ID_ADD_MULTIPLE_ITEMS:
      if (auto* selection_model = controller_.GetSelectionModel()) {
        ShowMultiCreateDialog(
            dialog_service_,
            {node_service_, task_manager_, selection_model->node().node_id()});
      }
      return;
  }

  if (auto node_id = GetCreateCommandTypeId(command_id); !node_id.is_null()) {
    CreateRecord(node_id, 0);
    return;
  }

  base::NotReached();
}

bool OpenedViewCreateCommand::CanCreateRecord(
    const scada::NodeId& type_node_id) const {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure)) {
    return false;
  }

  auto* selection_model = controller_.GetSelectionModel();
  if (!selection_model) {
    return false;
  }

  return create_tree_.GetCreateParentNode(
             selection_model->node(), controller_.GetRootNode(),
             node_service_.GetNode(type_node_id)) != nullptr;
}

void OpenedViewCreateCommand::CreateRecord(const scada::NodeId& type_node_id,
                                           int tag) {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure)) {
    return;
  }

  auto node_type = node_service_.GetNode(type_node_id);
  if (!node_type) {
    return;
  }

  const auto* selection_model = controller_.GetSelectionModel();
  if (!selection_model) {
    return;
  }

  auto parent_node = create_tree_.GetCreateParentNode(
      selection_model->node(), controller_.GetRootNode(), node_type);
  if (!parent_node) {
    return;
  }

  scada::NodeAttributes attributes;
  scada::NodeProperties properties;

  bool is104 = tag != 0;

  attributes.display_name = node_type.display_name();
  if (type_node_id == devices::id::Iec60870LinkType) {
    attributes.display_name =
        is104 ? u"IEC 60870-104 Link" : u"IEC 60870-101 Link";
  }

  if (type_node_id == devices::id::Iec60870LinkType) {
    auto protocol =
        is104 ? cfg::Iec60870Protocol::IEC104 : cfg::Iec60870Protocol::IEC101;
    properties.emplace_back(devices::id::Iec60870LinkType_Protocol,
                            static_cast<int>(protocol));

    transport::TransportString ts;
    if (is104) {
      ts.SetProtocol(transport::TransportString::TCP);
      ts.SetParam(transport::TransportString::kParamHost, "localhost");
      ts.SetParam(transport::TransportString::kParamPort, 2404);
    } else {
      ts.SetProtocol(transport::TransportString::SERIAL);
      ts.SetParam(transport::TransportString::kParamName, "COM1");
    }
    ts.SetParam(transport::TransportString::kParamActive);
    properties.emplace_back(devices::id::LinkType_Transport, ts.ToString());
  }

  auto title = u16format(L"Creating \"{}\"", attributes.display_name);

  CoSpawn(executor_, cancelation_,
          [this, type_node_id, parent_id = parent_node.node_id(),
           title = std::move(title), attributes = std::move(attributes),
           properties = std::move(properties)]() mutable -> Awaitable<void> {
            co_await CreateRecordAsync(type_node_id, parent_id,
                                       std::move(title), std::move(attributes),
                                       std::move(properties));
          });
}

Awaitable<void> OpenedViewCreateCommand::CreateRecordAsync(
    scada::NodeId type_node_id,
    scada::NodeId parent_id,
    std::u16string title,
    scada::NodeAttributes attributes,
    scada::NodeProperties properties) {
  auto node_id = co_await task_manager_.PostInsertTask(
      {.type_definition_id = type_node_id,
       .parent_id = parent_id,
       .attributes = std::move(attributes),
       .properties = std::move(properties)});

  if (!node_id.ok()) {
    ReportRequestResult(title, node_id.status(), local_events_, profile_);
    co_return;
  }

  ReportRequestResult(title, scada::StatusCode::Good, local_events_, profile_);
  co_await OnCreateRecordCompleteAsync(*node_id);
  co_return;
}

Awaitable<void> OpenedViewCreateCommand::OnCreateRecordCompleteAsync(
    scada::NodeId node_id) {
  auto node = node_service_.GetNode(node_id);
  co_await FetchNode(node);

  controller_.OnViewNodeCreated(node);
  co_await created_node_handler_(std::move(node));
  co_return;
}
