#include "opened_view_commands.h"

#include "aui/dialog_service.h"
#include "base/awaitable.h"
#include "base/awaitable_promise.h"
#include "base/program_options.h"
#include "base/excel.h"
#include "base/u16format.h"
#include "base/win/win_util2.h"
#include "clipboard/clipboard_util.h"
#include "resources/common_resources.h"
#include "components/create_service_item/create_service_item_dialog.h"
#include "components/multi_create/multi_create_dialog.h"
#include "components/node_properties/node_property_component.h"
#include "components/time_range/time_range_dialog.h"
#include "controller/controller.h"
#include "controller/controller_registry.h"
#include "controller/selection_model.h"
#include "controller/time_model.h"
#include "events/local_event_util.h"
#include "export/csv/csv_export_command.h"
#include "export/csv/csv_export_util.h"
#include "export/export_model.h"
#include "main_window/actions.h"
#include "main_window/main_window.h"
#include "main_window/main_window_util.h"
#include "main_window/opened_view.h"
#include "main_window/selection_commands.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/static_types.h"
#include "transport/transport_string.h"
#include "node_service/node_promises.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "print/service/print_service.h"
#include "scada/node_management_service.h"
#include "scada/session_service.h"
#include "scada/status_promise.h"
#include "services/create_tree.h"
#include "services/task_manager.h"
#include "window_definition_builder.h"

#if defined(UI_QT)
#include "main_window/qt/main_window_qt.h"
#endif

#if defined(UI_QT)
#include <QMenu>
#endif

namespace {

std::optional<TimeRange> GetTimeRangeCommand(unsigned command_id) {
  switch (command_id) {
    case ID_TIME_RANGE_15M:
      return base::TimeDelta::FromMinutes(15);
    case ID_TIME_RANGE_HOUR:
      return base::TimeDelta::FromHours(1);
    case ID_TIME_RANGE_DAY:
      return TimeRange::Type::Day;
    case ID_TIME_RANGE_WEEK:
      return TimeRange::Type::Week;
    case ID_TIME_RANGE_MONTH:
      return TimeRange::Type::Month;
    case ID_TIME_RANGE_CUSTOM:
      return TimeRange{/*start=*/base::Time{}, /*end=*/base::Time{}};
    default:
      return std::nullopt;
  }
}

}  // namespace

OpenedViewCommands::OpenedViewCommands(OpenedViewCommandsContext&& context)
    : OpenedViewCommandsContext{std::move(context)},
      excel_enabled_{client::HasOption("excel")} {}

OpenedViewCommands::~OpenedViewCommands() {}

void OpenedViewCommands::SetContext(OpenedView* opened_view,
                                    DialogService* dialog_service) {
  // Ensure the controller is initialized.
  assert(!opened_view || &opened_view->controller());

  opened_view_ = opened_view;
  main_window_ = opened_view ? &opened_view->main_window() : nullptr;
  dialog_service_ = dialog_service;
  controller_ = opened_view ? &opened_view->controller() : nullptr;
}

CommandHandler* OpenedViewCommands::GetCommandHandler(unsigned command_id) {
  assert(controller_);

  if (auto* handler = controller_->GetCommandHandler(command_id)) {
    return handler;
  }

  if (auto* handler = selection_commands_->GetCommandHandler(command_id)) {
    return handler;
  }

  switch (command_id) {
    case ID_PASTE:
      return session_service_.HasPrivilege(scada::Privilege::Configure)
                 ? this
                 : nullptr;
    case ID_VIEW_CLOSE:
      return this;

    case ID_EXPORT_EXCEL:
      if (!excel_enabled_)
        return nullptr;
#if defined(UI_QT)
    case ID_PRINT:
      return print_service_ && controller_->GetExportModel() ? this : nullptr;
#endif
    case ID_EXPORT_CSV:
      return controller_->GetExportModel() ? this : nullptr;

    case ID_NEW_SERVICE_ITEMS:
    case ID_ADD_MULTIPLE_ITEMS:
      return CanCreateRecord(data_items::id::DiscreteItemType) ? this : nullptr;

    case ID_NEW_IEC60870_LINK101:
    case ID_NEW_IEC60870_LINK104:
      return CanCreateRecord(devices::id::Iec60870LinkType) ? this : nullptr;
  }

  if (GetTimeRangeCommand(command_id)) {
    return controller_->GetTimeModel() != nullptr ? this : nullptr;
  }

  if (auto node_id = GetNewCommandTypeId(command_id); !node_id.is_null()) {
    return CanCreateRecord(node_id) ? this : nullptr;
  }

  return nullptr;
}

