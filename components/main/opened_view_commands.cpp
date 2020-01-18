#include "opened_view_commands.h"

#include "base/command_line.h"
#include "base/excel.h"
#include "base/strings/string_util.h"
#include "base/win/win_util2.h"
#include "client_utils.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "common/static_types.h"
#include "common_resources.h"
#include "components/create_service_item/create_service_item_dialog.h"
#include "components/csv_export/csv_export.h"
#include "components/main/actions.h"
#include "components/main/main_window_util.h"
#include "components/main/opened_view.h"
#include "components/main/selection_commands.h"
#include "components/multi_create/multi_create_dialog.h"
#include "components/print_preview/print_preview.h"
#include "components/time_range/time_range_dialog.h"
#include "controller.h"
#include "controller_factory.h"
#include "core/node_management_service.h"
#include "core/session_service.h"
#include "export_model.h"
#include "export_util.h"
#include "model/scada_node_ids.h"
#include "net/transport_string.h"
#include "services/dialog_service.h"
#include "services/print_service.h"
#include "services/profile.h"
#include "services/task_manager.h"
#include "time_model.h"

#if defined(UI_QT)
#include "components/main/qt/main_window_qt.h"

#include <QMenu>
#endif

namespace {

const base::char16 kExportTitle[] = L"Экспорт";

std::filesystem::path MakeFileName(base::StringPiece16 text) {
  base::string16 result;
  base::ReplaceChars(text.as_string(), L":", L"-", &result);
  return result;
}

NodeRef GetCreateParentNode(const NodeRef& suggested_parent,
                            const NodeRef& root,
                            const NodeRef& component_type) {
  if (!component_type)
    return nullptr;

  for (const auto& parent : {suggested_parent, root}) {
    if (parent && CanCreate(parent, component_type))
      return parent;
  }

  return nullptr;
}

TimeRange::Type GetTimeRangeCommand(unsigned command_id) {
  switch (command_id) {
    case ID_TIME_RANGE_DAY:
      return TimeRange::Type::Day;
    case ID_TIME_RANGE_WEEK:
      return TimeRange::Type::Week;
    case ID_TIME_RANGE_MONTH:
      return TimeRange::Type::Month;
    case ID_TIME_RANGE_CUSTOM:
      return TimeRange::Type::Custom;
    default:
      return TimeRange::Type::Count;
  }
}

}  // namespace

OpenedViewCommands::OpenedViewCommands(OpenedViewCommandsContext&& context)
    : OpenedViewCommandsContext{std::move(context)},
      excel_enabled_{
          base::CommandLine::ForCurrentProcess()->HasSwitch("excel")} {
  selection_commands_ =
      std::make_unique<SelectionCommands>(SelectionCommandsContext{
          task_manager_, session_service_, node_management_service_,
          event_manager_, timed_data_service_, local_events_, file_cache_,
          profile_, main_window_manager_, node_service_});
}

OpenedViewCommands::~OpenedViewCommands() {}

void OpenedViewCommands::SetContext(OpenedView* opened_view,
                                    DialogService* dialog_service) {
  opened_view_ = opened_view;
  main_window_ = opened_view ? &opened_view->main_window() : nullptr;
  dialog_service_ = dialog_service;
  controller_ = opened_view ? &opened_view->controller() : nullptr;

  auto* selection =
      opened_view_ ? &opened_view_->controller().selection() : nullptr;
  selection_commands_->SetContext(main_window_, dialog_service_, controller_,
                                  selection);
}

