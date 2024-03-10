#include "main_window/selection_commands.h"

#include "aui/dialog_service.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/promise_executor.h"
#include "base/strings/stringprintf.h"
#include "client_utils.h"
#include "clipboard/clipboard_util.h"
#include "common_resources.h"
#include "components/change_password/change_password_dialog.h"
#include "components/device_metrics/device_metrics_command.h"
#include "components/events/events_component.h"
#include "components/node_properties/node_property_component.h"
#include "components/node_table/node_table_component.h"
#include "components/summary/summary_component.h"
#include "components/table/table_component.h"
#include "components/timed_data/timed_data_component.h"
#include "components/transmission/transmission_component.h"
#include "components/watch/watch_component.h"
#include "controller/selection_model.h"
#include "controller/window_info.h"
#include "core/selection_command_context.h"
#include "events/node_event_provider.h"
#include "filesystem/file_cache.h"
#include "main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/main_window_util.h"
#include "main_window/opened_view.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/filesystem_node_ids.h"
#include "model/scada_node_ids.h"
#include "model/security_node_ids.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "profile/window_definition_util.h"
#include "scada/node_management_service.h"
#include "scada/session_service.h"
#include "window_definition_builder.h"

#if !defined(UI_WT)
#include "graph/graph_component.h"
#endif

namespace {

bool CanCreateSomething(const NodeRef& node) {
  if (node.target(scada::id::Creates))
    return true;

  for (auto type = node.type_definition(); type; type = type.supertype()) {
    if (type.target(scada::id::Creates))
      return true;
  }

  return false;
}

void RegisterOpenViewCommand(SelectionCommands& selection_commands,
                             CommandRegistry& command_registry,
                             unsigned command_id,
                             const WindowInfo& window_info) {
  command_registry.AddCommand(
      Command{command_id}
          .set_execute_handler([&selection_commands, &window_info] {
            ::OpenView(
                selection_commands.main_window(),
                selection_commands.GetOpenWindowDefinition(&window_info));
          })
          .set_available_handler([&selection_commands] {
            return !selection_commands.selection()->empty();
          }));
}

}  // namespace

// SelectionCommands

