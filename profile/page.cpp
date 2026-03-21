#include "profile/page.h"

#include <cassert>

#include "base/base64.h"
#include "base/files/file_util.h"
#include "base/json/json_string_value_serializer.h"
#include "base/stl_util.h"
#include "base/utils.h"
#include "base/value_util.h"
#include "controller/window_info.h"
#include "profile/page_layout.h"
#include "profile/window_definition_util.h"

namespace {
const size_t kMaxTitleLength = 30;
}

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
      auto w = FromJson<WindowDefinition>(win);
      if (!w.has_value())
        continue;

      windows_.emplace_back(std::make_unique<WindowDefinition>(std::move(*w)));
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
  if (const auto* layoute = FindDict(data, "layout")) {
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
  for (int i = 0; i < GetWindowCount(); ++i)
    windows.emplace_back(ToJson(GetWindow(i)));
  result.SetKey("windows", base::Value{std::move(windows)});

  // layout
  result.SetKey("layout", ToJson(layout));

  return result;
}

std::u16string Page::GetTitle() const {
  if (!title.empty())
    return title;

  std::u16string title;
  for (int i = 0; i < GetWindowCount(); ++i) {
    WindowDefinition& win = GetWindow(i);
    if (!title.empty())
      title += u", ";
    title += win.title;
    if (title.length() >= kMaxTitleLength)
      break;
  }

  if (title.length() > kMaxTitleLength) {
    title.erase(kMaxTitleLength, title.length() - kMaxTitleLength);
    title += u"...";
  }

  if (title.empty())
    title = u"(Пустой)";

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
  return i != windows_.end() ? static_cast<int>(i - windows_.begin()) : -1;
}

void Page::DeleteWindow(int index) {
  assert(index >= 0);
  windows_.erase(windows_.begin() + index);
}

void Page::Clear() {
  windows_.clear();
}
