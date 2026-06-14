#include "main_window/opened_view/opened_view_commands.h"

#include "aui/dialog_service.h"
#include "aui/key_codes.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/excel.h"
#include "base/program_options.h"
#include "base/u16format.h"
#include "clipboard/clipboard_util.h"
#include "controller/action.h"
#include "controller/action_manager.h"
#include "controller/command_ui_registry.h"
#include "controller/controller.h"
#include "controller/controller_registry.h"
#include "controller/selection_model.h"
#include "controller/time_model.h"
#include "events/local_event_util.h"
#include "export/csv/csv_export_command.h"
#include "export/csv/csv_export_util.h"
#include "export/export_model.h"
#include "main_window/main_window.h"
#include "main_window/main_window_util.h"
#include "main_window/opened_view/opened_view.h"
#include "main_window/selection_commands.h"
#include "main_window/window_definition_builder.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/history_node_ids.h"
#include "model/security_node_ids.h"
#include "model/static_types.h"
#include "modules/create_service_item/create_service_item_dialog.h"
#include "modules/multi_create/multi_create_dialog.h"
#include "modules/node_properties/node_property_component.h"
#include "modules/time_range/time_range_dialog.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_observer.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "print/service/print_service.h"
#include "resources/common_resources.h"
#include "scada/node_management_service.h"
#include "scada/session_service.h"
#include "services/create_tree.h"
#include "services/task_manager.h"
#include "transport/transport_string.h"

#if defined(UI_QT)
#include "main_window/main_window_qt.h"
#endif

#if defined(UI_QT)
#include <QMenu>
#endif

#include <memory>
#include <stdexcept>
#include <utility>

namespace {

// TODO(semenov): Refactor to avoid listing the types.
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

class NodeActionTitle : private NodeRefObserver {
 public:
  NodeActionTitle(ActionManager& action_manager,
                  unsigned command_id,
                  NodeRef node)
      : action_manager_(action_manager),
        command_id_(command_id),
        node_(std::move(node)) {
    node_.Subscribe(*this);
  }

  ~NodeActionTitle() { node_.Unsubscribe(*this); }

  std::u16string GetTitle() const { return ToString16(node_.display_name()); }

 private:
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override {
    action_manager_.NotifyActionChanged(command_id_, ActionChangeMask::Title);
  }