void OpenedViewCommands::ExecuteCommand(unsigned command_id) {
  assert(opened_view_);

  switch (command_id) {
    case ID_PASTE:
      PasteFromClipboard();
      return;
    case ID_VIEW_CLOSE:
      opened_view_->Close();
      return;
    case ID_EXPORT_CSV:
      assert(dialog_service_);
      if (auto* export_model = controller_->GetExportModel()) {
        RunCsvExport({executor_, *dialog_service_, profile_, *export_model,
                      /*window_title=*/opened_view_->GetWindowTitle()});
      }
      return;
    case ID_EXPORT_EXCEL:
      ExportToExcel();
      return;
    case ID_PRINT: {
      assert(print_service_);
      print_service_->ShowPrintPreviewDialog(
          *dialog_service_,
          [opened_view = opened_view_, print_service = print_service_] {
            opened_view->Print(*print_service);
          });
      return;
    }
    case ID_NEW_IEC60870_LINK101:
      CreateRecord(devices::id::Iec60870LinkType, 0);
      return;
    case ID_NEW_IEC60870_LINK104:
      CreateRecord(devices::id::Iec60870LinkType, 1);
      return;

    case ID_NEW_SERVICE_ITEMS:
      if (auto* selection_model = controller_->GetSelectionModel()) {
        ShowCreateServiceItemDialog(
            *dialog_service_,
            {node_service_, task_manager_, selection_model->node().node_id()});
      }
      return;
    case ID_ADD_MULTIPLE_ITEMS:
      if (auto* selection_model = controller_->GetSelectionModel()) {
        ShowMultiCreateDialog(
            *dialog_service_,
            {node_service_, task_manager_, selection_model->node().node_id()});
      }
      return;
  }

  // TODO: Extract `TimeModelCommands`.
  if (auto time_range = GetTimeRangeCommand(command_id)) {
    if (auto* model = controller_->GetTimeModel()) {
      if (time_range->type == TimeRange::Type::Custom) {
        auto range = model->GetTimeRange();
        bool time_required = model->IsTimeRequired();
        auto dialog_promise = ShowTimeRangeDialog(
            *dialog_service_, {profile_, range, time_required});
        // `cancelation_` gates the resumption so a destroyed
        // `OpenedViewCommands` never races the dialog completion — the prior
        // `cancelation_.Bind(...)` callback had the same effect.
        CoSpawn(executor_, cancelation_,
                [executor = executor_, model,
                 dialog_promise = std::move(dialog_promise)]() mutable
                -> Awaitable<void> {
                  auto picked = co_await AwaitPromise(
                      NetExecutorAdapter{executor}, std::move(dialog_promise));
                  model->SetTimeRange(picked);
                  co_return;
                });
      } else {
        model->SetTimeRange(*time_range);
      }
    }
    return;
  }

  if (auto node_id = GetNewCommandTypeId(command_id); !node_id.is_null()) {
    CreateRecord(node_id, 0);
    return;
  }

  // Command is supported but not handled.
  assert(false);
}

bool OpenedViewCommands::IsCommandChecked(unsigned command_id) const {
  if (auto time_range = GetTimeRangeCommand(command_id)) {
    if (auto* model = controller_->GetTimeModel()) {
      auto current_time_range = model->GetTimeRange();
      return time_range->is_custom()
                 ? current_time_range.type == time_range->type
                 : current_time_range == *time_range;
    }
    return false;
  }

  return false;
}

bool OpenedViewCommands::IsCommandEnabled(unsigned command_id) const {
  switch (command_id) {
    case ID_PASTE: {
      auto* selection_model = controller_->GetSelectionModel();
      return selection_model &&
             session_service_.HasPrivilege(scada::Privilege::Configure) &&
             GetPasteParentNode(node_service_, create_tree_,
                                selection_model->node(),
                                controller_->GetRootNode());
    }
    default:
      return true;
  }
}

bool OpenedViewCommands::CanCreateRecord(
    const scada::NodeId& type_node_id) const {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure))
    return false;

  auto* selection_model = controller_->GetSelectionModel();
  if (!selection_model)
    return false;

  return create_tree_.GetCreateParentNode(
             selection_model->node(), controller_->GetRootNode(),
             node_service_.GetNode(type_node_id)) != nullptr;
}

