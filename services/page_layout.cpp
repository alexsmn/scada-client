#include "services/page_layout.h"

#include "base/string_piece_util.h"
#include "base/strings/string_util.h"
#include "base/value_util.h"
#include "controller/window_definition_util.h"

namespace {
const char* kDockNames[4] = {"bottom", "top", "left", "right"};
}

base::Value SaveLayoutBlock(const PageLayoutBlock& block) {
  base::Value result{base::Value::Type::DICTIONARY};
  SetKey(result, "type",
         block.type == PageLayoutBlock::PANE ? "pane" : "split");

  if (block.type == PageLayoutBlock::SPLIT) {
    SetKey(result, "orientation", block.horz ? "horizontal" : "vertical");
    SetKey(result, "pos", block.pos);
    result.SetKey("first", SaveLayoutBlock(*block.left));
    result.SetKey("second", SaveLayoutBlock(*block.right));

  } else {
    base::Value::ListStorage list;
    list.reserve(block.wins.size());
    for (auto window_id : block.wins)
      list.emplace_back(base::Value{window_id});
    result.SetKey("windows", base::Value{std::move(list)});

    if (block.active_window != -1)
      SetKey(result, "active", block.active_window);
  }

  if (block.central)
    SetKey(result, "central", true);

  return result;
}

void LoadLayoutBlock(PageLayoutBlock& block, const base::Value& value) {
  assert(block.type == PageLayoutBlock::PANE);
  assert(block.wins.empty());
  assert(!block.left && !block.right);

  auto type = GetString(value, "type");
  if (base::EqualsCaseInsensitiveASCII(AsStringPiece(type), "split")) {
    auto orientation = GetString(value, "orientation");
    bool horz = base::EqualsCaseInsensitiveASCII(AsStringPiece(orientation),
                                                 "horizontal");
    block.split(horz);
    block.pos = GetInt(value, "pos", -1);
    if (auto* pane = FindDict(value, "first"))
      LoadLayoutBlock(*block.left, *pane);
    if (auto* pane = FindDict(value, "second"))
      LoadLayoutBlock(*block.right, *pane);

  } else if (base::EqualsCaseInsensitiveASCII(AsStringPiece(type), "pane")) {
    assert(block.type == PageLayoutBlock::PANE);
    if (auto* windows = GetList(value, "windows")) {
      for (auto& window : *windows) {
        if (window.is_int())
          block.wins.push_back(window.GetInt());
      }
    }
    block.active_window = GetInt(value, "active", -1);
  }

  block.central = GetBool(value, "central");
}

base::Value ToJson(const PageLayout& layout) {
  base::Value layout_data{base::Value::Type::DICTIONARY};
  if (!layout.main.empty())
    layout_data.SetKey("center", SaveLayoutBlock(layout.main));
  for (int i = 0; i < 4; i++) {
    auto& dock = layout.dock[i];
    if (dock.empty())
      continue;

    auto dock_data = SaveLayoutBlock(dock);
    SetKey(dock_data, "size", dock.size);
    SetKey(dock_data, "place", dock.place);
    layout_data.SetKey(kDockNames[i], std::move(dock_data));
  }
  if (!layout.blob.empty())
    SetKey(layout_data, "blob", SaveBlob(layout.blob));
  return layout_data;
}

template <>
std::optional<PageLayout> FromJson(const base::Value& json) {
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