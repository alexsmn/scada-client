#include "components/main/selection_commands.h"

#include "client_utils.h"
#include "commands/change_password_dialog.h"
#include "commands/write_dialog.h"
#include "common/event_manager.h"
#include "common/node_id_util.h"
#include "common/node_util.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "components/limits/limit_dialog.h"
#include "components/main/main_window_manager.h"
#include "components/main/main_window_util.h"
#include "components/main/opened_view.h"
#include "core/method_service.h"
#include "core/node_management_service.h"
#include "core/session_service.h"
#include "main_window.h"
#include "selection_model.h"
#include "services/dialog_service.h"
#include "services/file_cache.h"
#include "services/task_manager.h"
#include "window_info.h"

namespace {

bool CanCreateSomething(const NodeRef& node) {
  if (node.target(id::Creates))
    return true;

  for (auto type = node.type_definition(); type; type = type.supertype()) {
    if (type.target(id::Creates))
      return true;
  }

  return false;
}

}  // namespace

// SelectionCommands

SelectionCommands::SelectionCommands(SelectionCommandsContext&& context)
    : SelectionCommandsContext{std::move(context)} {}

void SelectionCommands::OpenWindow(unsigned type) {
  if (selection_ && !selection_->empty()) {
    ::OpenView(main_window_,
               MakeWindowDefinition(selection_->node(), type, true), true);
  }
}

CommandHandler* SelectionCommands::GetCommandHandler(unsigned command_id) {
  if (!selection_)
    return nullptr;

  auto node = selection_->node();

  switch (command_id) {
    case ID_ITEM_PARAMS:
      if (!session_service_.HasPrivilege(scada::Privilege::Configure))
        return nullptr;
      return node ? this : nullptr;

    case ID_TABLE_CONFIG: {
      if (!session_service_.HasPrivilege(scada::Privilege::Configure))
        return nullptr;
      return CanCreateSomething(node) ? this : nullptr;
    }

    case ID_OPEN_TABLE:
    case ID_OPEN_GRAPH:
    case ID_OPEN_SUMMARY:
    case ID_OPEN_EVENTS:
      return selection_->multiple() || selection_->GetTimedData().connected() ||
                     (node && node.node_class() == scada::NodeClass::Object)
                 ? this
                 : nullptr;

    case ID_ACKNOWLEDGE_CURRENT:
    case ID_OPEN_DISPLAY:
    case ID_HISTORICAL_EVENTS:
    case ID_TIMED_DATA_VIEW:
    case ID_OPEN_GROUP_TABLE:
      return selection_->GetTimedData().connected() ? this : nullptr;

    case ID_DEV1_REFR:
    case ID_DEV1_SYNC:
    case ID_WRITE:
    case ID_WRITE_MANUAL:
    case ID_UNLOCK_ITEM: {
      if (!session_service_.HasPrivilege(scada::Privilege::Control))
        return nullptr;
      return IsInstanceOf(node, id::DataItemType) ? this : nullptr;
    }

    case ID_EDIT_LIMITS:
      if (!session_service_.HasPrivilege(scada::Privilege::Control))
        return nullptr;
      return IsInstanceOf(node, id::AnalogItemType) ? this : nullptr;

    case ID_ITEM_ENABLE:
    case ID_ITEM_DISABLE: {
      if (!session_service_.HasPrivilege(scada::Privilege::Configure))
        return nullptr;
      return node[id::DeviceType_Disabled] ? this : nullptr;
    }

    case ID_TRANSMISSION_VIEW:
      if (!session_service_.HasPrivilege(scada::Privilege::Configure))
        return nullptr;
      return IsInstanceOf(node, id::Iec60870DeviceType) ? this : nullptr;

    case ID_CHANGE_PASSWORD:
      if (!session_service_.HasPrivilege(scada::Privilege::Configure))
        return nullptr;
      return IsInstanceOf(node, id::UserType) ? this : nullptr;

    case ID_OPEN_WATCH:
    case ID_OPEN_DEVICE_METRICS:
      return IsInstanceOf(node, id::DeviceType) ? this : nullptr;
  }

  return nullptr;
}

