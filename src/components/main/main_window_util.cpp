#include "components/main/main_window_util.h"

#include <Windows.h>

#include "client_utils.h"
#include "common/node_ref.h"
#include "common/node_ref_util.h"
#include "common/node_service.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "components/main/main_window.h"
#include "components/main/opened_view.h"
#include "contents_model.h"
#include "window_info.h"

void OpenView(MainWindow* main_window, const WindowDefinition& def) {
  DCHECK(main_window);
  main_window->OpenView(def, true);
}

void ExecuteDefaultItemCommand(MainWindow* main_window,
                               const NodeRef& node,
                               unsigned type,
                               unsigned shift,
                               const std::vector<scada::NodeId>& node_ids) {
  auto* view = main_window->GetActiveDataView();
  auto* contents = view ? view->GetContentsModel() : nullptr;
  if (view && contents && view->window_info().can_insert_item()) {
    if ((view->window_info().command_id == type) || (shift & MK_CONTROL)) {
      // insert items into active frame
      unsigned flags = (shift & MK_CONTROL) ? ContentsModel::APPEND : 0;
      for (auto& node_id : node_ids) {
        contents->AddContainedItem(node_id, flags);
        flags |= ContentsModel::APPEND;
      }
      return;
    }
  }

  OpenView(main_window, PrepareWindowDefinitionForOpen(node, type, node_ids));
}

void ExecuteDefaultItemCommand(scada::ViewService& view_service,
                               NodeService& node_service,
                               const NodeRef& node,
                               MainWindow* main_window) {
  assert(main_window);

  WORD shift = 0;
  if (::GetAsyncKeyState(VK_SHIFT))
    shift |= MK_SHIFT;
  if (::GetAsyncKeyState(VK_CONTROL))
    shift |= MK_CONTROL;

  if (IsInstanceOf(node, id::DataGroupType)) {
    std::vector<scada::NodeId> node_ids;
    auto weak_main_window = main_window ? main_window->GetWeakPtr() : nullptr;
    ExpandGroupItemIds(
        view_service, node_service, node,
        [weak_main_window, node, shift](std::vector<scada::NodeId> node_ids) {
          ExecuteDefaultItemCommand(weak_main_window.get(), node, ID_TABLE_VIEW,
                                    shift, std::move(node_ids));
        });
    return;
  }

  UINT type = ID_TABLE_VIEW;
  if (node.node_class() == scada::NodeClass::Variable)
    type = ID_GRAPH_VIEW;
  else if (IsInstanceOf(node, id::DeviceType))
    type = ID_WATCH_VIEW;
  else
    type = (shift & MK_CONTROL) ? ID_TABLE_EDITOR : ID_PROPERTY_VIEW;

  ExecuteDefaultItemCommand(main_window, node, type, shift, {node.id()});
}
