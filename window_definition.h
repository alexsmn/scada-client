#pragma once

#include "base/values.h"
#include "aui/size.h"

#include <filesystem>
#include <optional>
#include <vector>

struct WindowInfo;

class WindowItem {
 public:
  WindowItem() {}
  explicit WindowItem(std::string&& name);
  WindowItem(std::string&& name, base::Value&& attributes);

  WindowItem(const WindowItem& source);
  WindowItem& operator=(const WindowItem& source);

  WindowItem(WindowItem&& source) = default;
  WindowItem& operator=(WindowItem&& source) = default;

  bool name_is(std::string_view n) const;

  bool GetBool(std::string_view attr, bool default_value = false) const;
  int GetInt(std::string_view attr, int default_value = 0) const;
  std::string_view GetString(std::string_view attr,
                             std::string_view default_value = {}) const;
  std::u16string GetString16(std::string_view attr,
                             std::u16string_view default_value = {}) const;

  WindowItem& SetBool(std::string_view attr, bool value);
  WindowItem& SetInt(std::string_view attr, int value);
  WindowItem& SetString(std::string_view attr, std::string_view value);
  WindowItem& SetString(std::string_view attr, std::u16string_view value);

  template <class T>
  std::optional<T> Get() const;
  template <class T>
  WindowItem& Set(const T& value);
  WindowItem& Set(base::Value&& value);

  bool operator==(const WindowItem& other) const;

  std::string name;
  base::Value attributes{base::Value::Type::DICTIONARY};
};

typedef std::vector<WindowItem> WindowItems;

class WindowDefinition {
 public:
  explicit WindowDefinition(const WindowInfo& window_info);
  WindowDefinition(const WindowDefinition& other);
  ~WindowDefinition();

  WindowDefinition& operator=(const WindowDefinition& other);

  const WindowInfo& window_info() const {
    assert(window_info_);
    return *window_info_;
  }

  std::u16string GetTitle() const;

  WindowDefinition& AddItem(WindowItem&& window_item);
  template <class T>
  WindowDefinition& AddItem(std::string&& name, T&& value);
  WindowItem& AddItem(std::string name);

  WindowItem* FindItem(const char* name);
  const WindowItem* FindItem(const char* name) const;
  void Clear();

  template <class T>
  std::optional<T> Get(const char* name) const;
  template <class T>
  void Set(const char* name, const T& value);

  int id = 0;
  std::u16string title;
  std::filesystem::path path;
  aui::Size size;
  bool visible = true;
  bool locked = false;

  WindowItems items;

  base::Value storage;

  WindowDefinition& set_title(std::u16string_view title) {
    this->title = std::u16string{title};
    return *this;
  }

  bool operator==(const WindowDefinition& other) const;

 private:
  const WindowInfo* window_info_ = nullptr;
};

std::ostream& operator<<(std::ostream& stream, const WindowItem& window_item);

std::ostream& operator<<(std::ostream& stream,
                         const WindowDefinition& window_definition);

#include "window_definition_util.h"

template <class T>
inline WindowDefinition& WindowDefinition::AddItem(std::string&& name,
                                                   T&& value) {
  auto window_item = WindowItem{std::move(name)}.Set(std::forward<T>(value));
  AddItem(std::move(window_item));
  return *this;
}

template <class T>
inline std::optional<T> WindowItem::Get() const {
  return FromJson<T>(attributes);
}

template <class T>
inline WindowItem& WindowItem::Set(const T& value) {
  attributes = ToJson(value);
  return *this;
}

template <>
inline WindowItem& WindowItem::Set(const base::Value& value) {
  attributes = value.Clone();
  return *this;
}

inline WindowItem& WindowItem::Set(base::Value&& value) {
  attributes = std::move(value);
  return *this;
}

template <class T>
inline std::optional<T> WindowDefinition::Get(const char* name) const {
  auto* item = FindItem(name);
  return item ? item->Get<T>() : std::nullopt;
}

template <class T>
inline void WindowDefinition::Set(const char* name, const T& value) {
  AddItem(name).Set(value);
}
