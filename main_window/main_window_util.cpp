#include "main_window/main_window_util.h"

#include "aui/key_codes.h"
#include "base/awaitable.h"
#include "base/awaitable_promise.h"
#include "ui/common/client_utils.h"
#include "resources/common_resources.h"
#include "components/node_properties/node_property_component.h"
#include "components/node_table/node_table_component.h"
#include "components/table/table_component.h"
#include "components/watch/watch_component.h"
#include "controller/contents_model.h"
#include "controller/window_info.h"
#include "filesystem/filesystem_commands.h"
#include "main_window/main_window.h"
#include "main_window/opened_view.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/filesystem_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"
#include "window_definition_builder.h"

#if !defined(UI_WT)
#include "graph/graph_component.h"
#endif

#include <cassert>

promise<void> OpenView(MainWindowInterface* main_window,
                                     const WindowDefinition& window_def,
                                     bool activate) {
  assert(main_window);
  return ToVoidPromise(main_window->OpenView(window_def, activate));
}

const WindowInfo& GetDefaultNodeWindowInfo(const NodeRef& node,
                                           aui::KeyModifiers key_modifiers) {
  if (IsInstanceOf(node, data_items::id::DataGroupType))
    return kTableWindowInfo;
#if !defined(UI_WT)
  else if (IsInstanceOf(node, data_items::id::DataItemType))
    return kGraphWindowInfo;
#endif
  else if (IsInstanceOf(node, devices::id::DeviceType))
    return kWatchWindowInfo;
  else
    return (key_modifiers & aui::ControlModifier) ? kTableEditorWindowInfo
                                                  : kNodePropertyWindowInfo;
}

bool ExecuteDefaultNodeCommand(const std::shared_ptr<Executor>& executor,
                               const OpenFileCommand& file_command,
                               const NodeCommandContext& context) {
  assert(context.main_window);

  if (IsInstanceOf(context.node, filesystem::id::FileType)) {
    file_command(OpenFileCommandContext{context.main_window,
                                        context.dialog_service, executor,
                                        context.node, context.key_modifiers});
    return true;
  }

  const auto& window_info =
      GetDefaultNodeWindowInfo(context.node, context.key_modifiers);

  auto* view = context.main_window->GetActiveDataView();
  ContentsModel* contents = view ? view->GetContents() : nullptr;
  if (view && contents && view->GetWindowInfo().can_insert_item()) {
    if ((&view->GetWindowInfo() == &window_info) ||
        (context.key_modifiers & aui::ControlModifier)) {
      // insert items into active frame
      promise<NodeIdSet> node_ids_promise =
          IsInstanceOf(context.node, data_items::id::DataGroupType)
              ? ExpandGroupItemIds(context.node)
              : make_resolved_promise(MakeNodeIdSet(context.node.node_id()));

      // TODO: Capture weak pointer.
      CoSpawn(executor, [executor, contents,
                         key_modifiers = context.key_modifiers,
                         node_ids_promise = std::move(node_ids_promise)]() mutable
                        -> Awaitable<void> {
        auto node_ids = co_await AwaitPromise(NetExecutorAdapter{executor},
                                              std::move(node_ids_promise));
        unsigned flags =
            (key_modifiers & aui::ControlModifier) ? ContentsModel::APPEND : 0;
        for (const auto& node_id : node_ids) {
          contents->AddContainedItem(node_id, flags);
          flags |= ContentsModel::APPEND;
        }
        co_return;
      });
      return true;
    }
  }

  // TODO: Capture |main_window| by weak pointer.
  auto window_def_promise =
      MakeWindowDefinition(&window_info, context.node, /*include_default=*/true);
  auto* main_window = context.main_window;
  CoSpawn(executor, [executor, main_window,
                     window_def_promise = std::move(window_def_promise)]() mutable
                    -> Awaitable<void> {
    auto window_definition = co_await AwaitPromise(
        NetExecutorAdapter{executor}, std::move(window_def_promise));
    OpenView(main_window, window_definition);
    co_return;
  });
  return true;
}
