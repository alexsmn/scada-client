#include "components/main/main_window_util.h"

#include "aui/key_codes.h"
#include "client_utils.h"
#include "common_resources.h"
#include "components/main/main_window.h"
#include "components/main/opened_view.h"
#include "components/node_properties/node_property_component.h"
#include "components/node_table/node_table_component.h"
#include "components/table/table_component.h"
#include "components/watch/watch_component.h"
#include "controller/contents_model.h"
#include "controller/window_info.h"
#include "filesystem/filesystem_commands.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/filesystem_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"
#include "window_definition_builder.h"

#if !defined(UI_WT)
#include "components/graph/graph_component.h"
#endif

#include <cassert>

void OpenView(MainWindow* main_window,
              promise<WindowDefinition> window_def_promise,
              bool activate) {
  // TODO: Pass |main_window| by weak pointer.
  window_def_promise.then([main_window, activate](const WindowDefinition& def) {
    OpenView(main_window, def, activate);
  });
}

void OpenView(MainWindow* main_window,
              const WindowDefinition& window_def,
              bool activate) {
  assert(main_window);
  main_window->OpenView(window_def, activate);
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

bool ExecuteDefaultNodeCommand(MainWindow* main_window,
                               const std::shared_ptr<Executor>& executor,
                               const FileRegistry& file_registry,
                               const NodeRef& node,
                               aui::KeyModifiers key_modifiers) {
  assert(main_window);

  if (IsInstanceOf(node, filesystem::id::FileType)) {
    ExecuteFileCommand(main_window, executor, file_registry, node,
                       key_modifiers);
    return true;
  }

  const auto& window_info = GetDefaultNodeWindowInfo(node, key_modifiers);

  auto* view = main_window->GetActiveDataView();
  auto* contents = view ? view->GetContentsModel() : nullptr;
  if (view && contents && view->window_info().can_insert_item()) {
    if ((&view->window_info() == &window_info) ||
        (key_modifiers & aui::ControlModifier)) {
      // insert items into active frame
      promise<NodeIdSet> node_ids_promise =
          IsInstanceOf(node, data_items::id::DataGroupType)
              ? ExpandGroupItemIds(node)
              : make_resolved_promise(MakeNodeIdSet(node.node_id()));
      // TODO: Capture weak pointer.
      node_ids_promise.then([contents,
                             key_modifiers](const NodeIdSet& node_ids) {
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

  auto window_definition = MakeWindowDefinition(&window_info, node, true);

  // TODO: Capture |main_window| by weak pointer.

  OpenView(main_window, window_definition);
  return true;
}