CommandHandler* OpenedViewCommands::GetCommandHandler(unsigned command_id) {
  assert(controller_);

  if (auto* handler = controller_->GetCommandHandler(command_id))
    return handler;

  if (auto* handler = selection_commands_->GetCommandHandler(command_id))
    return handler;

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
#endif
    case ID_EXPORT_CSV:
      return controller_->GetExportModel() ? this : nullptr;

    case ID_NEW_SERVICE_ITEMS:
    case ID_ADD_MULTIPLE_ITEMS:
      return CanCreateRecord(id::DiscreteItemType) ? this : nullptr;

    case ID_NEW_IEC60870_LINK101:
    case ID_NEW_IEC60870_LINK104:
      return CanCreateRecord(id::Iec60870LinkType) ? this : nullptr;

    case ID_TIME_RANGE_DAY:
    case ID_TIME_RANGE_WEEK:
    case ID_TIME_RANGE_MONTH:
    case ID_TIME_RANGE_CUSTOM:
      return controller_->GetTimeModel() != nullptr ? this : nullptr;
  }

  auto node_id = GetNewCommandTypeId(command_id);
  if (!node_id.is_null())
    return CanCreateRecord(node_id) ? this : nullptr;

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
      ExportToCsv();
      return;
    case ID_EXPORT_EXCEL:
      ExportToExcel();
      return;
    case ID_PRINT: {
      PrintService print_service;
      ShowPrintPreviewDialog(*dialog_service_, print_service,
                             [opened_view = opened_view_, &print_service] {
                               opened_view->Print(print_service);
                             });
      return;
    }
    case ID_NEW_IEC60870_LINK101:
      CreateRecord(id::Iec60870LinkType, 0);
      return;
    case ID_NEW_IEC60870_LINK104:
      CreateRecord(id::Iec60870LinkType, 1);
      return;

    case ID_NEW_SERVICE_ITEMS:
      ShowCreateServiceItemDialog(*dialog_service_,
                                  {node_service_, task_manager_,
                                   controller_->selection().node().node_id()});
      return;
    case ID_ADD_MULTIPLE_ITEMS:
      ShowMultiCreateDialog(*dialog_service_,
                            {node_service_, task_manager_,
                             controller_->selection().node().node_id()});
      return;
  }

  if (auto time_range_type = GetTimeRangeCommand(command_id);
      time_range_type != TimeRange::Type::Count) {
    if (auto* model = controller_->GetTimeModel()) {
      if (time_range_type == TimeRange::Type::Custom) {
        auto range = model->GetTimeRange();
        bool time_required = model->IsTimeRequired();
        if (ShowTimeRangeDialog(*dialog_service_,
                                {profile_, range, time_required}))
          model->SetTimeRange(range);

      } else {
        model->SetTimeRange(time_range_type);
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
  if (auto time_range_type = GetTimeRangeCommand(command_id);
      time_range_type != TimeRange::Type::Count) {
    if (auto* model = controller_->GetTimeModel())
      return model->GetTimeRange().type == GetTimeRangeCommand(command_id);
    return false;
  }

  return false;
}

bool OpenedViewCommands::IsCommandEnabled(unsigned command_id) const {
  switch (command_id) {
    case ID_PASTE:
      return session_service_.HasPrivilege(scada::Privilege::Configure) &&
             GetPasteParentNode(node_service_, controller_->selection().node(),
                                controller_->GetRootNode());
    default:
      return true;
  }
}

bool OpenedViewCommands::CanCreateRecord(
    const scada::NodeId& type_node_id) const {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure))
    return false;

  return GetCreateParentNode(controller_->selection().node(),
                             controller_->GetRootNode(),
                             node_service_.GetNode(type_node_id)) != nullptr;
}

