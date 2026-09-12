#include "create/opened_view_create_command.h"

#include "aui/dialog_service.h"
#include "base/check.h"
#include "base/u16format.h"
#include "bulk_create/qt/bulk_create_wizard.h"
#include "controller/controller.h"
#include "controller/selection_model.h"
#include "create/create_module.h"
#include "events/local_event_util.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/static_types.h"
#include "modules/create_service_item/create_service_item_dialog.h"
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
      return CanCreateRecord(scada::data_items::id::DiscreteItemType) ? this
                                                                      : nullptr;

    case ID_NEW_IEC60870_LINK101:
    case ID_NEW_IEC60870_LINK104:
      return CanCreateRecord(scada::devices::id::Iec60870LinkType) ? this
                                                                   : nullptr;
  }

  if (auto node_id = GetCreateCommandTypeId(command_id); !node_id.is_null()) {
    return CanCreateRecord(node_id) ? this : nullptr;
  }

  return nullptr;
}

void OpenedViewCreateCommand::ExecuteCommand(unsigned command_id) {
  switch (command_id) {
    case ID_NEW_IEC60870_LINK101:
      CreateRecord(scada::devices::id::Iec60870LinkType, 0);
      return;
    case ID_NEW_IEC60870_LINK104:
      CreateRecord(scada::devices::id::Iec60870LinkType, 1);
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
        // The wizard the screens draw, replacing the flat dialog
        // (docs/product/ui-mockups/screens/bulk-create.html). It hands over no
        // source nodes, so this entry offers the data-item subject alone --
        // a transmission rule forwards an existing node, and nothing here has
        // selected any to forward.
        ShowBulkCreateWizard(
            dialog_service_,
            {node_service_, task_manager_, selection_model->node().node_id()});
      }
      return;
  }

  if (auto node_id = GetCreateCommandTypeId(command_id); !node_id.is_null()) {
    CreateRecord(node_id, 0);
    return;
  }

  scada::base::NotReached();
}

bool OpenedViewCreateCommand::CanCreateRecord(
    const scada::NodeId& type_node_id) const {
  if (!session_service_.HasPermission(scada::Permission::kAddNode)) {
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
  if (!session_service_.HasPermission(scada::Permission::kAddNode)) {
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
  if (type_node_id == scada::devices::id::Iec60870LinkType) {
    attributes.display_name =
        is104 ? u"IEC 60870-104 Link" : u"IEC 60870-101 Link";
  }

  if (type_node_id == scada::devices::id::Iec60870LinkType) {
    auto protocol = is104 ? scada::cfg::Iec60870Protocol::IEC104
                          : scada::cfg::Iec60870Protocol::IEC101;
    properties.emplace_back(scada::devices::id::Iec60870LinkType_Protocol,
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
    properties.emplace_back(scada::devices::id::LinkType_Transport,
                            ts.ToString());
  }

  auto title = u16format(L"Creating \"{}\"", attributes.display_name.text);

  // The cancelation overload of `CoSpawn` checks the token once, before the
  // first resume. Everything after the awaits below re-checks it: the operator
  // can close the tab while the insert is in flight, and `ViewManager` deletes
  // the view, its controller and this command synchronously, so a resumed
  // frame must not touch `this` or `controller_` once the token has expired.
  CoSpawn(executor_, cancelation_,
          [this, type_node_id, parent_id = parent_node.node_id(),
           title = std::move(title), attributes = std::move(attributes),
           properties = std::move(properties),
           cancelation = cancelation_.ref()]() mutable -> Awaitable<void> {
            co_await CreateRecordAsync(type_node_id, parent_id,
                                       std::move(title), std::move(attributes),
                                       std::move(properties), cancelation);
          });
}

Awaitable<void> OpenedViewCreateCommand::CreateRecordAsync(
    scada::NodeId type_node_id,
    scada::NodeId parent_id,
    std::u16string title,
    scada::NodeAttributes attributes,
    scada::NodeProperties properties,
    CancelationRef cancelation) {
  // Copied out before the await: the event journal and the profile outlive
  // this command, so a result that arrives after the view closed is still
  // reported — through the copies, never through `this`.
  LocalEvents& local_events = local_events_;
  Profile& profile = profile_;
  TaskManager& task_manager = task_manager_;

  auto node_id = co_await task_manager.PostInsertTask(
      {.type_definition_id = type_node_id,
       .parent_id = parent_id,
       .attributes = std::move(attributes),
       .properties = std::move(properties)});

  if (!node_id.ok()) {
    ReportRequestResult(title, node_id.status(), local_events, profile);
    co_return;
  }

  ReportRequestResult(title, scada::StatusCode::Good, local_events, profile);
  if (cancelation.canceled()) {
    co_return;
  }
  co_await OnCreateRecordCompleteAsync(*node_id, cancelation);
  co_return;
}

Awaitable<void> OpenedViewCreateCommand::OnCreateRecordCompleteAsync(
    scada::NodeId node_id,
    CancelationRef cancelation) {
  auto node = node_service_.GetNode(node_id);
  co_await FetchNode(node);
  if (cancelation.canceled()) {
    co_return;
  }

  // The handler is copied so that the view it may open after its own awaits
  // is reached without `this`, which the operator can have closed meanwhile.
  CreatedNodeHandler created_node_handler = created_node_handler_;
  controller_.OnViewNodeCreated(node);
  co_await created_node_handler(std::move(node));
  co_return;
}
