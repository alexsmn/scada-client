#include "components/node_properties/node_group_model.h"

#include "components/node_properties/node_property_model.h"
#include "services/property_defs.h"
#include "services/task_manager.h"

NodeGroupModel::NodeGroupModel(NodePropertyModel& property_model)
    : property_model_{property_model} {}

NodeGroupModel::~NodeGroupModel() {}

PropertyGroup* NodeGroupModel::GetSubgroup(int index) const {
  return properties[index].submodel.get();
}

std::wstring NodeGroupModel::GetName(int index) const {
  return properties[index].name;
}

std::wstring NodeGroupModel::GetValue(int index) const {
  auto& prop = properties[index];
  if (prop.def)
    return prop.def->GetText(property_model_, property_model_.node_,
                             prop.prop_decl_id);
  else
    return ToString16(property_model_.node_.attribute(prop.attribute_id));
}

PropertyGroup::ItemType NodeGroupModel::GetType(int index) const {
  return properties[index].type;
}

bool NodeGroupModel::IsInherited(int index) const {
  return false;
}

void NodeGroupModel::SetValue(int index, const std::wstring& value) {
  auto& prop = properties[index];
  if (prop.def)
    prop.def->SetText(property_model_, property_model_.node_, prop.prop_decl_id,
                      value);
  else {
    scada::NodeAttributes attributes;
    // TODO: Other attributes.
    switch (prop.attribute_id) {
      case scada::AttributeId::BrowseName:
        attributes.set_browse_name(ToString(value));
        break;
      case scada::AttributeId::DisplayName:
        attributes.set_display_name(scada::ToLocalizedText(value));
        break;
    }

    if (attributes.empty())
      return;

    property_model_.task_manager_.PostUpdateTask(
        property_model_.node_.node_id(), attributes, {});
  }
}

ui::EditData NodeGroupModel::GetEditData(int index) const {
  auto& prop = properties[index];
  if (prop.def)
    return prop.def->GetPropertyEditor(property_model_, property_model_.node_,
                                       prop.prop_decl_id);
  else
    return {};
}
