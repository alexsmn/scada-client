#include "components/main/main_window_util.h"

#include "client_utils.h"
#include "common_resources.h"
#include "components/main/main_window.h"
#include "components/main/opened_view.h"
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

bool ExecuteDefaultNodeCommand(MainWindow* main_window,
                               const NodeRef& node,
                               unsigned shift) {
  assert(main_window);

  UINT type;
  if (IsInstanceOf(node, data_items::id::DataGroupType))
    type = ID_TABLE_VIEW;
  else if (IsInstanceOf(node, data_items::id::DataItemType))
    type = ID_GRAPH_VIEW;
  else if (IsInstanceOf(node, devices::id::DeviceType))
    type = ID_WATCH_VIEW;
  else
    type = (shift & MK_CONTROL) ? ID_TABLE_EDITOR : ID_PROPERTY_VIEW;

  auto* view = main_window->GetActiveDataView();
  auto* contents = view ? view->GetContentsModel() : nullptr;
  if (view && contents && view->window_info().can_insert_item()) {
    if ((view->window_info().command_id == type) || (shift & MK_CONTROL)) {
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

  // TODO: Capture |main_window| by weak pointer.

  OpenView(main_window, MakeWindowDefinition(node, type, true));
  return true;
}
