#include "window_definition_builder.h"

#include "client_utils.h"
#include "common/formula_util.h"
#include "common_resources.h"
#include "controller.h"
#include "model/data_items_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"

WindowDefinition MakeEmptyWindowDefinition(const NodeRef& node, unsigned type) {
  if (!type)
    type = ID_GRAPH_VIEW;

#if defined(UI_QT)
  if (type == ID_PROPERTY_VIEW)
    type = ID_NEW_PROPERTY_VIEW;
#endif

  const WindowInfo& window_info = GetWindowInfo(type);

  WindowDefinition window_def{window_info};
  window_def.title = base::StringPrintf(
      L"%ls: %ls", window_info.title, ToString16(node.display_name()).c_str());
  return window_def;
}

WindowDefinition MakeSingleWindowDefinition(const NodeRef& node,
                                            unsigned type) {
  auto window_def = MakeEmptyWindowDefinition(node, type);

  auto& item_id = window_def.AddItem("Item");
  item_id.SetString("path", MakeNodeIdFormula(node.node_id()));

  return window_def;
}

void AddNodeIds(WindowDefinition& window_def, const NodeIdSet& node_ids) {
  for (const auto& node_id : node_ids) {
    WindowItem& item_id = window_def.AddItem("Item");
    item_id.SetString("path", MakeNodeIdFormula(node_id));
  }
}

// TODO: Combine with |MakeSingleWindowDefinition()|.
promise<WindowDefinition> MakeWindowDefinition(const NodeRef& node,
                                               unsigned type,
                                               bool expand_groups) {
  promise<NodeIdSet> node_ids_promise;
  if (expand_groups && node && node.node_class() == scada::NodeClass::Object)
    node_ids_promise = ExpandGroupItemIds(node);
  else
    node_ids_promise = make_resolved_promise(MakeNodeIdSet(node.node_id()));

  return node_ids_promise.then([node, type](const NodeIdSet& node_ids) {
    auto window_def = MakeEmptyWindowDefinition(node, type);
    AddNodeIds(window_def, node_ids);
    return window_def;
  });
}

promise<WindowDefinition> MakeWindowDefinition(const OpenContext& open_context,
                                               unsigned type) {
  auto promise = open_context.node
                     ? MakeWindowDefinition(open_context.node, type, true)
                     : make_resolved_promise(
                           MakeWindowDefinition(open_context.node_ids, type));

  return promise.then([open_context](const WindowDefinition& window_def) {
    auto new_window_def = window_def;
    if (open_context.time_range.has_value())
      SaveTimeRange(new_window_def, *open_context.time_range);
    return new_window_def;
  });
}

WindowDefinition MakeWindowDefinition(const NodeRef& node,
                                      unsigned type,
                                      const NodeIdSet& item_ids) {
  if (!type)
    type = ID_GRAPH_VIEW;

  const WindowInfo& window_info = GetWindowInfo(type);
  WindowDefinition window_def(GetWindowInfo(type));
  window_def.title = base::StringPrintf(
      L"%ls: %ls", window_info.title, ToString16(node.display_name()).c_str());

  for (auto& id : item_ids) {
    WindowItem& item_id = window_def.AddItem("Item");
    item_id.SetString("path", MakeNodeIdFormula(id));
  }

  return window_def;
}

WindowDefinition MakeWindowDefinition(
    const std::vector<scada::NodeId>& node_ids,
    unsigned type,
    const wchar_t* title) {
  if (!type)
    type = ID_TABLE_VIEW;

  WindowDefinition window_def(GetWindowInfo(type));
  if (title)
    window_def.title = title;

  for (auto& node_id : node_ids) {
    WindowItem& item = window_def.AddItem("Item");
    item.SetString("path", MakeNodeIdFormula(node_id));
  }

  return window_def;
}

WindowDefinition MakeWindowDefinition(const char* formula, unsigned type) {
  if (!type)
    type = ID_GRAPH_VIEW;

  WindowDefinition window_def(GetWindowInfo(type));
  window_def.title = base::SysNativeMBToWide(formula);

  WindowItem& item = window_def.AddItem("Item");
  item.SetString("path", formula);

  return window_def;
}

promise<std::optional<WindowDefinition>> MakeGroupWindowDefinition(
    const NodeRef& node,
    unsigned type) {
  auto parent = node.parent();
  if (!IsInstanceOf(parent, data_items::id::DataGroupType))
    return make_resolved_promise(std::optional<WindowDefinition>());

  return ExpandGroupItemIds(parent).then(
      [node, type](const NodeIdSet& node_ids) {
        return std::optional<WindowDefinition>{
            MakeWindowDefinition(node, type, node_ids)};
      });
}