  ActionManager& action_manager_;
  const unsigned command_id_;
  const NodeRef node_;
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

scada::NodeId GetNewCommandTypeId(unsigned command_id) {
  if (command_id < ID_NEW)
    return scada::NodeId();

  auto index = command_id - ID_NEW;
  if (index >= std::size(kNewCommandTypeIds))
    return scada::NodeId();

  return kNewCommandTypeIds[index];
}

void RegisterOpenedViewCommandActions(UiCommandRegistry& ui_command_registry,
                                      NodeService& node_service) {
  ui_command_registry.AddAction(
      Action{.command_id_ = ID_PASTE,
             .category_ = CATEGORY_EDIT,
             .title_ = Translate("Paste"),
             .image_id_ = IDB_PASTE,
             .shortcut_ = Shortcut{aui::ControlModifier, aui::KeyCode::V}});
  ui_command_registry.AddAction(Action{.command_id_ = ID_PRINT,
                                       .category_ = CATEGORY_SETUP,
                                       .title_ = Translate("Print"),
                                       .image_id_ = IDB_PRINTER});
  ui_command_registry.AddAction(Action{.command_id_ = ID_EXPORT_CSV,
                                       .category_ = CATEGORY_EXPORT,
                                       .title_ = Translate("Export to CSV")});
  ui_command_registry.AddAction(Action{.command_id_ = ID_EXPORT_EXCEL,
                                       .category_ = CATEGORY_EXPORT,
                                       .title_ = Translate("Export to Excel")});
  ui_command_registry.AddAction(Action{.command_id_ = ID_VIEW_LEGEND,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Legend"),
                                       .flags_ = Action::CHECKABLE});
  ui_command_registry.AddAction(Action{.command_id_ = ID_GRAPH_DOTS,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Dots"),
                                       .flags_ = Action::CHECKABLE});
  ui_command_registry.AddAction(Action{.command_id_ = ID_GRAPH_STEPS,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Steps"),
                                       .flags_ = Action::CHECKABLE});
  ui_command_registry.AddAction(Action{.command_id_ = ID_GRAPH_SCROLL_BAR,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Scroll Bar"),
                                       .flags_ = Action::CHECKABLE});
  ui_command_registry.AddAction(Action{.command_id_ = ID_NOW,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Scroll to Now"),
                                       .short_title_ = Translate("Now"),
                                       .flags_ = Action::CHECKABLE});
  ui_command_registry.AddAction(Action{.command_id_ = ID_GRAPH_COLOR,
                                       .category_ = CATEGORY_SETUP,
                                       .title_ = Translate("Line Color..."),
                                       .short_title_ = Translate("Color")});
  ui_command_registry.AddAction(Action{.command_id_ = ID_GRAPH_SETUP,
                                       .category_ = CATEGORY_SETUP,
                                       .title_ = Translate("Graph Setup..."),
                                       .short_title_ = Translate("Setup"),
                                       .image_id_ = ID_GRAPH_VIEW});
  ui_command_registry.AddAction(
      Action{.command_id_ = ID_GRAPH_BK_COLOR,
             .category_ = CATEGORY_SETUP,
             .title_ = Translate("Background Color..."),
             .short_title_ = Translate("Background")});
  ui_command_registry.AddAction(Action{.command_id_ = ID_GRAPH_ADD_PANE,
                                       .category_ = CATEGORY_EDIT,
                                       .title_ = Translate("Add Pane")});
  ui_command_registry.AddAction(Action{.command_id_ = ID_GRAPH_DELETE_PANE,
                                       .category_ = CATEGORY_EDIT,
                                       .title_ = Translate("Delete Pane")});
  ui_command_registry.AddAction(Action{.command_id_ = ID_EDIT,
                                       .category_ = CATEGORY_SETUP,
                                       .title_ = Translate("Edit"),
                                       .flags_ = Action::CHECKABLE});
  ui_command_registry.AddAction(Action{.command_id_ = ID_SAVE,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Save")});
  ui_command_registry.AddAction(Action{.command_id_ = ID_SAVE_AS,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Save As..."),
                                       .short_title_ = Translate("Save")});

  const std::pair<unsigned, const char*> time_range_actions[] = {
      {ID_TIME_RANGE_15M, "15 min"},
      {ID_TIME_RANGE_HOUR, "Hour"},
      {ID_TIME_RANGE_DAY, "Day"},
      {ID_TIME_RANGE_WEEK, "Week"},
      {ID_TIME_RANGE_MONTH, "Month"}};
  for (const auto& [command_id, title] : time_range_actions) {
    ui_command_registry.AddAction(Action{.command_id_ = command_id,
                                         .category_ = CATEGORY_PERIOD,
                                         .title_ = Translate(title),
                                         .flags_ = Action::CHECKABLE});
  }
  ui_command_registry.AddAction(Action{.command_id_ = ID_TIME_RANGE_CUSTOM,
                                       .category_ = CATEGORY_PERIOD,
                                       .title_ = Translate("Custom..."),
                                       .short_title_ = Translate("Custom"),
                                       .flags_ = Action::CHECKABLE});

  const std::pair<unsigned, const char*> interval_actions[] = {
      {ID_INTERVAL_1M, "1-Minute"}, {ID_INTERVAL_5M, "5 min"},
      {ID_INTERVAL_15M, "15 min"},  {ID_INTERVAL_30M, "30 min"},
      {ID_INTERVAL_1H, "1-Hour"},   {ID_INTERVAL_12H, "12 hours"},
      {ID_INTERVAL_1D, "1-Day"}};
  for (const auto& [command_id, title] : interval_actions) {
    ui_command_registry.AddAction(Action{.command_id_ = command_id,
                                         .category_ = CATEGORY_INTERVAL,
                                         .title_ = Translate(title),
                                         .flags_ = Action::CHECKABLE});
  }

  const std::pair<unsigned, const char*> aggregation_actions[] = {
      {ID_AGGREGATION_START, "First"}, {ID_AGGREGATION_END, "Last"},
      {ID_AGGREGATION_COUNT, "Count"}, {ID_AGGREGATION_MIN, "Minimum"},
      {ID_AGGREGATION_MAX, "Maximum"}, {ID_AGGREGATION_SUM, "Sum"},
      {ID_AGGREGATION_AVG, "Average"}};
  for (const auto& [command_id, title] : aggregation_actions) {
    ui_command_registry.AddAction(Action{.command_id_ = command_id,
                                         .category_ = CATEGORY_AGGREGATION,
                                         .title_ = Translate(title),
                                         .flags_ = Action::CHECKABLE});
  }

  ui_command_registry.AddAction(
      Action{.command_id_ = ID_ADD_MULTIPLE_ITEMS,
             .category_ = CATEGORY_CREATE,
             .title_ = Translate("Multiple Create...")});
  ui_command_registry.AddAction(
      Action{.command_id_ = ID_NEW_SERVICE_ITEMS,
             .category_ = CATEGORY_CREATE,
             .title_ = Translate("Service Items...")});
  ui_command_registry.AddAction(
      Action{.command_id_ = ID_NEW_IEC60870_LINK101,
             .category_ = CATEGORY_CREATE,
             .title_ = Translate("IEC 60870-101 Link")});
  ui_command_registry.AddAction(
      Action{.command_id_ = ID_NEW_IEC60870_LINK104,
             .category_ = CATEGORY_CREATE,
             .title_ = Translate("IEC 60870-104 Link")});

  for (size_t i = 0; i < std::size(kNewCommandTypeIds); ++i) {
    ui_command_registry.AddAction(MakeNodeAction(
        ui_command_registry.action_manager(), ID_NEW + i, CATEGORY_CREATE,
        node_service.GetNode(kNewCommandTypeIds[i])));
  }
}

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
      CoSpawn(executor_, cancelation_, [this]() mutable -> Awaitable<void> {
        co_await PasteFromClipboard();
        co_return;
      });
      return;
    case ID_VIEW_CLOSE:
      opened_view_->Close();
      return;
    case ID_EXPORT_CSV:
      assert(dialog_service_);
      if (auto* export_model = controller_->GetExportModel()) {
        CoSpawn(
            executor_, cancelation_,
            [this, export_model,
             window_title =
                 opened_view_->GetWindowTitle()]() mutable -> Awaitable<void> {
              co_await RunCsvExport({executor_, *dialog_service_, profile_,
                                     *export_model, std::move(window_title)});
              co_return;
            });
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
        // `cancelation_` gates the resumption so a destroyed
        // `OpenedViewCommands` never races the dialog completion — the prior
        // `cancelation_.Bind(...)` callback had the same effect.
        CoSpawn(executor_, cancelation_,
                [model, &dialog_service = *dialog_service_, &profile = profile_,
                 range, time_required]() mutable -> Awaitable<void> {
                  auto picked = co_await ShowTimeRangeDialog(
                      dialog_service, {profile, range, time_required});
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

  // `cancelation_` gates the coroutine: if `this` is destroyed before either
  // await resumes, the body returns immediately without touching member state.
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
  // Keep request execution and error reporting in one coroutine body so
  // `title` is only threaded once.
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

Awaitable<void> OpenedViewCommands::OnCreateRecordCompleteAsync(
    scada::NodeId node_id) {
  auto node = node_service_.GetNode(node_id);
  // Hold `node` across the suspension so it doesn't release before the fetch
  // completes — the original callback form held the same NodeRef in its
  // capture list for the same reason.
  co_await FetchNode(node);

  controller_->OnViewNodeCreated(node);
  auto def = co_await MakeWindowDefinitionAsync(
      executor_, &kNodePropertyWindowInfo, node, /*expand_groups=*/false);
  co_await ::OpenView(main_window_, def, true);
  co_return;
}

Awaitable<void> OpenedViewCommands::PasteFromClipboardAsync() {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure))
    throw std::runtime_error{"Configure privilege is required to paste"};

  const auto* selection_model = controller_->GetSelectionModel();
  if (!selection_model)
    throw std::runtime_error{"Selection model is required to paste"};

  const auto& parent_node =
      GetPasteParentNode(node_service_, create_tree_, selection_model->node(),
                         controller_->GetRootNode());
  if (!parent_node)
    throw std::runtime_error{"No valid paste parent is available"};

  co_await PasteNodesFromClipboard(task_manager_, parent_node.node_id());
}

Awaitable<void> OpenedViewCommands::PasteFromClipboard() {
  co_await PasteFromClipboardAsync();
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
        u"Export failed. Please check that Microsoft Excel is installed "
        u"correctly.",
        u"Export", MessageBoxMode::Error);
  }
}