void OpenedViewCommands::CreateRecord(const scada::NodeId& type_node_id,
                                      int tag) {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure))
    return;

  auto node_type = node_service_.GetNode(type_node_id);
  if (!node_type)
    return;

  const auto* selection_model = controller_->GetSelectionModel();
  if (!selection_model)
    return;

  auto parent_node = create_tree_.GetCreateParentNode(
      selection_model->node(), controller_->GetRootNode(), node_type);
  if (!parent_node)
    return;

  scada::NodeAttributes attributes;
  scada::NodeProperties properties;

  bool is104 = tag != 0;

  // name
  attributes.display_name = node_type.display_name();
  if (type_node_id == devices::id::Iec60870LinkType) {
    attributes.display_name =
        is104 ? u"IEC 60870-104 Link" : u"IEC 60870-101 Link";
  }

  // IEC link specific.
  if (type_node_id == devices::id::Iec60870LinkType) {
    // type
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

  // `cancelation_` gates the coroutine the same way `cancelation_.Bind(...)`
  // gated the legacy `.then(...).except(...)` continuations: if `this` is
  // destroyed before either await resumes, the body returns immediately
  // without touching member state.
  CoSpawn(executor_, cancelation_,
          [this, type_node_id, parent_id = parent_node.node_id(),
           title = std::move(title), attributes = std::move(attributes),
           properties = std::move(properties)]() mutable -> Awaitable<void> {
            co_await CreateRecordAsync(type_node_id, parent_id,
                                       std::move(title), std::move(attributes),
                                       std::move(properties));
          });
}

Awaitable<void> OpenedViewCommands::CreateRecordAsync(
    scada::NodeId type_node_id,
    scada::NodeId parent_id,
    std::u16string title,
    scada::NodeAttributes attributes,
    scada::NodeProperties properties) {
  // Single coroutine body replaces the `then(...).except(...)` split: the same
  // `ReportRequestResult` runs on either branch, but now we only have to
  // thread `title` once.
  scada::NodeId node_id;
  std::exception_ptr error;
  try {
    node_id = co_await AwaitPromise(
        NetExecutorAdapter{executor_},
        task_manager_.PostInsertTask({.type_definition_id = type_node_id,
                                      .parent_id = parent_id,
                                      .attributes = std::move(attributes),
                                      .properties = std::move(properties)}));
  } catch (...) {
    error = std::current_exception();
  }

  if (error) {
    ReportRequestResult(title, scada::GetExceptionStatus(error), local_events_,
                        profile_);
    co_return;
  }

  ReportRequestResult(title, scada::StatusCode::Good, local_events_, profile_);
  co_await OnCreateRecordCompleteAsync(node_id);
  co_return;
}

Awaitable<void> OpenedViewCommands::OnCreateRecordCompleteAsync(
    scada::NodeId node_id) {
  auto node = node_service_.GetNode(node_id);
  // Hold `node` across the suspension so it doesn't release before the fetch
  // completes — the original callback form held the same NodeRef in its
  // capture list for the same reason.
  co_await AwaitPromise(NetExecutorAdapter{executor_}, FetchNode(node));

  controller_->OnViewNodeCreated(node);
  auto def = co_await AwaitPromise(
      NetExecutorAdapter{executor_},
      MakeWindowDefinition(&kNodePropertyWindowInfo, node, /*activate=*/false));
  ::OpenView(main_window_, def, true);
  co_return;
}

promise<> OpenedViewCommands::PasteFromClipboard() {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure))
    return MakeRejectedPromise();

  const auto* selection_model = controller_->GetSelectionModel();
  if (!selection_model)
    return MakeRejectedPromise();

  const auto& parent_node =
      GetPasteParentNode(node_service_, create_tree_, selection_model->node(),
                         controller_->GetRootNode());
  if (!parent_node)
    return MakeRejectedPromise();

  return PasteNodesFromClipboard(task_manager_, parent_node.node_id());
}

void OpenedViewCommands::ExportToExcel() {
  auto* export_model = controller_->GetExportModel();
  if (!export_model)
    return;

  auto export_data = export_model->GetExportData();

  try {
    ExcelSheetModel sheet;
    std::visit([&](auto& data) { ::ExportToExcel(data, sheet); }, export_data);

    Excel excel;
    excel.NewWorkbook();
    excel.NewSheet(sheet);
    excel.SetVisible();

  } catch (HRESULT /*err*/) {
    dialog_service_->RunMessageBox(
        u"Export failed. Please check that Microsoft Excel is installed correctly.",
        u"Export", MessageBoxMode::Error);
  }
}
