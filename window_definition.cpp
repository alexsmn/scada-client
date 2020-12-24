#include "window_definition.h"

#include "base/string_piece_util.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "value_util.h"
#include "window_info.h"

WindowItem::WindowItem(const WindowItem& source)
    : name{source.name}, attributes{source.attributes.Clone()} {}

WindowItem& WindowItem::operator=(const WindowItem& source) {
  if (&source != this) {
    name = source.name;
    attributes = source.attributes.Clone();
  }
  return *this;
}

bool WindowItem::name_is(std::string_view n) const {
  return base::EqualsCaseInsensitiveASCII(name, ToStringPiece(n));
}

bool WindowItem::GetBool(std::string_view attr, bool default_value) const {
  return ::GetBool(attributes, attr, default_value);
}

int WindowItem::GetInt(std::string_view attr, int default_value) const {
  return ::GetInt(attributes, attr, default_value);
}

std::string_view WindowItem::GetString(std::string_view attr,
                                       std::string_view default_value) const {
  return ::GetString(attributes, attr, default_value);
}

std::wstring WindowItem::GetString16(std::string_view attr,
                                     std::wstring_view default_value) const {
  return ::GetString16(attributes, attr, default_value);
}

void WindowItem::SetBool(std::string_view attr, bool value) {
  SetKey(attributes, attr, value);
}

void WindowItem::SetInt(std::string_view attr, int value) {
  SetKey(attributes, attr, value);
}

void WindowItem::SetString(std::string_view attr, std::string_view value) {
  SetKey(attributes, attr, value);
}

void WindowItem::SetString(std::string_view attr, std::wstring_view value) {
  SetKey(attributes, attr, base::UTF16ToUTF8(ToStringPiece(value)));
}

// WindowDefinition

WindowDefinition::WindowDefinition(const WindowInfo& window_info)
    : window_info_(&window_info) {}

WindowDefinition::WindowDefinition(const WindowDefinition& other)
    : window_info_(other.window_info_),
      id(other.id),
      title(other.title),
      path(other.path),
      size(other.size),
      visible(other.visible),
      locked(other.locked),
      items(other.items),
      storage(other.storage.Clone()) {}

WindowDefinition::~WindowDefinition() {}

WindowDefinition& WindowDefinition::operator=(const WindowDefinition& other) {
  window_info_ = other.window_info_;
  id = other.id;
  title = other.title;
  path = other.path;
  size = other.size;
  visible = other.visible;
  locked = other.locked;
  items = other.items;
  storage = other.storage.Clone();
  return *this;
}

std::wstring WindowDefinition::GetTitle() const {
  if (!title.empty())
    return title;

  std::wstring title = window_info().title;
  if (!path.empty())
    title += L": " + path.value();
  return title;
}

WindowItem& WindowDefinition::AddItem(std::string name) {
  WindowItem& item = items.emplace_back();
  item.name = std::move(name);
  return item;
}

WindowItem* WindowDefinition::FindItem(const char* name) {
  for (WindowItems::iterator i = items.begin(); i != items.end(); i++) {
    WindowItem& item = *i;
    if (item.name_is(name))
      return &item;
  }
  return NULL;
}

const WindowItem* WindowDefinition::FindItem(const char* name) const {
  for (WindowItems::const_iterator i = items.begin(); i != items.end(); i++) {
    const WindowItem& item = *i;
    if (item.name_is(name))
      return &item;
  }
  return NULL;
}

void WindowDefinition::Clear() {
  items.clear();
}