bool SelectionCommands::IsCommandEnabled(unsigned command_id) const {
  auto node = selection_->node();

  switch (command_id) {
    case ID_ACKNOWLEDGE_CURRENT:
      return selection_->GetTimedData().alerting();

    case ID_UNLOCK_ITEM:
      return node && node[id::DataItemType_Locked].value().get_or(false);

    case ID_WRITE:
      return node && !node[id::DataItemType_Output].value().is_null();

    case ID_ITEM_ENABLE:
    case ID_ITEM_DISABLE: {
      bool enable = command_id == ID_ITEM_ENABLE;
      return node &&
             node[id::DeviceType_Disabled].value().get_or(false) == enable;
    }

    default:
      return true;
  }
}

bool SelectionCommands::IsCommandChecked(unsigned command_id) const {
  return false;
}

void SelectionCommands::ExecuteCommand(unsigned command_id) {
  assert(dialog_service_);
  assert(selection_);

  if (selection_->multiple()) {
    UINT type;
    if (command_id == ID_OPEN_TABLE)
      type = ID_TABLE_VIEW;
    else if (command_id == ID_OPEN_GRAPH)
      type = ID_GRAPH_VIEW;
    else if (command_id == ID_OPEN_SUMMARY)
      type = ID_SUMMARY_VIEW;
    else if (command_id == ID_OPEN_EVENTS)
      type = ID_EVENT_JOURNAL_VIEW;
    else {
      assert(false);
      return;
    }

    auto title = selection_->GetTitle();
    auto items = selection_->GetMultipleNodeIds();
    ::OpenView(main_window_, MakeWindowDefinition(items, type, title.c_str()));
    return;
  }

  switch (command_id) {
    case ID_CHANGE_PASSWORD: {
      auto node = selection_->node();
      if (IsInstanceOf(node, id::UserType)) {
        ShowChangePasswordDialog(node, node_management_service_, local_events_,
                                 profile_);
      }
      return;
    }
  }

  auto node_id = selection_->node().node_id();
  if (node_id == scada::NodeId() && selection_->GetTimedData().connected()) {
    unsigned type = 0;
    switch (command_id) {
      case ID_OPEN_GRAPH:
        type = ID_GRAPH_VIEW;
        break;
      case ID_OPEN_TABLE:
        type = ID_TABLE_VIEW;
        break;
      case ID_OPEN_SUMMARY:
        type = ID_SUMMARY_VIEW;
        break;
      case ID_TIMED_DATA_VIEW:
        type = command_id;
        break;
    }
    if (type) {
      const std::string& formula = selection_->GetTimedData().formula();
      ::OpenView(main_window_, MakeWindowDefinition(formula.c_str(), type));
    }
    return;
  }

  auto node = selection_->node();

  scada::NodeId call_command_id;

  switch (command_id) {
    case ID_OPEN_GRAPH:
      // TODO: formula
      ::OpenView(main_window_, MakeWindowDefinition(node, ID_GRAPH_VIEW, true));
      return;
    case ID_OPEN_TABLE:
      // TODO: formula
      ::OpenView(main_window_, MakeWindowDefinition(node, ID_TABLE_VIEW, true));
      return;
    case ID_OPEN_SUMMARY:
      // TODO: formula
      ::OpenView(main_window_,
                 MakeWindowDefinition(node, ID_SUMMARY_VIEW, true));
      return;
    case ID_OPEN_EVENTS:
    case ID_HISTORICAL_EVENTS:
      if (IsInstanceOf(node, id::DataGroupType) ||
          IsInstanceOf(node, id::DataItemType)) {
        WindowDefinition win =
            MakeWindowDefinition(node, ID_EVENT_JOURNAL_VIEW, true);
        if (command_id == ID_OPEN_EVENTS)
          win.AddItem("Window").SetString("mode", "Current");
        ::OpenView(main_window_, win, true);
      }
      return;
    case ID_OPEN_DISPLAY:
      OpenModusView(node);
      return;
    case ID_TIMED_DATA_VIEW:
      ::OpenView(main_window_,
                 MakeWindowDefinition(node, ID_TIMED_DATA_VIEW, true));
      return;
    case ID_OPEN_GROUP_TABLE:
      if (auto win = MakeGroupWindowDefinition(node, ID_TABLE_VIEW))
        ::OpenView(main_window_, win.value());
      return;
    case ID_WRITE:
      ExecuteWriteDialog(*dialog_service_, node_id, false, timed_data_service_,
                         profile_);
      return;
    case ID_WRITE_MANUAL:
      ExecuteWriteDialog(*dialog_service_, node_id, true, timed_data_service_,
                         profile_);
      return;
    case ID_UNLOCK_ITEM:
      task_manager_.PostUpdateTask(node_id, {},
                                   {{id::DataItemType_Locked, false}});
      return;
    case ID_EDIT_LIMITS:
      if (IsInstanceOf(node, id::AnalogItemType))
        ShowLimitsDialog(task_manager_, node);
      return;
    case ID_ACKNOWLEDGE_CURRENT:
      event_manager_.AcknowledgeItemEvents(node_id);
      return;
    case ID_ITEM_PARAMS:
      ::OpenView(main_window_,
                 MakeWindowDefinition(node, ID_PROPERTY_VIEW, false));
      return;
    case ID_TABLE_CONFIG:
      ::OpenView(main_window_,
                 MakeWindowDefinition(node, ID_TABLE_EDITOR, false));
      return;
    case ID_TRANSMISSION_VIEW:
      ::OpenView(main_window_,
                 MakeWindowDefinition(node, ID_TRANSMISSION_VIEW, false));
      return;
    case ID_OPEN_WATCH:
      ::OpenView(main_window_,
                 MakeWindowDefinition(node, ID_WATCH_VIEW, false));
      return;
    case ID_OPEN_DEVICE_METRICS:
      if (auto win = MakeDeviceMetricsWindowDefinition(node))
        ::OpenView(main_window_, win.value());
      return;
    case ID_ITEM_ENABLE:
    case ID_ITEM_DISABLE:
      ExecuteDisableItem(task_manager_, node, command_id == ID_ITEM_DISABLE);
      return;

    case ID_DEV1_REFR:
      call_command_id = id::DeviceType_Interrogate;
      break;
    case ID_DEV1_SYNC:
      call_command_id = id::DeviceType_SyncClock;
      break;
  }

  if (!call_command_id.is_null() && IsInstanceOf(node, id::DataItemType)) {
    DoIOCtrl(node.node_id(), call_command_id, {});
    return;
  }

  // Command is supported but not handled.
  assert(false);
}

