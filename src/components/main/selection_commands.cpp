#include "components/main/selection_commands.h"

#include "client_utils.h"
#include "commands/change_password_dialog.h"
#include "commands/write_dialog.h"
#include "common/event_manager.h"
#include "common/node_ref_util.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "components/limits/limit_dialog.h"
#include "components/main/main_window.h"
#include "components/main/main_window_util.h"
#include "components/main/opened_view.h"
#include "core/node_management_service.h"
#include "core/session_service.h"
#include "selection_model.h"
#include "services/file_cache.h"
#include "services/task_manager.h"
#include "window_info.h"

// SelectionCommands

SelectionCommands::SelectionCommands(SelectionCommandsContext&& context)
    : SelectionCommandsContext(std::move(context)) {}

void SelectionCommands::OpenWindow(unsigned type) {
  if (!selection_ || selection_->empty())
    return;

  // TODO: Formula.
  auto node = selection_->node();
  if (!node)
    return;

  auto weak_main_window = main_window_->GetWeakPtr();
  PrepareWindowDefinitionForOpenExpandGroups(
      view_service_, node_service_, node, type,
      [weak_main_window](WindowDefinition win) {
        ::OpenView(weak_main_window.get(), std::move(win));
      });
}

CommandHandler* SelectionCommands::GetCommandHandler(unsigned command_id) {
  if (!selection_)
    return nullptr;

  const auto node = selection_->node();

  switch (command_id) {
    case ID_ITEM_PARAMS:
      if (!session_service_.IsAdministrator())
        return nullptr;
      return node ? this : nullptr;

    case ID_TABLE_CONFIG: {
      if (!session_service_.IsAdministrator())
        return nullptr;
      return !node.type_definition().components().empty() ? this : nullptr;
    }

    case ID_OPEN_TABLE:
    case ID_OPEN_GRAPH:
    case ID_OPEN_SUMMARY:
    case ID_OPEN_EVENTS:
      return selection_->multiple() || selection_->GetTimedData().connected() ||
                     node && (node.node_class() == scada::NodeClass::Object)
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
      if (!session_service_.IsAdministrator())
        return nullptr;
      return IsInstanceOf(node, id::DataItemType) ? this : nullptr;
    }

    case ID_EDIT_LIMITS:
      if (!session_service_.IsAdministrator())
        return nullptr;
      return IsInstanceOf(node, id::AnalogItemType) ? this : nullptr;

    case ID_ITEM_ENABLE:
    case ID_ITEM_DISABLE: {
      if (!session_service_.IsAdministrator())
        return nullptr;
      return node.type_definition()[id::DeviceType_Disabled] ? this : nullptr;
    }

    case ID_TRANSMISSION_VIEW:
      if (!session_service_.IsAdministrator())
        return nullptr;
      return IsInstanceOf(node, id::Iec60870DeviceType) ? this : nullptr;

    case ID_CHANGE_PASSWORD:
      if (!session_service_.IsAdministrator())
        return nullptr;
      return IsInstanceOf(node, id::UserType) ? this : nullptr;

    case ID_OPEN_WATCH:
    case ID_OPEN_DEVICE_METRICS:
      return IsInstanceOf(node, id::DeviceType) ? this : nullptr;
  }

  return nullptr;
}

bool SelectionCommands::IsCommandEnabled(unsigned command_id) const {
  const auto& node = selection_->node();

  switch (command_id) {
    case ID_ACKNOWLEDGE_CURRENT:
      return selection_->GetTimedData().alerting();

    case ID_UNLOCK_ITEM:
      return node[id::DataItemType_Locked].value().get_or(false);

    case ID_WRITE:
      return !node[id::DataItemType_Output].value().is_null();

    case ID_ITEM_ENABLE:
    case ID_ITEM_DISABLE: {
      bool enable = command_id == ID_ITEM_ENABLE;
      auto prop = node[id::DeviceType_Disabled];
      return prop && prop.value().get_or(false) == enable;
    }

    default:
      return true;
  }
}

bool SelectionCommands::IsCommandChecked(unsigned command_id) const {
  return false;
}