SelectionCommands::SelectionCommands(SelectionCommandsContext&& context)
    : SelectionCommandsContext{std::move(context)} {
  // |selection_| and |dialog_service_| are never null in command handlers.

  RegisterOpenViewCommand(*this, command_registry_, ID_OPEN_TABLE,
                          kTableWindowInfo);
#if !defined(UI_WT)
  RegisterOpenViewCommand(*this, command_registry_, ID_OPEN_GRAPH,
                          kGraphWindowInfo);
#endif
  RegisterOpenViewCommand(*this, command_registry_, ID_OPEN_SUMMARY,
                          kSummaryWindowInfo);

  command_registry_.AddCommand(
      Command{ID_OPEN_DEVICE_METRICS}
          .set_execute_handler([this] {
            MakeDeviceMetricsWindowDefinition(selection_->node())
                .then([weak_ptr = weak_ptr_factory_.GetWeakPtr()](
                          const WindowDefinition& window_definition) {
                  if (auto* ptr = weak_ptr.get())
                    ptr->OpenWindow(window_definition);
                });
          })
          .set_available_handler([this] {
            return IsInstanceOf(selection()->node(), devices::id::DeviceType);
          }));

  // TODO: Move to the event module.
  command_registry_.AddCommand(
      Command{ID_OPEN_EVENTS}
          .set_execute_handler([this] {
            ::OpenView(main_window_,
                       GetOpenWindowDefinition(&kEventJournalWindowInfo)
                           .then([](const WindowDefinition& window_def) {
                             auto new_window_def = window_def;
                             new_window_def.AddItem("Window").SetString(
                                 "mode", "Current");
                             return new_window_def;
                           }),
                       /*activate=*/true);
          })
          .set_available_handler([this] { return !selection_->empty(); }));

  // TODO: Move to the event module.
  command_registry_.AddCommand(
      Command{ID_HISTORICAL_EVENTS}
          .set_execute_handler([this] {
            ::OpenView(main_window_,
                       GetOpenWindowDefinition(&kEventJournalWindowInfo), true);
          })
          .set_available_handler([this] { return !selection_->empty(); }));

#if !defined(UI_WT)
  command_registry_.AddCommand(
      Command{ID_OPEN_DISPLAY}
          .set_execute_handler([this] {
            OpenViewContainingNode(ID_MODUS_VIEW, selection_->node());
          })
          .set_available_handler(
              [this] { return selection_->timed_data().connected(); }));
#endif

  command_registry_.AddCommand(
      Command{ID_TIMED_DATA_VIEW}
          .set_execute_handler([this] {
            ::OpenView(main_window_,
                       GetOpenWindowDefinition(&kTimedDataWindowInfo));
          })
          .set_available_handler(
              [this] { return selection_->timed_data().connected(); }));

  command_registry_.AddCommand(
      Command{ID_OPEN_GROUP_TABLE}
          .set_execute_handler([this] {
            // TODO: Capture |main_window_| by weak pointer.
            MakeGroupWindowDefinition(&kTableWindowInfo, selection_->node())
                .then([main_window = main_window_](
                          const std::optional<WindowDefinition>& window_def) {
                  if (window_def.has_value())
                    ::OpenView(main_window, *window_def);
                });
          })
          .set_available_handler(
              [this] { return selection_->timed_data().connected(); }));

  command_registry_.AddCommand(
      Command{ID_ITEM_PARAMS}
          .set_execute_handler([this] {
            ::OpenView(main_window_,
                       MakeSingleWindowDefinition(&kNodePropertyWindowInfo,
                                                  selection_->node()));
          })
          .set_available_handler([this] {
            return session_service_.HasPrivilege(scada::Privilege::Configure) &&
                   selection_->node();
          }));

  command_registry_.AddCommand(
      Command{ID_TABLE_CONFIG}
          .set_execute_handler([this] {
            ::OpenView(main_window_,
                       MakeSingleWindowDefinition(&kTableEditorWindowInfo,
                                                  selection_->node()));
          })
          .set_available_handler([this] {
            return session_service_.HasPrivilege(scada::Privilege::Configure) &&
                   CanCreateSomething(selection_->node());
          }));

  command_registry_.AddCommand(
      Command{ID_OPEN_WATCH}
          .set_execute_handler([this] {
            ::OpenView(main_window_,
                       MakeSingleWindowDefinition(&kWatchWindowInfo,
                                                  selection_->node()));
          })
          .set_available_handler([this] {
            return IsInstanceOf(selection_->node(), devices::id::DeviceType);
          }));

  command_registry_.AddCommand(
      Command{ID_CHANGE_PASSWORD}
          .set_execute_handler([this] {
            ShowChangePasswordDialog(
                *dialog_service_,
                ChangePasswordContext{selection_->node(), local_events_,
                                      profile_});
          })
          .set_available_handler([this] {
            return session_service_.HasPrivilege(scada::Privilege::Configure) &&
                   IsInstanceOf(selection_->node(), security::id::UserType);
          }));

  // ID_TRANSMISSION_VIEW
  command_registry_.AddCommand(
      Command{ID_TRANSMISSION_VIEW}
          .set_execute_handler([this] {
            ::OpenView(main_window_,
                       MakeSingleWindowDefinition(&kTransmissionWindowInfo,
                                                  selection_->node()));
          })
          .set_available_handler([this] {
            return session_service_.HasPrivilege(scada::Privilege::Configure) &&
                   IsInstanceOf(selection_->node(), devices::id::DeviceType) &&
                   !IsInstanceOf(selection_->node(), devices::id::LinkType);
          }));

  command_registry_.AddCommand(
      Command{ID_COPY}
          .set_execute_handler([this] { CopyToClipboard(); })
          .set_enabled_handler([this] { return !selection_->empty(); })
          .set_available_handler([this] {
            return session_service_.HasPrivilege(scada::Privilege::Configure) &&
                   !selection_->empty();
          }));

  command_registry_.AddCommand(
      Command{ID_DELETE}
          .set_execute_handler([this] { DeleteSelection(); })
          .set_enabled_handler([this] { return !selection_->empty(); })
          .set_available_handler([this] {
            return session_service_.HasPrivilege(scada::Privilege::Configure) &&
                   !selection_->empty();
          }));

  // TODO: Move to the event module.
  command_registry_.AddCommand(
      Command{ID_ACKNOWLEDGE_CURRENT}
          .set_execute_handler([this] {
            node_event_provider_.AcknowledgeItemEvents(
                selection_->node().node_id());
          })
          .set_enabled_handler(
              [this] { return selection_->timed_data().alerting(); })
          .set_available_handler(
              [this] { return selection_->timed_data().connected(); }));
}

void SelectionCommands::OpenWindow(const WindowInfo* window_info) {
  if (selection_ && !selection_->empty()) {
    // TODO: Capture |main_window_| by weak pointer.
    MakeWindowDefinition(window_info, selection_->node(), true)
        .then([weak_ptr = weak_ptr_factory_.GetWeakPtr()](
                  const WindowDefinition& window_definition) {
          if (auto ptr = weak_ptr.get())
            ptr->OpenWindow(window_definition);
        });
  }
}

void SelectionCommands::OpenWindow(const WindowDefinition& window_definition) {
  ::OpenView(main_window_, window_definition, true);
}

CommandHandler* SelectionCommands::GetCommandHandler(unsigned command_id) {
  if (!selection_ || !dialog_service_) {
    return nullptr;
  }

  if (auto* handler = command_registry_.GetCommandHandler(command_id)) {
    return handler;
  }

  if (const auto* command = selection_commands_.FindCommand(command_id)) {
    if (!command->available_handler ||
        command->available_handler(command_context())) {
      return this;
    }
  }

  return nullptr;
}

