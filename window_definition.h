#pragma once

#include "base/files/file_path.h"
#include "base/format.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#include "ui/gfx/size.h"

#include <optional>
#include <vector>

struct WindowInfo;

class WindowItem {
 public:
  WindowItem() {}

  WindowItem(const WindowItem& source);
  WindowItem& operator=(const WindowItem& source);

  WindowItem(WindowItem&& source) = default;
  WindowItem& operator=(WindowItem&& source) = default;

  bool name_is(std::string_view n) const;

  bool GetBool(std::string_view attr, bool default_value = false) const;
  int GetInt(std::string_view attr, int default_value = 0) const;
  std::string_view GetString(std::string_view attr,
                             std::string_view default_value = {}) const;
  std::wstring GetString16(std::string_view attr,
                           std::wstring_view default_value = {}) const;

  void SetBool(std::string_view attr, bool value);
  void SetInt(std::string_view attr, int value);
  void SetString(std::string_view attr, std::string_view value);
  void SetString(std::string_view attr, std::wstring_view value);

  template <class T>
  std::optional<T> Get() const;
  template <class T>
  void Set(const T& value);

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

  std::wstring GetTitle() const;

  WindowItem& AddItem(std::string name);
  WindowItem* FindItem(const char* name);
  const WindowItem* FindItem(const char* name) const;
  void Clear();

  template <class T>
  std::optional<T> Get(const char* name) const;
  template <class T>
  void Set(const char* name, const T& value);

  int id = 0;
  std::wstring title;
  base::FilePath path;
  gfx::Size size;
  bool visible = true;
  bool locked = false;

  WindowItems items;

  base::Value storage;

 private:
  const WindowInfo* window_info_ = nullptr;
};

#include "window_definition_util.h"

template <class T>
inline std::optional<T> WindowItem::Get() const {
  return FromJson<T>(attributes);
}

template <class T>
inline void WindowItem::Set(const T& value) {
  attributes = ToJson(value);
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
