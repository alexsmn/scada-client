#pragma once

#include "aui/models/property_model.h"
#include "scada/attribute_ids.h"
#include "scada/node_id.h"

class NodePropertyModel;
class PropertyDefinition;

class NodeGroupModel : public aui::PropertyGroup {
 public:
  explicit NodeGroupModel(NodePropertyModel& property_model);
  ~NodeGroupModel();

  virtual int GetCount() const override;
  virtual PropertyGroup* GetSubgroup(int index) const override;
  virtual std::u16string GetName(int index) const override;
  virtual std::u16string GetValue(int index) const override;
  virtual ItemType GetType(int index) const override;
  virtual bool IsInherited(int index) const override;
  virtual void SetValue(int index, const std::u16string& value) override;
  virtual aui::EditData GetEditData(int index) const override;
  virtual void HandleEditButton(int index) const override;

  struct Property {
    ItemType type;
    std::u16string name;
    scada::AttributeId attribute_id;
    const PropertyDefinition* def = nullptr;
    scada::NodeId prop_decl_id;
    std::unique_ptr<NodeGroupModel> submodel;
    std::vector<scada::NodeId> additional_targets;
  };

  std::u16string group_title;
  std::vector<Property> properties;

 private:
  NodePropertyModel& property_model_;
};
