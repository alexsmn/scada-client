#pragma once

#include "controls/models/edit_data.h"

#include <functional>

namespace aui {

class PropertyGroup {
 public:
  virtual ~PropertyGroup() {}

  // Category is colored differently.
  enum class ItemType { Property, Group, Category };

  virtual int GetCount() const = 0;
  virtual PropertyGroup* GetSubgroup(int index) const = 0;
  virtual std::u16string GetName(int index) const = 0;
  virtual std::u16string GetValue(int index) const = 0;
  virtual ItemType GetType(int index) const = 0;
  virtual bool IsInherited(int index) const = 0;
  virtual void SetValue(int index, const std::u16string& value) = 0;
  virtual aui::EditData GetEditData(int index) const = 0;
  virtual void HandleEditButton(int index) const = 0;
};

class PropertyModel {
 public:
  virtual ~PropertyModel() {}

  virtual PropertyGroup& GetRootGroup() = 0;

  using ModelChangedHandler = std::function<void()>;
  ModelChangedHandler model_changed_handler;

  using PropertiesChangedHandler =
      std::function<void(PropertyGroup& group, int first, int count)>;
  PropertiesChangedHandler properties_changed_handler;
};

}  // namespace aui
