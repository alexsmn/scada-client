#pragma once

#include "base/strings/string16.h"
#include "ui/base/models/edit_data.h"

#include <functional>

class PropertyModel {
 public:
  virtual ~PropertyModel() {}

  virtual int GetCount() = 0;
  virtual base::string16 GetName(int index) = 0;
  virtual base::string16 GetValue(int index) = 0;
  virtual bool IsInherited(int index) = 0;
  virtual void SetValue(int index, const base::string16& value) = 0;
  virtual ui::EditData GetEditData(int index) = 0;

  using PropertiesChangedHandler = std::function<void(int first, int count)>;
  PropertiesChangedHandler properties_changed_handler;
};
