#include "base/values.h"
#include "window_info.h"
#include "window_definition.h"

WindowItem::WindowItem()
    : attrs_(new base::DictionaryValue) {
}

WindowItem::WindowItem(const WindowItem& source)
    : name_(source.name_),
      attrs_(source.attrs_->DeepCopy()) {
}

WindowItem& WindowItem::operator=(const WindowItem& source) {
  name_ = source.name_;
  attrs_.reset(source.attrs_->DeepCopy());
  return *this;
}

WindowDefinition::WindowDefinition(const WindowInfo& window_info)
    : window_info_(&window_info) {
}

WindowDefinition::WindowDefinition(const WindowDefinition& other)
    : window_info_(other.window_info_),
      id(other.id),
      title(other.title),
      path(other.path),
      size(other.size),
      visible(other.visible),
      locked(other.locked),
      items(other.items),
      storage_(other.storage_ ? other.storage_->DeepCopy() : NULL) {
}

WindowDefinition::~WindowDefinition() {
}

WindowDefinition& WindowDefinition::operator=(const WindowDefinition& other) {
  window_info_ = other.window_info_;
  id = other.id;
  title = other.title;
  path = other.path;
  size = other.size;
  visible = other.visible;
  locked = other.locked;
  items = other.items;
  storage_.reset(other.storage_ ? other.storage_->DeepCopy() : NULL);
  return *this;
}

base::string16 WindowDefinition::GetTitle() const {
  if (!title.empty())
    return title;

  base::string16 title = window_info().title;
  if (!path.empty())
    title += L": " + path.value();
  return title;
}

WindowItem& WindowDefinition::AddItem(const char* name) {
  items.push_back(WindowItem());
  WindowItem& item = items.back();
  item.set_name(name);
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

/*void WindowDefinition::SetStorage(base::Value* storage) {
  storage_.reset(storage);
}*/
