#include "profile/page_layout.h"

#include <boost/algorithm/string/predicate.hpp>
#include "base/value_util.h"
#include "profile/window_definition_util.h"

namespace {
const char* kDockNames[4] = {"bottom", "top", "left", "right"};
}

boost::json::value SaveLayoutBlock(const PageLayoutBlock& block) {
  boost::json::value result{boost::json::object{}};
  SetKey(result, "type",
         block.type == PageLayoutBlock::PANE ? "pane" : "split");

  if (block.type == PageLayoutBlock::SPLIT) {
    SetKey(result, "orientation", block.horz ? "horizontal" : "vertical");
    SetKey(result, "pos", block.pos);
    result.as_object()["first"] = SaveLayoutBlock(*block.left);
    result.as_object()["second"] = SaveLayoutBlock(*block.right);

  } else {
    boost::json::array list;
    list.reserve(block.wins.size());
    for (auto window_id : block.wins)
      list.emplace_back(window_id);
    result.as_object()["windows"] = std::move(list);

    if (block.active_window != -1)
      SetKey(result, "active", block.active_window);
  }

  if (block.central)
    SetKey(result, "central", true);

  return result;
}

void LoadLayoutBlock(PageLayoutBlock& block, const boost::json::value& value) {
  assert(block.type == PageLayoutBlock::PANE);
  assert(block.wins.empty());
  assert(!block.left && !block.right);

  auto type = GetString(value, "type");
  if (boost::iequals(type, "split")) {
    auto orientation = GetString(value, "orientation");
    bool horz = boost::iequals(orientation, "horizontal");
    block.split(horz);
    block.pos = GetInt(value, "pos", -1);
    if (auto* pane = FindDict(value, "first"))
      LoadLayoutBlock(*block.left, *pane);
    if (auto* pane = FindDict(value, "second"))
      LoadLayoutBlock(*block.right, *pane);

  } else if (boost::iequals(type, "pane")) {
    assert(block.type == PageLayoutBlock::PANE);
    if (auto* windows = GetList(value, "windows")) {
      for (auto& window : *windows) {
        if (window.is_int64())
          block.wins.push_back(static_cast<int>(window.as_int64()));
      }
    }
    block.active_window = GetInt(value, "active", -1);
  }

  block.central = GetBool(value, "central");
}

boost::json::value ToJson(const PageLayout& layout) {
  boost::json::value layout_data{boost::json::object{}};
  if (!layout.main.empty())
    layout_data.as_object()["center"] = SaveLayoutBlock(layout.main);
  for (int i = 0; i < 4; i++) {
    auto& dock = layout.dock[i];
    if (dock.empty())
      continue;

    auto dock_data = SaveLayoutBlock(dock);
    SetKey(dock_data, "size", dock.size);
    SetKey(dock_data, "place", dock.place);
    layout_data.as_object()[kDockNames[i]] = std::move(dock_data);
  }
  if (!layout.blob.empty())
    SetKey(layout_data, "blob", SaveBlob(layout.blob));
  return layout_data;
}

template <>
std::optional<PageLayout> FromJson(const boost::json::value& json) {
  PageLayout layout;

  if (const auto* maine = FindDict(json, "center"))
    LoadLayoutBlock(layout.main, *maine);

  for (int i = 0; i < 4; i++) {
    const auto* docke = FindDict(json, kDockNames[i]);
    if (!docke)
      continue;

    auto& dock = layout.dock[i];
    dock.size = GetInt(*docke, "size", 0);
    dock.place = GetInt(*docke, "place", 0);
    LoadLayoutBlock(dock, *docke);
  }

  layout.blob = RestoreBlob(GetString(json, "blob"));
  return layout;
}