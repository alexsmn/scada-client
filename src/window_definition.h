#pragma once

#include "base/files/file_path.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#include "base/format.h"
#include "ui/gfx/size.h"

#include <vector>

struct WindowInfo;

class WindowItem {
 public:
  WindowItem();
  WindowItem(const WindowItem& source);

  WindowItem& operator=(const WindowItem& source);

  const std::string& name() const { return name_; }
  bool name_is(const char* name) const { return _stricmp(name_.c_str(), name) == 0; }
  void set_name(const std::string& name) { name_ = name; }

  base::DictionaryValue& attributes() { return *attrs_; }
  const base::DictionaryValue& attributes() const { return *attrs_; }

  int GetInt(const std::string& attr, int default = 0) const {
    base::string16 s;
    if (!attrs_->GetString(attr, &s))
      return default;
    int v = 0;
    if (!base::StringToInt(s, &v))
      return default;
    return v;
  }

  std::string GetString(const std::string& attr,
                        const std::string& default = std::string()) const {
    base::string16 s;
    return attrs_->GetString(attr, &s) ? base::SysWideToNativeMB(s) : default;
  }

  base::string16 GetString16(const std::string& attr,
                             const base::string16& default = base::string16()) const {
    base::string16 s;
    return attrs_->GetString(attr, &s) ? s : default;
  }

  void SetInt(const std::string& attr, int default) {
    attrs_->SetString(attr, base::IntToString16(default));
  }

  void SetString(const std::string& attr, const std::string& value) {
    // Must convert to UTF, as we expect Unicode characters here.
    attrs_->SetString(attr, base::SysNativeMBToWide(value));
  }

  void SetString(const std::string& attr, const base::string16& value) {
    attrs_->SetString(attr, value);
  }

 private:
  std::string name_;
  std::unique_ptr<base::DictionaryValue> attrs_;
};

typedef std::vector<WindowItem> WindowItems;

class WindowDefinition {
 public:
  explicit WindowDefinition(const WindowInfo& window_info);
  WindowDefinition(const WindowDefinition& other);
  ~WindowDefinition();

  WindowDefinition& operator=(const WindowDefinition& other);

  const WindowInfo& window_info() const { assert(window_info_); return *window_info_; }
  base::string16 GetTitle() const;
  
  WindowItem& AddItem(const char* name);
  WindowItem* FindItem(const char* name);
  const WindowItem* FindItem(const char* name) const;
  void Clear();

//  const base::Value* storage() const { return storage_.get(); }
//  void SetStorage(base::Value* storage);

  int id = 0;
  base::string16 title;
  base::FilePath path;
  gfx::Size size;
  bool visible = true;
  bool locked = false;

  WindowItems	items;

  base::Value* storage() { return storage_.get(); }
  const base::Value* storage() const { return storage_.get(); }
  void set_storage(std::unique_ptr<base::Value> storage) { storage_ = std::move(storage); }

 private:
  const WindowInfo* window_info_ = nullptr;

  std::unique_ptr<base::Value> storage_;
};
