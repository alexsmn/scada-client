#include "main_window/selection_commands.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "base/program_options.h"
#include "base/promise_executor.h"
#include "base/u16format.h"
#include "client_utils.h"
#include "clipboard/clipboard_util.h"
#include "common_resources.h"
#include "components/device_metrics/device_metrics_command.h"
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
#include "main_window/main_window_interface.h"
#include "main_window/main_window_manager.h"
#include "main_window/main_window_util.h"
#include "main_window/opened_view.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/filesystem_node_ids.h"
#include "model/scada_node_ids.h"
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

BasicCommand<SelectionCommandContext> MakeOpenViewCommand(
    unsigned command_id,
    const WindowInfo& window_info) {
  return BasicCommand<SelectionCommandContext>{
      .command_id = command_id,
      .execute_handler =
          [&window_info](const SelectionCommandContext& context) {
            context.opened_view
                .GetOpenWindowDefinition(&window_info)
                // TODO: Capture `main_window` as a weak_ptr.
                .then([&main_window = context.main_window](
                          const WindowDefinition& window_def) {
                  return main_window.OpenView(window_def);
                });
          },
      .available_handler =
          [](const SelectionCommandContext& context) {
            return !context.selection.empty();
          }};
}

}  // namespace

// SelectionCommands

SelectionCommands::SelectionCommands(SelectionCommandsContext&& context)
    : SelectionCommandsContext{std::move(context)} {
  // |selection_| and |dialog_service_| are never null in command handlers.

  selection_commands_.AddCommand(
      MakeOpenViewCommand(ID_OPEN_TABLE, kTableWindowInfo));

  selection_commands_.AddCommand(
      MakeOpenViewCommand(ID_OPEN_SUMMARY, kSummaryWindowInfo));

  selection_commands_.AddCommand(
      MakeOpenViewCommand(ID_TIMED_DATA_VIEW, kTimedDataWindowInfo));

#if !defined(UI_WT)
  selection_commands_.AddCommand(
      MakeOpenViewCommand(ID_OPEN_GRAPH, kGraphWindowInfo));
#endif

  command_registry_.AddCommand(
      Command{ID_OPEN_DEVICE_METRICS}
          .set_execute_handler([this] {
            MakeDeviceMetricsWindowDefinition(selection_->node())
                .then(cancelation_.Bind(
                    [this](const WindowDefinition& window_definition) {
                      OpenWindow(window_definition);
                    }));
          })
          .set_available_handler([this] {
            return IsInstanceOf(selection()->node(), devices::id::DeviceType);
          }));

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
      Command{ID_OPEN_GROUP_TABLE}
          .set_execute_handler([this] {
            // TODO: Capture |main_window_| by weak pointer.
            MakeGroupWindowDefinition(&kTableWindowInfo, selection_->node())
                .then([main_window = main_window_](
                          const std::optional<WindowDefinition>& window_def) {
                  if (window_def.has_value()) {
                    ::OpenView(main_window, *window_def);
                  }
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
        .then(cancelation_.Bind(
            [this](const WindowDefinition& window_definition) {
              OpenWindow(window_definition);
            }));
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

promise<OpenedViewInterface*> SelectionCommands::OpenViewContainingNode(
    int view_type_id,
    const NodeRef& node) {
  assert(main_window_);
  assert(dialog_service_);

  auto cached_items =
      file_cache_.GetList(view_type_id).GetFilesContainingItem(node.node_id());

  if (cached_items.empty()) {
    auto msg = u16format(L"Display for item \"{}\" was not found.",
                         ToString16(node.display_name()));
    return ToRejectedPromise<OpenedViewInterface*>(
        dialog_service_->RunMessageBox(msg, Translate("Display"), MessageBoxMode::Info));
  }

  // TODO: Let user select scheme from list.
  const FileCache::DisplayItem& cached_item = cached_items.front();
  const std::filesystem::path& path = cached_item.first;

  promise<OpenedViewInterface*> open_promise;

  if (auto* view = main_window_manager_.FindOpenedViewByFilePath(path); view) {
    view->Activate();
    open_promise = make_resolved_promise<OpenedViewInterface*>(view);
  } else {
    WindowDefinition win(GetWindowInfo(view_type_id));
    win.path = path;
    open_promise = main_window_->OpenView(win);
  }

  return open_promise.then(
      [node_id = node.node_id()](OpenedViewInterface* opened_view) {
        opened_view->Select(node_id);
        return opened_view;
      });
}

void SelectionCommands::SetContext(MainWindowInterface* main_window,
                                   DialogService* dialog_service,
                                   OpenedViewInterface* opened_view,
                                   Controller* controller,
                                   SelectionModel* selection) {
  main_window_ = main_window;
  dialog_service_ = dialog_service;
  opened_view_ = opened_view;
  controller_ = controller;
  selection_ = selection;
}

void SelectionCommands::DeleteSelection() {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure)) {
    return;
  }

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

  if (nodes.empty()) {
    return;
  }

  auto message =
      nodes.size() == 1
          ? u16format(L"Are you sure you want to delete {}?",
                      nodes.front().display_name())
          : u16format(L"Are you sure you want to delete {} items?",
                      nodes.size());

  dialog_service_
      ->RunMessageBox(message, Translate("Delete"), MessageBoxMode::QuestionYesNo)
      .then(BindPromiseExecutor(
          executor_, [&task_manager = task_manager_,
                      nodes = std::move(nodes)](MessageBoxResult result) {
            if (result != MessageBoxResult::Yes) {
              return;
            }

            for (const NodeRef& node : nodes) {
              DeleteTreeRecordsRecursive(task_manager, node);
            }
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
  assert(main_window_);
  assert(opened_view_);

  return {.selection = *selection_,
          .dialog_service = *dialog_service_,
          .main_window = *main_window_,
          .opened_view = *opened_view_};
}
