#pragma once

#include "base/strings/string16.h"
#include "ui/base/models/edit_data.h"

#include <functional>

class PropertyGroup {
 public:
  virtual ~PropertyGroup() {}

  virtual int GetCount() const = 0;
  virtual PropertyGroup* GetSubgroup(int index) const = 0;
  virtual base::string16 GetName(int index) const = 0;
  virtual base::string16 GetValue(int index) const = 0;
  virtual bool IsInherited(int index) const = 0;
  virtual void SetValue(int index, const base::string16& value) = 0;
  virtual ui::EditData GetEditData(int index) const = 0;
};

class PropertyModel {
 public:
  virtual ~PropertyModel() {}

  virtual PropertyGroup& GetRootGroup() = 0;

  using PropertiesChangedHandler = std::function<void(PropertyGroup& group, int first, int count)>;
  PropertiesChangedHandler properties_changed_handler;
};
