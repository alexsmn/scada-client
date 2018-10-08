#include "services/page.h"

#include "base/base64.h"
#include "base/files/file_util.h"
#include "base/json/json_string_value_serializer.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/utils.h"
#include "value_util.h"
#include "window_definition_util.h"
#include "window_info.h"

#define MAX_TITLE 30

namespace {

static const base::char16 kPageFileExtension[] = L"page";

void LoadWinItems(WindowItems& items, const base::Value& data) {
  if (!data.is_dict())
    return;

  for (auto& [key, value] : data.DictItems()) {
    auto& item = items.emplace_back();
    item.name = key;
    item.attributes = value.Clone();
  }
}

base::Value SaveWinItems(const WindowItems& items) {
  base::Value data{base::Value::Type::DICTIONARY};
  for (const auto& item : items)
    data.SetKey(item.name, item.attributes.Clone());
  return data;
}

void LoadLayoutBlock(PageLayoutBlock& block, const base::Value& value) {
  assert(block.type == PageLayoutBlock::PANE);
  assert(block.wins.empty());
  assert(!block.left && !block.right);

  auto type = GetString(value, "type");
  if (base::EqualsCaseInsensitiveASCII(type, "split")) {
    auto orientation = GetString(value, "orientation");
    bool horz = base::EqualsCaseInsensitiveASCII(orientation, "horizontal");
    block.split(horz);
    block.pos = GetInt(value, "pos", -1);
    if (auto* pane = GetDict(value, "first"))
      LoadLayoutBlock(*block.left, *pane);
    if (auto* pane = GetDict(value, "second"))
      LoadLayoutBlock(*block.right, *pane);

  } else if (base::EqualsCaseInsensitiveASCII(type, "pane")) {
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

const char* dock_names[4] = {"bottom", "top", "left", "right"};

base::Value SaveLayoutBlock(const PageLayoutBlock& block) {
  base::Value result{base::Value::Type::DICTIONARY};
  SetKey(result, "type",
         block.type == PageLayoutBlock::PANE ? "pane" : "split");

  if (block.type == PageLayoutBlock::SPLIT) {
    SetKey(result, "orientation", block.horz ? L"horizontal" : L"vertical");
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

}  // namespace

// Page

Page::Page(const Page& source)
    : id{source.id}, title{source.title}, layout{source.layout} {
  windows_.reserve(source.windows_.size());
  for (auto& w : source.windows_)
    windows_.emplace_back(std::make_unique<WindowDefinition>(*w));
}

Page& Page::operator=(const Page& source) {
  if (&source != this) {
    id = source.id;
    title = source.title;
    layout = source.layout;

    windows_.clear();
    windows_.reserve(source.windows_.size());
    for (auto& w : source.windows_)
      windows_.emplace_back(std::make_unique<WindowDefinition>(*w));
  }

  return *this;
}

void Page::Load(const base::Value& data) {
  id = GetInt(data, "id");
  title = GetString16(data, "title");

  if (const auto* winse = GetList(data, "windows")) {
    for (auto& win : *winse) {
      UINT type = ParseWindowType(GetString(win, "type"));
      const WindowInfo* info = FindWindowInfo(type);
      if (!info)
        continue;

      // check single window
      bool found = false;
      if (info->is_pane()) {
        for (int i = 0; i < GetWindowCount(); ++i)
          if (&GetWindow(i).window_info() == info) {
            found = true;
            break;
          }
      }
      if (found) {
        assert(false);
        // don't load window
        continue;
      }

      auto w = std::make_unique<WindowDefinition>(*info);
      w->id = GetInt(win, "id", 0);
      if (info->flags & WIN_SING)
        w->visible = GetBool(win, "visible", true);
      w->title = GetString16(win, "title");
      w->path = base::FilePath(GetString16(win, "path"));
      w->size = gfx::Size(GetInt(win, "width"), GetInt(win, "height"));
      if (auto* items = GetDict(win, "items"))
        LoadWinItems(w->items, *items);

      if (auto* data = GetDict(win, "data"))
        w->storage = data->Clone();

      windows_.emplace_back(std::move(w));
    }
  }

  // fix page ids
  for (int i = 0; i < GetWindowCount(); ++i) {
    WindowDefinition& window = GetWindow(i);
    if (!window.id) {
      window.id = NewWindowId();
      continue;
    }
    for (int j = i + 1; j < GetWindowCount(); ++j)
      if (GetWindow(j).id == window.id)
        GetWindow(j).id = NewWindowId();
  }

  // layout
  if (const auto* layoute = GetDict(data, "Layout")) {
    if (const auto* maine = GetDict(*layoute, "Center"))
      LoadLayoutBlock(layout.main, *maine);

    for (int i = 0; i < 4; i++) {
      const auto* docke = GetDict(*layoute, dock_names[i]);
      if (!docke)
        continue;

      auto& dock = layout.dock[i];
      dock.size = GetInt(*docke, "size", 0);
      dock.place = GetInt(*docke, "place", 0);
      LoadLayoutBlock(dock, *docke);
    }

    layout.blob = RestoreBlob(GetString(*layoute, "blob"));
  }
}

base::Value Page::Save(bool current) const {
  base::Value result{base::Value::Type::DICTIONARY};

  // page attributes
  if (id)
    SetKey(result, "id", id);
  if (!title.empty())
    SetKey(result, "title", title);

  base::Value::ListStorage windows;
  windows.reserve(GetWindowCount());
  for (int i = 0; i < GetWindowCount(); ++i) {
    WindowDefinition& def = GetWindow(i);
    const WindowInfo& window_info = def.window_info();

    base::Value win{base::Value::Type::DICTIONARY};
    SetKey(win, "type", window_info.name);
    SetKey(win, "id", def.id);
    if (!def.visible) {
      assert(window_info.flags & WIN_SING);
      SetKey(win, "visible", def.visible);
    }
    if (!def.title.empty())
      SetKey(win, "title", def.title);
    if (!def.path.empty())
      SetKey(win, "path", def.path.value());
    SetKey(win, "width", def.size.width());
    SetKey(win, "height", def.size.height());
    SetKey(win, "locked", def.locked);

    win.SetKey("items", SaveWinItems(def.items));
    win.SetKey("data", def.storage.Clone());

    windows.emplace_back(std::move(win));
  }
  result.SetKey("windows", base::Value{std::move(windows)});

  // layout
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
    layout_data.SetKey(dock_names[i], std::move(dock_data));
  }
  if (!layout.blob.empty())
    SetKey(layout_data, "blob", SaveBlob(layout.blob));
  result.SetKey("layout", std::move(layout_data));

  return result;
}

base::string16 Page::GetTitle() const {
  if (!title.empty())
    return title;

  base::string16 title;
  for (int i = 0; i < GetWindowCount(); ++i) {
    WindowDefinition& win = GetWindow(i);
    if (!title.empty())
      title += L", ";
    title += win.title;
    if (title.length() >= MAX_TITLE)
      break;
  }

  if (title.length() > MAX_TITLE) {
    title.erase(MAX_TITLE, title.length() - MAX_TITLE);
    title += L"...";
  }

  if (title.empty())
    title = L"(Пустой)";

  return title;
}

int Page::NewWindowId() {
  static int _id = 1;
  while (FindWindowDef(_id))
    _id++;
  return _id++;
}

WindowDefinition& Page::AddWindow(const WindowDefinition& window) {
  auto w = std::make_unique<WindowDefinition>(window);
  w->id = NewWindowId();
  return *windows_.emplace_back(std::move(w));
}

WindowDefinition* Page::FindWindowDef(int id) {
  for (int i = 0; i < GetWindowCount(); ++i) {
    WindowDefinition& win = GetWindow(i);
    if (win.id == id)
      return &win;
  }
  return NULL;
}

const WindowDefinition* Page::FindWindowDef(int id) const {
  return const_cast<Page*>(this)->FindWindowDef(id);
}

int Page::FindWindowDef(const WindowDefinition& window) const {
  auto i = std::find_if(windows_.begin(), windows_.end(),
                        [&window](auto& p) { return p.get() == &window; });
  return i != windows_.end() ? i - windows_.begin() : -1;
}

void Page::DeleteWindow(int index) {
  DCHECK_GE(index, 0);
  windows_.erase(windows_.begin() + index);
}

void Page::Clear() {
  windows_.clear();
}
