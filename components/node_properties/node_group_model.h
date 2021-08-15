#pragma once

#include "controls/property_model.h"
#include "core/attribute_ids.h"
#include "core/node_id.h"

class NodePropertyModel;
class PropertyDefinition;

class NodeGroupModel : public PropertyGroup {
 public:
  explicit NodeGroupModel(NodePropertyModel& property_model);
  ~NodeGroupModel();

  virtual int GetCount() const override;
  virtual PropertyGroup* GetSubgroup(int index) const override;
  virtual std::wstring GetName(int index) const override;
  virtual std::wstring GetValue(int index) const override;
  virtual ItemType GetType(int index) const override;
  virtual bool IsInherited(int index) const override;
  virtual void SetValue(int index, const std::wstring& value) override;
  virtual ui::EditData GetEditData(int index) const override;

  struct Property {
    ItemType type;
    std::wstring name;
    scada::AttributeId attribute_id;
    const PropertyDefinition* def;
    scada::NodeId prop_decl_id;
    std::unique_ptr<NodeGroupModel> submodel;
  };

  std::wstring group_title;
  std::vector<Property> properties;

 private:
  NodePropertyModel& property_model_;
};
