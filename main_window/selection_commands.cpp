#include "main_window/selection_commands.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/program_options.h"
#include "base/u16format.h"
#include "ui/common/client_utils.h"
#include "clipboard/clipboard_util.h"
#include "resources/common_resources.h"
#include "modules/device_metrics/device_metrics_command.h"
#include "modules/node_properties/node_property_component.h"
#include "modules/node_table/node_table_component.h"
#include "modules/summary/summary_component.h"
#include "modules/table/table_component.h"
#include "modules/timed_data/timed_data_component.h"
#include "modules/transmission/transmission_component.h"
#include "modules/watch/watch_component.h"
#include "controller/selection_model.h"
#include "controller/window_info.h"
#include "core/selection_command_context.h"
#include "events/node_event_provider.h"
#include "filesystem/file_cache.h"
#include "main_window/main_window_interface.h"
#include "main_window/main_window_manager.h"
#include "main_window/main_window_util.h"
#include "main_window/opened_view/opened_view.h"
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

#include <exception>

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
    const WindowInfo& window_info,
    AnyExecutor executor) {
  return BasicCommand<SelectionCommandContext>{
      .command_id = command_id,
      .execute_handler =
          [&window_info,
           executor = std::move(executor)](const SelectionCommandContext& context) {
            auto window_def =
                context.opened_view.GetOpenWindowDefinition(&window_info);
            CoSpawn(executor, [&main_window = context.main_window,
                               window_def = std::move(window_def)]() mutable
                              -> Awaitable<void> {
              co_await main_window.OpenView(co_await std::move(window_def));
              co_return;
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
      MakeOpenViewCommand(ID_OPEN_TABLE, kTableWindowInfo, executor_));

  selection_commands_.AddCommand(
      MakeOpenViewCommand(ID_OPEN_SUMMARY, kSummaryWindowInfo, executor_));

  selection_commands_.AddCommand(
      MakeOpenViewCommand(ID_TIMED_DATA_VIEW, kTimedDataWindowInfo, executor_));

#if !defined(UI_WT)
  selection_commands_.AddCommand(
      MakeOpenViewCommand(ID_OPEN_GRAPH, kGraphWindowInfo, executor_));
#endif

  selection_commands_.AddCommand(
      BasicCommand<SelectionCommandContext>{ID_OPEN_DEVICE_METRICS}
          .set_execute_handler([this](const SelectionCommandContext& context) {
            // The coroutine is gated by `cancelation_` so it cannot run after
            // this `SelectionCommands` is destroyed.
            CoSpawn(executor_, cancelation_,
                    [this, node = context.selection.node()]() mutable
                    -> Awaitable<void> {
                      auto window_definition =
                          co_await MakeDeviceMetricsWindowDefinitionAsync(
                              executor_, node);
                      OpenWindow(window_definition);
                      co_return;
                    });
          })
          .set_available_handler([](const SelectionCommandContext& context) {
            return IsInstanceOf(context.selection.node(),
                                devices::id::DeviceType);
          }));

#if !defined(UI_WT)
  selection_commands_.AddCommand(
      BasicCommand<SelectionCommandContext>{ID_OPEN_DISPLAY}
          .set_execute_handler([this](const SelectionCommandContext& context) {
            CoSpawn(executor_, cancelation_,
                    [this, node = context.selection.node(),
                     main_window = &context.main_window,
                     dialog_service = &context.dialog_service]() mutable
                    -> Awaitable<void> {
                      co_await OpenViewContainingNode(ID_MODUS_VIEW, node,
                                                      *main_window,
                                                      *dialog_service);
                      co_return;
                    });
          })
          .set_available_handler(
              [](const SelectionCommandContext& context) {
                return context.selection.timed_data().connected();
              }));
#endif

  selection_commands_.AddCommand(
      BasicCommand<SelectionCommandContext>{ID_OPEN_GROUP_TABLE}
          .set_execute_handler([this](const SelectionCommandContext& context) {
            CoSpawn(executor_, [executor = executor_,
                                main_window = &context.main_window,
                                node = context.selection.node()]() mutable
                               -> Awaitable<void> {
              auto window_def = co_await MakeGroupWindowDefinitionAsync(
                  executor, &kTableWindowInfo, node);
              if (window_def.has_value()) {
                co_await ::OpenView(main_window, *window_def);
              }
              co_return;
            });
          })
          .set_available_handler(
              [](const SelectionCommandContext& context) {
                return context.selection.timed_data().connected();
              }));

  selection_commands_.AddCommand(
      BasicCommand<SelectionCommandContext>{ID_ITEM_PARAMS}
          .set_execute_handler([this](const SelectionCommandContext& context) {
            OpenWindow(MakeSingleWindowDefinition(&kNodePropertyWindowInfo,
                                                  context.selection.node()));
          })
          .set_available_handler([this](const SelectionCommandContext& context) {
            return session_service_.HasPrivilege(scada::Privilege::Configure) &&
                   context.selection.node();
          }));

  selection_commands_.AddCommand(
      BasicCommand<SelectionCommandContext>{ID_TABLE_CONFIG}
          .set_execute_handler([this](const SelectionCommandContext& context) {
            OpenWindow(MakeSingleWindowDefinition(&kTableEditorWindowInfo,
                                                  context.selection.node()));
          })
          .set_available_handler([this](const SelectionCommandContext& context) {
            return session_service_.HasPrivilege(scada::Privilege::Configure) &&
                   CanCreateSomething(context.selection.node());
          }));

  selection_commands_.AddCommand(
      BasicCommand<SelectionCommandContext>{ID_OPEN_WATCH}
          .set_execute_handler([this](const SelectionCommandContext& context) {
            OpenWindow(MakeSingleWindowDefinition(&kWatchWindowInfo,
                                                  context.selection.node()));
          })
          .set_available_handler([](const SelectionCommandContext& context) {
            return IsInstanceOf(context.selection.node(),
                                devices::id::DeviceType);
          }));

  // ID_TRANSMISSION_VIEW
  selection_commands_.AddCommand(
      BasicCommand<SelectionCommandContext>{ID_TRANSMISSION_VIEW}
          .set_execute_handler([this](const SelectionCommandContext& context) {
            OpenWindow(MakeSingleWindowDefinition(&kTransmissionWindowInfo,
                                                  context.selection.node()));
          })
          .set_available_handler([this](const SelectionCommandContext& context) {
            return session_service_.HasPrivilege(scada::Privilege::Configure) &&
                   IsInstanceOf(context.selection.node(),
                                devices::id::DeviceType) &&
                   !IsInstanceOf(context.selection.node(),
                                 devices::id::LinkType);
          }));

  selection_commands_.AddCommand(
      BasicCommand<SelectionCommandContext>{ID_COPY}
          .set_execute_handler([this](const SelectionCommandContext& context) {
            CopyToClipboard(context);
          })
          .set_enabled_handler([](const SelectionCommandContext& context) {
            return !context.selection.empty();
          })
          .set_available_handler([this](const SelectionCommandContext& context) {
            return session_service_.HasPrivilege(scada::Privilege::Configure) &&
                   !context.selection.empty();
          }));

  selection_commands_.AddCommand(
      BasicCommand<SelectionCommandContext>{ID_DELETE}
          .set_execute_handler([this](const SelectionCommandContext& context) {
            DeleteSelection(context);
          })
          .set_enabled_handler([](const SelectionCommandContext& context) {
            return !context.selection.empty();
          })
          .set_available_handler([this](const SelectionCommandContext& context) {
            return session_service_.HasPrivilege(scada::Privilege::Configure) &&
                   !context.selection.empty();
          }));

  // TODO: Move to the event module.
  selection_commands_.AddCommand(
      BasicCommand<SelectionCommandContext>{ID_ACKNOWLEDGE_CURRENT}
          .set_execute_handler([this](const SelectionCommandContext& context) {
            node_event_provider_.AcknowledgeItemEvents(
                context.selection.node().node_id());
          })
          .set_enabled_handler([](const SelectionCommandContext& context) {
            return context.selection.timed_data().alerting();
          })
          .set_available_handler([](const SelectionCommandContext& context) {
            return context.selection.timed_data().connected();
          }));
}

void SelectionCommands::OpenWindow(const WindowInfo* window_info) {
  if (selection_ && !selection_->empty()) {
    // TODO: Capture |main_window_| by weak pointer.
    CoSpawn(executor_, cancelation_,
            [this, window_info, node = selection_->node()]() mutable
            -> Awaitable<void> {
              auto window_definition = co_await MakeWindowDefinitionAsync(
                  executor_, window_info, node,
                  /*expand_groups=*/true);
              co_await ::OpenView(main_window_, window_definition);
              co_return;
            });
  }
}

void SelectionCommands::OpenWindow(const WindowDefinition& window_definition) {
  CoSpawn(executor_, [this, window_definition]() -> Awaitable<void> {
    co_await ::OpenView(main_window_, window_definition, true);
  });
}

Awaitable<OpenedViewInterface*> SelectionCommands::OpenViewContainingNode(
    int view_type_id,
    const NodeRef& node,
    MainWindowInterface& main_window,
    DialogService& dialog_service) {
  co_return co_await OpenViewContainingNodeAsync(view_type_id, node, main_window,
                                                dialog_service);
}

Awaitable<OpenedViewInterface*> SelectionCommands::OpenViewContainingNodeAsync(
    int view_type_id,
    NodeRef node,
    MainWindowInterface& main_window,
    DialogService& dialog_service) {

  auto cached_items =
      file_cache_.GetList(view_type_id).GetFilesContainingItem(node.node_id());

  if (cached_items.empty()) {
    auto msg = u16format(L"Display for item \"{}\" was not found.",
                         ToString16(node.display_name()));
    co_await dialog_service.RunMessageBox(msg, Translate("Display"),
                                          MessageBoxMode::Info);
    throw std::exception{};
  }

  // TODO: Let user select scheme from list.
  const FileCache::DisplayItem& cached_item = cached_items.front();
  const std::filesystem::path& path = cached_item.first;

  OpenedViewInterface* opened_view = nullptr;

  if (auto* view = main_window_manager_.FindOpenedViewByFilePath(path); view) {
    view->Activate();
    opened_view = view;
  } else {
    WindowDefinition win(GetWindowInfo(view_type_id));
    win.path = path;
    opened_view = co_await main_window.OpenView(win);
  }

  opened_view->Select(node.node_id());
  co_return opened_view;
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

void SelectionCommands::DeleteSelection(const SelectionCommandContext& context) {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure)) {
    return;
  }

  std::vector<NodeRef> nodes;

  if (context.selection.multiple()) {
    auto node_ids = context.selection.GetMultipleNodeIds();
    nodes.reserve(node_ids.size());
    std::ranges::transform(
        node_ids, std::back_inserter(nodes),
        [&node_service = node_service_](const scada::NodeId& node_id) {
          return node_service.GetNode(node_id);
        });

  } else if (auto node = context.selection.node()) {
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

  CoSpawn(executor_, [executor = executor_, &task_manager = task_manager_,
                      &dialog_service = context.dialog_service,
                      message = std::move(message),
                      nodes = std::move(nodes)]() mutable
                     -> Awaitable<void> {
    auto result = co_await dialog_service.RunMessageBox(
        message, Translate("Delete"), MessageBoxMode::QuestionYesNo);
    if (result != MessageBoxResult::Yes) {
      co_return;
    }
    for (const NodeRef& node : nodes) {
      DeleteTreeRecordsRecursive(task_manager, node);
    }
    co_return;
  });
}

void SelectionCommands::CopyToClipboard(const SelectionCommandContext& context) {
  std::vector<NodeRef> nodes;

  if (context.selection.multiple()) {
    for (const auto& node_id : context.selection.GetMultipleNodeIds()) {
      const auto& node = node_service_.GetNode(node_id);
      nodes.emplace_back(node);
      GetNodesRecursive(node, nodes);
    }

  } else if (const auto& node = context.selection.node()) {
    nodes.emplace_back(node);
  }

  if (!nodes.empty())
    CopyNodesToClipboard(nodes);
}
