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
#include "services/page_layout.h"
#include "value_util.h"
#include "window_definition_util.h"
#include "window_info.h"

#define MAX_TITLE 30

namespace {
  
const base::char16 kPageFileExtension[] = L"page";

void LoadWinItems(WindowItems& items, const base::Value& data) {
  if (!data.is_list())
    return;

  for (auto& item_data : data.GetList()) {
    auto& item = items.emplace_back();
    item.name = GetString(item_data, "name").as_string();
    item.attributes = item_data.Clone();
    item.attributes.RemoveKey("name");
  }
}

base::Value SaveWinItems(const WindowItems& items) {
  base::Value::ListStorage list;
  list.reserve(items.size());
  for (const auto& item : items) {
    assert(item.attributes.is_dict());
    assert(!item.attributes.FindKey("name"));
    auto item_data = item.attributes.Clone();
    SetKey(item_data, "name", item.name);
    list.emplace_back(std::move(item_data));
  }
  return base::Value{std::move(list)};
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
      if (auto* items = win.FindKey("items"))
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
  if (const auto* layoute = GetDict(data, "layout")) {
    if (auto l = FromJson<PageLayout>(*layoute))
      layout = std::move(*l);
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
  result.SetKey("layout", ToJson(layout));

  return result;
}

std::wstring Page::GetTitle() const {
  if (!title.empty())
    return title;

  std::wstring title;
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
