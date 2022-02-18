#include "window_definition_builder.h"

#include "base/string_piece_util.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "client_utils.h"
#include "common/formula_util.h"
#include "common_resources.h"
#include "components/table/table_component.h"
#include "controller.h"
#include "model/data_items_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"
#include "window_info.h"

#if !defined(UI_WT)
#include "components/graph/graph_component.h"
#endif

namespace {

#if defined(UI_WT)
static const WindowInfo& kDefaultWindowInfo = kTableWindowInfo;
#else
static const WindowInfo& kDefaultWindowInfo = kGraphWindowInfo;
#endif

static const WindowInfo& kDefaultMultiWindowInfo = kTableWindowInfo;

std::u16string MakeTitle(const WindowInfo& window_info, const NodeRef& node) {
  return base::StrCat({AsStringPiece(window_info.title), u": ",
                       ToString16(node.display_name())});
}

}  // namespace

WindowDefinition MakeEmptyWindowDefinition(const WindowInfo* window_info,
                                           const NodeRef& node) {
  if (!window_info)
    window_info = &kDefaultWindowInfo;

  WindowDefinition window_def{*window_info};
  window_def.title = MakeTitle(*window_info, node);
  return window_def;
}

WindowDefinition MakeSingleWindowDefinition(const WindowInfo* window_info,
                                            const NodeRef& node) {
  auto window_def = MakeEmptyWindowDefinition(window_info, node);

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
promise<WindowDefinition> MakeWindowDefinition(const WindowInfo* window_info,
                                               const NodeRef& node,
                                               bool expand_groups) {
  promise<NodeIdSet> node_ids_promise;
  if (expand_groups && node && node.node_class() == scada::NodeClass::Object)
    node_ids_promise = ExpandGroupItemIds(node);
  else
    node_ids_promise = make_resolved_promise(MakeNodeIdSet(node.node_id()));

  return node_ids_promise.then([window_info, node](const NodeIdSet& node_ids) {
    auto window_def = MakeEmptyWindowDefinition(window_info, node);
    AddNodeIds(window_def, node_ids);
    return window_def;
  });
}

promise<WindowDefinition> MakeWindowDefinition(
    const WindowInfo* window_info,
    const OpenContext& open_context) {
  auto promise = open_context.node ? MakeWindowDefinition(
                                         window_info, open_context.node, true)
                                   : make_resolved_promise(MakeWindowDefinition(
                                         window_info, open_context.node_ids));

  return promise.then([open_context](const WindowDefinition& window_def) {
    auto new_window_def = window_def;
    if (open_context.time_range.has_value())
      SaveTimeRange(new_window_def, *open_context.time_range);
    return new_window_def;
  });
}

WindowDefinition MakeWindowDefinition(const WindowInfo* window_info,
                                      const NodeRef& node,
                                      const NodeIdSet& item_ids) {
  if (!window_info)
    window_info = &kDefaultWindowInfo;

  WindowDefinition window_def(*window_info);
  window_def.title = MakeTitle(*window_info, node);

  for (auto& id : item_ids) {
    WindowItem& item_id = window_def.AddItem("Item");
    item_id.SetString("path", MakeNodeIdFormula(id));
  }

  return window_def;
}

WindowDefinition MakeWindowDefinition(
    const WindowInfo* window_info,
    const std::vector<scada::NodeId>& node_ids,
    std::u16string title) {
  if (!window_info)
    window_info = &kDefaultMultiWindowInfo;

  WindowDefinition window_def(*window_info);
  window_def.title = std::move(title);

  for (auto& node_id : node_ids) {
    WindowItem& item = window_def.AddItem("Item");
    item.SetString("path", MakeNodeIdFormula(node_id));
  }

  return window_def;
}

WindowDefinition MakeWindowDefinition(const WindowInfo* window_info,
                                      std::string formula) {
  if (!window_info)
    window_info = &kDefaultWindowInfo;

  WindowDefinition window_def(*window_info);
  window_def.title = base::UTF8ToUTF16(formula);

  WindowItem& item = window_def.AddItem("Item");
  item.SetString("path", std::move(formula));

  return window_def;
}

promise<std::optional<WindowDefinition>> MakeGroupWindowDefinition(
    const WindowInfo* window_info,
    const NodeRef& node) {
  auto parent = node.parent();
  if (!IsInstanceOf(parent, data_items::id::DataGroupType))
    return make_resolved_promise(std::optional<WindowDefinition>());

  return ExpandGroupItemIds(parent).then(
      [window_info, node](const NodeIdSet& node_ids) {
        return std::optional<WindowDefinition>{
            MakeWindowDefinition(window_info, node, node_ids)};
      });
}
