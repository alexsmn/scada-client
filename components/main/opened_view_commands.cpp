#include "opened_view_commands.h"

#include "base/command_line.h"
#include "base/excel.h"
#include "base/string_piece_util.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/win/win_util2.h"
#include "client_utils.h"
#include "clipboard/clipboard_util.h"
#include "common_resources.h"
#include "components/create_service_item/create_service_item_dialog.h"
#include "components/csv_export/csv_export.h"
#include "components/main/actions.h"
#include "components/main/main_window_util.h"
#include "components/main/opened_view.h"
#include "components/main/selection_commands.h"
#include "components/multi_create/multi_create_dialog.h"
#include "components/node_properties/node_property_component.h"
#include "components/print_preview/print_preview.h"
#include "components/time_range/time_range_dialog.h"
#include "controller/controller.h"
#include "controller/controller_registry.h"
#include "export_model.h"
#include "export_util.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/static_types.h"
#include "net/transport_string.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "scada/node_management_service.h"
#include "scada/session_service.h"
#include "scada/status_promise.h"
#include "controller/selection_model.h"
#include "services/create_tree.h"
#include "services/dialog_service.h"
#include "services/print_service.h"
#include "services/profile.h"
#include "services/task_manager.h"
#include "time_model.h"
#include "window_definition_builder.h"

#if defined(UI_QT)
#include "components/main/qt/main_window_qt.h"
#endif

#if defined(UI_QT)
#include <QMenu>
#endif