void OpenedViewCommands::CreateRecord(const scada::NodeId& type_node_id,
                                      int tag) {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure))
    return;

  auto node_type = node_service_.GetNode(type_node_id);
  if (!node_type)
    return;

  auto parent_node = GetCreateParentNode(controller_->selection().node(),
                                         controller_->GetRootNode(), node_type);
  if (!parent_node)
    return;

  scada::NodeAttributes attributes;
  scada::NodeProperties properties;

  bool is104 = tag != 0;

  // name
  attributes.display_name = node_type.display_name();
  if (type_node_id == id::Iec60870LinkType) {
    attributes.display_name =
        is104 ? L"Направление МЭК-60870-104" : L"Направление МЭК-60870-101";
  }

  // IEC link specific.
  if (type_node_id == id::Iec60870LinkType) {
    // type
    auto protocol =
        is104 ? cfg::Iec60870Protocol::IEC104 : cfg::Iec60870Protocol::IEC101;
    properties.emplace_back(id::Iec60870LinkType_Protocol,
                            static_cast<int>(protocol));

    net::TransportString ts;
    if (is104) {
      ts.SetProtocol(net::TransportString::TCP);
      ts.SetParam(net::TransportString::kParamHost, "localhost");
      ts.SetParam(net::TransportString::kParamPort, 2404);
    } else {
      ts.SetProtocol(net::TransportString::SERIAL);
      ts.SetParam(net::TransportString::kParamName, "COM1");
    }
    ts.SetParam(net::TransportString::kParamActive);
    properties.emplace_back(id::LinkType_Transport, ts.ToString());
  }

  auto dispay_name = attributes.display_name;
  auto weak_ptr = weak_factory_.GetWeakPtr();
  task_manager_.PostInsertTask(
      scada::NodeId(), parent_node.node_id(), type_node_id,
      std::move(attributes), std::move(properties),
      [weak_ptr, dispay_name](const scada::Status& status,
                              const scada::NodeId& node_id) {
        if (auto ptr = weak_ptr.get())
          ptr->OnCreateRecordComplete(dispay_name, status, node_id);
      });
}

void OpenedViewCommands::OnCreateRecordComplete(
    const scada::LocalizedText& display_name,
    const scada::Status& status,
    const scada::NodeId& node_id) {
  base::string16 title =
      base::StringPrintf(L"Создание \"%ls\"", display_name.c_str());
  ReportRequestResult(title, status, local_events_, profile_);

  if (!status)
    return;

  auto weak_ptr = weak_factory_.GetWeakPtr();
  auto node = node_service_.GetNode(node_id);
  node.Fetch(NodeFetchStatus::NodeOnly(),
             [weak_ptr, node](const NodeRef& node) {
               if (auto* ptr = weak_ptr.get()) {
                 ptr->controller_->OnViewNodeCreated(node);
                 auto def = MakeWindowDefinition(node, ID_PROPERTY_VIEW, false);
                 ::OpenView(ptr->main_window_, def, true);
               }
             });
}

void OpenedViewCommands::PasteFromClipboard() {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure))
    return;

  const auto& parent_node =
      GetPasteParentNode(node_service_, controller_->selection().node(),
                         controller_->GetRootNode());
  if (!parent_node)
    return;

  if (!PasteNodesFromClipboard(task_manager_, parent_node.node_id()))
    LOG(ERROR) << "Paste records error";
}

void OpenedViewCommands::ExportToCsv() {
  auto* export_model = controller_->GetExportModel();
  if (!export_model)
    return;

  const base::StringPiece kCsvExt[] = {"*.csv"};
  const DialogService::Filter kFilters[] = {
      {L"Файлы CSV", kCsvExt},
  };

  auto file_name = MakeFileName(opened_view_->GetWindowTitle());
  file_name += ".csv";

  DialogService::SaveParams save_params;
  save_params.title = kExportTitle;
  save_params.default_path = profile_.csv_export_dir / file_name;
  save_params.filters = kFilters;
  auto path = dialog_service_->SelectSaveFile(save_params);
  if (path.empty())
    return;

  profile_.csv_export_dir = path.parent_path();

  CsvExportParams& params = profile_.csv_export_params;
  if (!ShowCsvExportDialog(*dialog_service_, params))
    return;

  auto export_data = export_model->GetExportData();

  try {
    std::visit([&](auto& data) { ::ExportToCsv(data, params, path); },
               export_data);

  } catch (const std::runtime_error&) {
    dialog_service_->RunMessageBox(L"Ошибка при экспорте.", kExportTitle,
                                   MessageBoxMode::Error);
    return;
  }

  if (dialog_service_->RunMessageBox(
          L"Экспорт завершен. Открыть файл сейчас?", kExportTitle,
          MessageBoxMode::QuestionYesNo) == MessageBoxResult::Yes)
    win_util::OpenWithAssociatedProgram(path);
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
        L"Не удалось выполнить экспорт. Проверьте корректность установки "
        L"Microsoft Excel.",
        L"Экспорт", MessageBoxMode::Error);
  }
}