void SelectionCommands::ExecuteCommand(unsigned command_id) {
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
    OpenView(main_window_,
             PrepareWindowDefinitionForOpen(items, type, title.c_str()));
    return;
  }

  switch (command_id) {
    case ID_CHANGE_PASSWORD: {
      const auto& node = selection_->node();
      if (IsInstanceOf(node, id::UserType))
        ShowChangePasswordDialog(node, local_events_, profile_,
                                 node_management_service_);
      return;
    }
  }

  const auto& node = selection_->node();

  auto node_id = node.id();
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
      OpenView(main_window_,
               PrepareWindowDefinitionForOpen(formula.c_str(), type));
    }
    return;
  }

  scada::NodeId call_command_id;

  switch (command_id) {
    case ID_OPEN_GRAPH:
      OpenWindow(ID_GRAPH_VIEW);
      return;
    case ID_OPEN_TABLE:
      OpenWindow(ID_TABLE_VIEW);
      return;
    case ID_OPEN_SUMMARY:
      OpenWindow(ID_SUMMARY_VIEW);
      return;
    case ID_TIMED_DATA_VIEW:
      OpenWindow(ID_TIMED_DATA_VIEW);
      return;
    case ID_OPEN_EVENTS:
    case ID_HISTORICAL_EVENTS: {
      auto weak_main_window = main_window_->GetWeakPtr();
      PrepareWindowDefinitionForOpenExpandGroups(
          view_service_, node_service_, node, ID_EVENT_JOURNAL_VIEW,
          [command_id, weak_main_window](WindowDefinition win) {
            if (command_id == ID_OPEN_EVENTS)
              win.AddItem("Window").SetString("mode", "Current");
            OpenView(weak_main_window.get(), std::move(win));
          });
      return;
    }
    case ID_OPEN_DISPLAY:
      OpenModusView(node_id);
      return;
    case ID_OPEN_GROUP_TABLE: {
      auto weak_main_window = main_window_->GetWeakPtr();
      PrepareWindowDefinitionForGroup(
          view_service_, node_service_, node, ID_TABLE_VIEW,
          [weak_main_window](WindowDefinition win) {
            OpenView(weak_main_window.get(), std::move(win));
          });
      return;
    }
    case ID_WRITE:
      ExecuteWriteDialog(main_window_, timed_data_service_, node, false,
                         profile_);
      return;
    case ID_WRITE_MANUAL:
      ExecuteWriteDialog(main_window_, timed_data_service_, node, true,
                         profile_);
      return;
    case ID_UNLOCK_ITEM: {
      auto& local_events = local_events_;
      auto& profile = profile_;
      task_manager_.PostUpdateTask(
          node_id, {}, {{id::DataItemType_Locked, false}},
          [&local_events, &profile](const scada::Status& status) {
            ReportRequestResult(L"Снять блокировку", status, local_events,
                                profile);
          });
      return;
    }
    case ID_EDIT_LIMITS:
      if (IsInstanceOf(node, id::AnalogItemType))
        ShowLimitsDialog(dialog_service_, node, task_manager_);
      return;
    case ID_ACKNOWLEDGE_CURRENT:
      event_manager_.AcknowledgeItemEvents(node_id);
      return;
    case ID_ITEM_PARAMS:
      OpenView(main_window_,
               PrepareWindowDefinitionForOpen(node, ID_PROPERTY_VIEW));
      return;
    case ID_TABLE_CONFIG:
      OpenView(main_window_,
               PrepareWindowDefinitionForOpen(node, ID_TABLE_EDITOR));
      return;
    case ID_TRANSMISSION_VIEW:
      OpenView(main_window_,
               PrepareWindowDefinitionForOpen(node, ID_TRANSMISSION_VIEW));
      return;
    case ID_OPEN_WATCH:
      OpenView(main_window_,
               PrepareWindowDefinitionForOpen(node, ID_WATCH_VIEW));
      return;
    case ID_OPEN_DEVICE_METRICS:
      if (node) {
        auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
        PrepareDeviceMetricsView(view_service_, node_service_, node,
                                 [weak_ptr](WindowDefinition win) {
                                   if (auto* ptr = weak_ptr.get())
                                     OpenView(ptr->main_window_,
                                              std::move(win));
                                 });
      }
      return;
    case ID_ITEM_ENABLE:
    case ID_ITEM_DISABLE:
      ExecuteDisableItem(node, command_id == ID_ITEM_DISABLE, task_manager_);
      return;

    case ID_DEV1_REFR:
      call_command_id = id::DeviceType_Interrogate;
      break;
    case ID_DEV1_SYNC:
      call_command_id = id::DeviceType_SyncClock;
      break;
  }

  if (!call_command_id.is_null() && IsInstanceOf(node, id::DataItemType)) {
    DoIOCtrl(node.id(), call_command_id, std::vector<scada::Variant>(),
             local_events_, profile_, method_service_);
    return;
  }

  // Command is supported but not handled.
  assert(false);
}

void SelectionCommands::OpenModusView(const scada::NodeId& item_id) {
  auto cached_items =
      file_cache_.GetList(VIEW_TYPE_MODUS).GetFilesContainingItem(item_id);

  if (cached_items.empty()) {
    ShowMessageBox(dialog_service_, L"Схема для объекта не найдена.", L"Схема",
                   MB_ICONEXCLAMATION);
    return;
  }

  // TODO: Let user select scheme from list.
  const FileCache::DisplayItem& cached_item = cached_items.front();
  const base::FilePath& path = cached_item.first;

  auto* view = find_opened_view_(path);
  if (view) {
    view->Activate();
  } else if (main_window_) {
    WindowDefinition win(GetWindowInfo(ID_MODUS_VIEW));
    win.path = path;
    view = main_window_->OpenView(win, true);
  }

  if (view)
    view->SetSelection(item_id);
}
