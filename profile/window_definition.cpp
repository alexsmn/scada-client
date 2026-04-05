#include "profile/window_definition.h"

#include "base/format.h"
#include <boost/algorithm/string/predicate.hpp>
#include "base/struct_writer.h"
#include "base/value_util.h"
#include "controller/window_info.h"

#include "base/debug_util.h"

std::ostream& operator<<(std::ostream& stream, const WindowItem& window_item) {
  StructWriter{stream}
      .AddField("name", window_item.name)
      .AddField("attributes", window_item.attributes);
  return stream;
}

std::ostream& operator<<(std::ostream& stream,
                         const WindowDefinition& window_definition) {
  StructWriter{stream}
      .AddField("type", window_definition.type)
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

WindowItem::WindowItem(std::string&& name, boost::json::value&& attributes)
    : name{std::move(name)}, attributes{std::move(attributes)} {}

WindowItem::WindowItem(const WindowItem& source)
    : name{source.name}, attributes{source.attributes} {}

WindowItem& WindowItem::operator=(const WindowItem& source) {
  if (&source != this) {
    name = source.name;
    attributes = source.attributes;
  }
  return *this;
}

bool WindowItem::name_is(std::string_view n) const {
  return boost::iequals(name, n);
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
    : WindowDefinition{window_info.name} {}

WindowDefinition::WindowDefinition(const WindowDefinition& other)
    : id(other.id),
      type(other.type),
      title(other.title),
      path(other.path),
      size(other.size),
      visible(other.visible),
      locked(other.locked),
      items(other.items),
      storage(other.storage) {}

WindowDefinition::~WindowDefinition() {}

WindowDefinition& WindowDefinition::operator=(const WindowDefinition& other) {
  id = other.id;
  type = other.type;
  title = other.title;
  path = other.path;
  size = other.size;
  visible = other.visible;
  locked = other.locked;
  items = other.items;
  storage = other.storage;
  return *this;
}

std::u16string WindowDefinition::GetTitle(const WindowInfo& window_info) const {
  if (!title.empty())
    return title;

  std::u16string title{window_info.title};
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
  for (WindowItems::iterator i = items.begin(); i != items.end(); ++i) {
    WindowItem& item = *i;
    if (item.name_is(name))
      return &item;
  }
  return nullptr;
}

const WindowItem* WindowDefinition::FindItem(const char* name) const {
  for (WindowItems::const_iterator i = items.begin(); i != items.end(); ++i) {
    const WindowItem& item = *i;
    if (item.name_is(name))
      return &item;
  }
  return nullptr;
}

void WindowDefinition::Clear() {
  items.clear();
}
