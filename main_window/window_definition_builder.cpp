#include "window_definition_builder.h"

#include "base/awaitable_promise.h"
#include "base/executor_conversions.h"
#include "base/u16format.h"
#include "base/utf_convert.h"
#include "ui/common/client_utils.h"
#include "common/formula_util.h"
#include "resources/common_resources.h"
#include "components/table/table_component.h"
#include "controller/controller.h"
#include "controller/window_info.h"
#include "model/data_items_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"

#if !defined(UI_WT)
#include "graph/graph_component.h"
#endif

namespace {

#if defined(UI_WT)
static const WindowInfo& kDefaultWindowInfo = kTableWindowInfo;
#else
static const WindowInfo& kDefaultWindowInfo = kGraphWindowInfo;
#endif

static const WindowInfo& kDefaultMultiWindowInfo = kTableWindowInfo;

std::u16string MakeTitle(const WindowInfo& window_info, const NodeRef& node) {
  return u16format(L"{}: {}", window_info.title,
                    ToString16(node.display_name()));
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
promise<WindowDefinition> MakeWindowDefinition(
    const WindowInfo* optional_window_info,
    const NodeRef& node,
    bool expand_groups) {
  auto executor = MakeThreadAnyExecutor();
  return ToPromise(executor,
                   MakeWindowDefinitionAsync(executor, optional_window_info,
                                             node, expand_groups));
}

Awaitable<WindowDefinition> MakeWindowDefinitionAsync(
    AnyExecutor executor,
    const WindowInfo* optional_window_info,
    NodeRef node,
    bool expand_groups) {
  const auto& window_info =
      optional_window_info ? *optional_window_info : kDefaultWindowInfo;

  bool expand = expand_groups && node &&
                node.node_class() == scada::NodeClass::Object &&
                !window_info.single_item();

  NodeIdSet node_ids;
  if (expand) {
    node_ids = co_await ExpandGroupItemIdsAsync(executor, node);
  } else {
    node_ids = MakeNodeIdSet(node.node_id());
  }
  auto window_def = MakeEmptyWindowDefinition(&window_info, node);
  AddNodeIds(window_def, node_ids);
  co_return window_def;
}

promise<WindowDefinition> MakeWindowDefinition(
    const WindowInfo* window_info,
    const OpenContext& open_context) {
  auto executor = MakeThreadAnyExecutor();
  return ToPromise(
      executor,
      MakeWindowDefinitionAsync(executor, window_info, open_context));
}

Awaitable<WindowDefinition> MakeWindowDefinitionAsync(
    AnyExecutor executor,
    const WindowInfo* window_info,
    OpenContext open_context) {
  WindowDefinition window_def;
  if (open_context.node) {
    window_def = co_await MakeWindowDefinitionAsync(executor, window_info,
                                                    open_context.node, true);
  } else {
    window_def = MakeWindowDefinition(window_info, open_context.node_ids);
  }

  if (open_context.time_range.has_value()) {
    SaveTimeRange(window_def, *open_context.time_range);
  }

  co_return window_def;
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
  window_def.title = UtfConvert<char16_t>(formula);

  WindowItem& item = window_def.AddItem("Item");
  item.SetString("path", std::move(formula));

  return window_def;
}

promise<std::optional<WindowDefinition>> MakeGroupWindowDefinition(
    const WindowInfo* window_info,
    const NodeRef& node) {
  auto executor = MakeThreadAnyExecutor();
  return ToPromise(executor,
                   MakeGroupWindowDefinitionAsync(executor, window_info, node));
}

Awaitable<std::optional<WindowDefinition>> MakeGroupWindowDefinitionAsync(
    AnyExecutor executor,
    const WindowInfo* window_info,
    NodeRef node) {
  auto parent = node.parent();
  if (!IsInstanceOf(parent, data_items::id::DataGroupType))
    co_return std::optional<WindowDefinition>();

  auto node_ids = co_await ExpandGroupItemIdsAsync(executor, parent);
  co_return MakeWindowDefinition(window_info, node, node_ids);
}
