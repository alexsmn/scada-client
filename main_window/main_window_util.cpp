#include "main_window/main_window_util.h"

#include "aui/key_codes.h"
#include "client_utils.h"
#include "common_resources.h"
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

promise<void> OpenView(
    MainWindowInterface* main_window,
    promise<WindowDefinition> window_def_promise,
    bool activate) {
  // TODO: Pass |main_window| by weak pointer.
  return window_def_promise.then(
      [main_window, activate](const WindowDefinition& def) {
        return OpenView(main_window, def, activate);
      });
}

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
      node_ids_promise.then([contents, key_modifiers = context.key_modifiers](
                                const NodeIdSet& node_ids) {
        unsigned flags =
            (key_modifiers & aui::ControlModifier) ? ContentsModel::APPEND : 0;
        for (const auto& node_id : node_ids) {
          contents->AddContainedItem(node_id, flags);
          flags |= ContentsModel::APPEND;
        }
      });
      return true;
    }
  }

  auto window_definition =
      MakeWindowDefinition(&window_info, context.node, true);

  // TODO: Capture |main_window| by weak pointer.

  OpenView(context.main_window, window_definition);
  return true;
}
