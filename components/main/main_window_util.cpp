#include "components/main/main_window_util.h"

#include "client_utils.h"
#include "common_resources.h"
#include "components/graph/graph_component.h"
#include "components/main/main_window.h"
#include "components/main/opened_view.h"
#include "components/node_properties/node_property_component.h"
#include "components/node_table/node_table_component.h"
#include "components/table/table_component.h"
#include "components/watch/watch_component.h"
#include "contents_model.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/scada_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"
#include "window_definition_builder.h"
#include "window_info.h"

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
                                           unsigned shift) {
  if (IsInstanceOf(node, data_items::id::DataGroupType))
    return kTableWindowInfo;
  else if (IsInstanceOf(node, data_items::id::DataItemType))
    return kGraphWindowInfo;
  else if (IsInstanceOf(node, devices::id::DeviceType))
    return kWatchWindowInfo;
  else
    return (shift & MK_CONTROL) ? kTableEditorWindowInfo
                                : kNodePropertyWindowInfo;
}

bool ExecuteDefaultNodeCommand(MainWindow* main_window,
                               const NodeRef& node,
                               unsigned shift) {
  assert(main_window);

  const auto& window_info = GetDefaultNodeWindowInfo(node, shift);

  auto* view = main_window->GetActiveDataView();
  auto* contents = view ? view->GetContentsModel() : nullptr;
  if (view && contents && view->window_info().can_insert_item()) {
    if ((&view->window_info() == &window_info) || (shift & MK_CONTROL)) {
      // insert items into active frame
      promise<NodeIdSet> node_ids_promise =
          IsInstanceOf(node, data_items::id::DataGroupType)
              ? ExpandGroupItemIds(node)
              : make_resolved_promise(MakeNodeIdSet(node.node_id()));
      // TODO: Capture weak pointer.
      node_ids_promise.then([contents, shift](const NodeIdSet& node_ids) {
        unsigned flags = (shift & MK_CONTROL) ? ContentsModel::APPEND : 0;
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
