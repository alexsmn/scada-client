#include "components/main/main_window_util.h"

#include "client_utils.h"
#include "common/node_ref.h"
#include "common/node_util.h"
#include "common_resources.h"
#include "components/main/main_window.h"
#include "components/main/opened_view.h"
#include "contents_model.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/scada_node_ids.h"
#include "window_info.h"

#include <cassert>

void OpenView(MainWindow* main_window,
              const WindowDefinition& def,
              bool activate) {
  assert(main_window);
  main_window->OpenView(def, activate);
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
      NodeIdSet trids;
      if (IsInstanceOf(node, data_items::id::DataGroupType))
        ExpandGroupItemIds(node, trids);
      else
        trids.insert(node.node_id());
      unsigned flags = (shift & MK_CONTROL) ? ContentsModel::APPEND : 0;
      for (NodeIdSet::iterator i = trids.begin(); i != trids.end(); ++i) {
        contents->AddContainedItem(*i, flags);
        flags |= ContentsModel::APPEND;
      }
      return true;
    }
  }

  OpenView(main_window, MakeWindowDefinition(node, type, true));
  return true;
}