namespace {

const char16_t kExportTitle[] = u"Экспорт";

std::filesystem::path MakeFileName(std::u16string_view text) {
  std::u16string result;
  base::ReplaceChars(AsStringPiece(text), u":", u"-", &result);
  return result;
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
          base::CommandLine::ForCurrentProcess()->HasSwitch("excel")} {}

OpenedViewCommands::~OpenedViewCommands() {}

void OpenedViewCommands::SetContext(OpenedView* opened_view,
                                    DialogService* dialog_service) {
  opened_view_ = opened_view;
  main_window_ = opened_view ? &opened_view->main_window() : nullptr;
  dialog_service_ = dialog_service;
  controller_ = opened_view ? &opened_view->controller() : nullptr;
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
      return CanCreateRecord(data_items::id::DiscreteItemType) ? this : nullptr;

    case ID_NEW_IEC60870_LINK101:
    case ID_NEW_IEC60870_LINK104:
      return CanCreateRecord(devices::id::Iec60870LinkType) ? this : nullptr;

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
      static PrintService print_service;
      ShowPrintPreviewDialog(
          *dialog_service_, print_service,
          [opened_view = opened_view_, &print_service = print_service] {
            opened_view->Print(print_service);
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

  if (auto time_range_type = GetTimeRangeCommand(command_id);
      time_range_type != TimeRange::Type::Count) {
    if (auto* model = controller_->GetTimeModel()) {
      if (time_range_type == TimeRange::Type::Custom) {
        auto range = model->GetTimeRange();
        bool time_required = model->IsTimeRequired();
        ShowTimeRangeDialog(*dialog_service_, {profile_, range, time_required})
            .then([model, weak_ptr = weak_factory_.GetWeakPtr()](
                      const TimeRange& time_range) {
              if (weak_ptr.get()) {
                model->SetTimeRange(time_range);
              }
            });
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

promise<> OpenedViewCommands::CreateRecord(const scada::NodeId& type_node_id,
                                           int tag) {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure))
    return MakeRejectedPromise();

  auto node_type = node_service_.GetNode(type_node_id);
  if (!node_type)
    return MakeRejectedPromise();

  auto* selection_model = controller_->GetSelectionModel();
  if (!selection_model)
    return MakeRejectedPromise();

  auto parent_node = create_tree_.GetCreateParentNode(
      selection_model->node(), controller_->GetRootNode(), node_type);
  if (!parent_node)
    return MakeRejectedPromise();

  scada::NodeAttributes attributes;
  scada::NodeProperties properties;

  bool is104 = tag != 0;

  // name
  attributes.display_name = node_type.display_name();
  if (type_node_id == devices::id::Iec60870LinkType) {
    attributes.display_name =
        is104 ? u"Направление МЭК-60870-104" : u"Направление МЭК-60870-101";
  }

  // IEC link specific.
  if (type_node_id == devices::id::Iec60870LinkType) {
    // type
    auto protocol =
        is104 ? cfg::Iec60870Protocol::IEC104 : cfg::Iec60870Protocol::IEC101;
    properties.emplace_back(devices::id::Iec60870LinkType_Protocol,
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
    properties.emplace_back(devices::id::LinkType_Transport, ts.ToString());
  }

  // `std::format` doesn't support `u16string`s.
  auto title =
      base::StringPrintf(u"Создание \"%ls\"", attributes.display_name.c_str());

  auto insert_promise = task_manager_.PostInsertTask(
      scada::NodeId(), parent_node.node_id(), type_node_id,
      std::move(attributes), std::move(properties), {});

  ToVoidPromise(insert_promise)
      .except([this, weak_ptr = weak_factory_.GetWeakPtr(),
               title](std::exception_ptr e) {
        if (weak_ptr.get()) {
          ReportRequestResult(title, scada::GetExceptionStatus(e),
                              local_events_, profile_);
        }
      });

  return insert_promise.then([this, weak_ptr = weak_factory_.GetWeakPtr(),
                              title](const scada::NodeId& node_id) {
    if (!weak_ptr.get()) {
      return MakeRejectedPromise();
    }
    ReportRequestResult(title, scada::StatusCode::Good, local_events_,
                        profile_);
    return OnCreateRecordComplete(node_id);
  });
}

promise<> OpenedViewCommands::OnCreateRecordComplete(
    const scada::NodeId& node_id) {
  auto node = node_service_.GetNode(node_id);
  promise<> promise;
  // Capture node so it doesn't release before completion.
  node.Fetch(NodeFetchStatus::NodeOnly(),
             [this, weak_ptr = weak_factory_.GetWeakPtr(), node,
              promise](const NodeRef& node) mutable {
               if (!weak_ptr.get()) {
                 promise.reject(std::exception{});
                 return;
               }
               controller_->OnViewNodeCreated(node);
               auto def =
                   MakeWindowDefinition(&kNodePropertyWindowInfo, node, false);
               ::OpenView(main_window_, def, true);
               promise.resolve();
             });
  return promise;
}

promise<> OpenedViewCommands::PasteFromClipboard() {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure))
    return MakeRejectedPromise();

  auto* selection_model = controller_->GetSelectionModel();
  if (!selection_model)
    return MakeRejectedPromise();

  const auto& parent_node =
      GetPasteParentNode(node_service_, create_tree_, selection_model->node(),
                         controller_->GetRootNode());
  if (!parent_node)
    return MakeRejectedPromise();

  return PasteNodesFromClipboard(task_manager_, parent_node.node_id());
}

void OpenedViewCommands::ExportToCsv() {
  auto* export_model = controller_->GetExportModel();
  if (!export_model)
    return;

  const std::string_view kCsvExt[] = {"*.csv"};
  const DialogService::Filter kFilters[] = {
      {u"Файлы CSV", kCsvExt},
  };

  auto file_name = MakeFileName(opened_view_->GetWindowTitle());
  file_name += ".csv";

  dialog_service_
      ->SelectSaveFile({.title = kExportTitle,
                        .default_path = profile_.csv_export_dir / file_name,
                        .filters = kFilters})
      .then([this, export_model, weak_ptr = weak_factory_.GetWeakPtr()](
                const std::filesystem::path& path) {
        if (!weak_ptr.get())
          return;

        profile_.csv_export_dir = path.parent_path();

        ShowCsvExportDialog(*dialog_service_, profile_)
            .then([this, export_model, path](const CsvExportParams& params) {
              auto export_data = export_model->GetExportData();

              try {
                std::visit(
                    [&](auto& data) { ::ExportToCsv(data, params, path); },
                    export_data);

              } catch (const std::runtime_error&) {
                dialog_service_->RunMessageBox(u"Ошибка при экспорте.",
                                               kExportTitle,
                                               MessageBoxMode::Error);
                return;
              }

              dialog_service_
                  ->RunMessageBox(u"Экспорт завершен. Открыть файл сейчас?",
                                  kExportTitle, MessageBoxMode::QuestionYesNo)
                  .then(BindExecutor(
                      executor_, [path = std::move(path)](
                                     MessageBoxResult message_box_result) {
                        if (message_box_result == MessageBoxResult::Yes)
                          win_util::OpenWithAssociatedProgram(path);
                      }));
            });
      });
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
        u"Не удалось выполнить экспорт. Проверьте корректность установки "
        u"Microsoft Excel.",
        u"Экспорт", MessageBoxMode::Error);
  }
}