void SelectionCommands::DoIOCtrl(const scada::NodeId& node_id,
                                 const scada::NodeId& method_id,
                                 const std::vector<scada::Variant>& arguments) {
  method_service_.Call(
      node_id, method_id, arguments, {},
      [node_id, &local_events = local_events_,
       &profile = profile_](const scada::Status& status) {
        // TODO: Fill |title|.
        base::string16 title =
            base::SysNativeMBToWide(NodeIdToScadaString(node_id));
        ReportRequestResult(title, status, local_events, profile);
      });
}

void SelectionCommands::OpenModusView(const NodeRef& node) {
  assert(main_window_);
  assert(dialog_service_);

  auto cached_items = file_cache_.GetList(VIEW_TYPE_MODUS)
                          .GetFilesContainingItem(node.node_id());

  if (cached_items.empty()) {
    base::string16 msg =
        base::StringPrintf(L"Схема для объекта \"%ls\" не найдена.",
                           ToString16(node.display_name()).c_str());
    dialog_service_->RunMessageBox(msg, L"Схема", MessageBoxMode::Info);
    return;
  }

  // TODO: Let user select scheme from list.
  const FileCache::DisplayItem& cached_item = cached_items.front();
  const base::FilePath& path = cached_item.first;

  auto* view = main_window_manager_.FindOpenedViewByFilePath(path);
  if (view) {
    view->Activate();
  } else {
    WindowDefinition win(GetWindowInfo(ID_MODUS_VIEW));
    win.path = path;
    view = main_window_->OpenView(win, true);
  }

  if (view)
    view->SetSelection(node.node_id());
}

void SelectionCommands::SetContext(MainWindow* main_window,
                                   DialogService* dialog_service,
                                   SelectionModel* selection) {
  main_window_ = main_window;
  dialog_service_ = dialog_service;
  selection_ = selection;
}