promise<OpenedView*> SelectionCommands::OpenViewContainingNode(
    int view_type_id,
    const NodeRef& node) {
  assert(main_window_);
  assert(dialog_service_);

  auto cached_items =
      file_cache_.GetList(view_type_id).GetFilesContainingItem(node.node_id());

  if (cached_items.empty()) {
    auto msg = base::StringPrintf(u"Схема для объекта \"%ls\" не найдена.",
                                  ToString16(node.display_name()).c_str());
    return ToRejectedPromise<OpenedView*>(
        dialog_service_->RunMessageBox(msg, u"Схема", MessageBoxMode::Info));
  }

  // TODO: Let user select scheme from list.
  const FileCache::DisplayItem& cached_item = cached_items.front();
  const std::filesystem::path& path = cached_item.first;

  scada::status_promise<OpenedView*> open_promise;

  if (auto* view = main_window_manager_.FindOpenedViewByFilePath(path); view) {
    view->Activate();
    open_promise = make_resolved_promise(view);
  } else {
    WindowDefinition win(GetWindowInfo(view_type_id));
    win.path = path;
    open_promise = main_window_->OpenView(win, true);
  }

  return open_promise.then([node_id = node.node_id()](OpenedView* opened_view) {
    opened_view->SetSelection(node_id);
    return opened_view;
  });
}

void SelectionCommands::SetContext(MainWindow* main_window,
                                   DialogService* dialog_service,
                                   Controller* controller,
                                   SelectionModel* selection) {
  main_window_ = main_window;
  dialog_service_ = dialog_service;
  controller_ = controller;
  selection_ = selection;
}

void SelectionCommands::DeleteSelection() {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure))
    return;

  std::vector<NodeRef> nodes;

  if (selection_->multiple()) {
    auto node_ids = selection_->GetMultipleNodeIds();
    nodes.reserve(node_ids.size());
    std::ranges::transform(
        node_ids, std::back_inserter(nodes),
        [&node_service = node_service_](const scada::NodeId& node_id) {
          return node_service.GetNode(node_id);
        });
  } else if (auto node = selection_->node()) {
    nodes.emplace_back(std::move(node));
  }

  if (nodes.empty())
    return;

  auto message =
      nodes.size() == 1
          ? base::StringPrintf(u"Вы действительно хотите удалить %ls?",
                               nodes.front().display_name().c_str())
          : base::StringPrintf(u"Вы действительно хотите удалить %Iu объектов?",
                               nodes.size());

  dialog_service_
      ->RunMessageBox(message, u"Удаление", MessageBoxMode::QuestionYesNo)
      .then(BindPromiseExecutor(
          executor_, [&task_manager = task_manager_,
                      nodes = std::move(nodes)](MessageBoxResult result) {
            if (result != MessageBoxResult::Yes)
              return;
            for (auto& node : nodes)
              DeleteTreeRecordsRecursive(task_manager, node);
          }));
}

void SelectionCommands::CopyToClipboard() {
  std::vector<NodeRef> nodes;

  if (selection_->multiple()) {
    for (const auto& node_id : selection_->GetMultipleNodeIds()) {
      const auto& node = node_service_.GetNode(node_id);
      nodes.emplace_back(node);
      GetNodesRecursive(node, nodes);
    }

  } else if (const auto& node = selection_->node()) {
    nodes.emplace_back(node);
  }

  if (!nodes.empty())
    CopyNodesToClipboard(nodes);
}

promise<WindowDefinition> SelectionCommands::GetOpenWindowDefinition(
    const WindowInfo* window_info) const {
  if (auto open_context = controller_->GetOpenContext();
      open_context.has_value()) {
    return MakeWindowDefinition(window_info, open_context.value());
  }

  if (selection_->multiple()) {
    auto title = selection_->GetTitle();
    auto node_ids = selection_->GetMultipleNodeIds();
    return make_resolved_promise(MakeWindowDefinition(
        window_info, {node_ids.begin(), node_ids.end()}, title));
  }

  const auto& node_id = selection_->node().node_id();
  if (node_id.is_null() && selection_->timed_data().connected()) {
    const std::string& formula = selection_->timed_data().formula();
    return make_resolved_promise(MakeWindowDefinition(window_info, formula));
  }

  return MakeWindowDefinition(window_info, selection_->node(), true);
}

bool SelectionCommands::IsCommandEnabled(unsigned command_id) const {
  const auto* command = selection_commands_.FindCommand(command_id);
  return command && (!command->enabled_handler ||
                     command->enabled_handler(command_context()));
}

bool SelectionCommands::IsCommandChecked(unsigned command_id) const {
  const auto* command = selection_commands_.FindCommand(command_id);
  return command && command->checked_handler &&
         command->checked_handler(command_context());
}

void SelectionCommands::ExecuteCommand(unsigned command_id) {
  if (const auto* command = selection_commands_.FindCommand(command_id)) {
    if (command->execute_handler) {
      command->execute_handler(command_context());
    }
  }
}

SelectionCommandContext SelectionCommands::command_context() const {
  // |selection_| and |dialog_service_| are never null in command handlers.
  assert(selection_);
  assert(dialog_service_);

  return {.selection = *selection_, .dialog_service = *dialog_service_};
}
