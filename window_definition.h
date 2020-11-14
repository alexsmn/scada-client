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

  bool name_is(base::StringPiece n) const;

  bool GetBool(base::StringPiece attr, bool default = false) const;
  int GetInt(base::StringPiece attr, int default = 0) const;
  base::StringPiece GetString(base::StringPiece attr,
                              base::StringPiece default = {}) const;
  std::wstring GetString16(base::StringPiece attr,
                             base::StringPiece16 default = {}) const;

  void SetBool(base::StringPiece attr, bool value);
  void SetInt(base::StringPiece attr, int value);
  void SetString(base::StringPiece attr, base::StringPiece value);
  void SetString(base::StringPiece attr, base::StringPiece16 value);

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

  std::unique_ptr<base::Value> storage_;
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
