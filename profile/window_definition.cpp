#include "profile/window_definition.h"

#include "base/format.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/struct_writer.h"
#include "base/value_util.h"
#include "base/values.h"
#include "controller/window_info.h"

#include "base/debug_util-inl.h"

std::ostream& operator<<(std::ostream& stream, const WindowItem& window_item) {
  StructWriter{stream}
      .AddField("name", window_item.name)
      .AddField("attributes", window_item.attributes);
  return stream;
}

std::ostream& operator<<(std::ostream& stream,
                         const WindowDefinition& window_definition) {
  StructWriter{stream}
      .AddField("window_info.name", window_definition.window_info().name)
      .AddField("id", window_definition.id)
      .AddField("title", window_definition.title)
      .AddField("path", window_definition.path)
      // TODO: Proper fix.
      // .AddField("size", window_definition.size)
      .AddField("visible", window_definition.visible)
      .AddField("locked", window_definition.locked)
      .AddField("items", window_definition.items)
      .AddField("storage", window_definition.storage);
  return stream;
}

// WindowItem

WindowItem::WindowItem(std::string&& name) : name{std::move(name)} {}

WindowItem::WindowItem(std::string&& name, base::Value&& attributes)
    : name{std::move(name)}, attributes{std::move(attributes)} {}

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
  return base::EqualsCaseInsensitiveASCII(name, n);
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

std::u16string WindowItem::GetString16(
    std::string_view attr,
    std::u16string_view default_value) const {
  return ::GetString16(attributes, attr, default_value);
}

WindowItem& WindowItem::SetBool(std::string_view attr, bool value) {
  SetKey(attributes, attr, value);
  return *this;
}

WindowItem& WindowItem::SetInt(std::string_view attr, int value) {
  SetKey(attributes, attr, value);
  return *this;
}

WindowItem& WindowItem::SetString(std::string_view attr,
                                  std::string_view value) {
  SetKey(attributes, attr, value);
  return *this;
}

WindowItem& WindowItem::SetString(std::string_view attr,
                                  std::u16string_view value) {
  SetKey(attributes, attr, value);
  return *this;
}

bool WindowItem::operator==(const WindowItem& other) const {
  return name == other.name && attributes == other.attributes;
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

std::u16string WindowDefinition::GetTitle() const {
  if (!title.empty())
    return title;

  std::u16string title{window_info().title};
  if (!path.empty())
    title += u": " + path.u16string();
  return title;
}

WindowDefinition& WindowDefinition::AddItem(WindowItem&& window_item) {
  items.emplace_back(std::move(window_item));
  return *this;
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

bool WindowDefinition::operator==(const WindowDefinition& other) const {
  return window_info_ == other.window_info_ && id == other.id &&
         title == other.title && path == other.path && size == other.size &&
         visible == other.visible && locked == other.locked &&
         items == other.items && storage == other.storage;
}
