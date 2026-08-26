#include "modules/node_properties/node_group_model.h"

#include "modules/node_properties/node_property_model.h"
#include "properties/property_definition.h"
#include "services/task_manager.h"

NodeGroupModel::NodeGroupModel(NodePropertyModel& property_model)
    : property_model_{property_model} {}

NodeGroupModel::~NodeGroupModel() = default;

int NodeGroupModel::GetCount() const {
  return properties.size();
}

scada::aui::PropertyGroup* NodeGroupModel::GetSubgroup(int index) const {
  return properties[index].submodel.get();
}

std::u16string NodeGroupModel::GetName(int index) const {
  return properties[index].name;
}

std::u16string NodeGroupModel::GetValue(int index) const {
  auto& prop = properties[index];
  if (prop.def)
    return prop.def->GetText(property_model_, property_model_.node_,
                             prop.prop_decl_id);
  else
    return ToString16(property_model_.node_.attribute(prop.attribute_id));
}

scada::aui::PropertyGroup::ItemType NodeGroupModel::GetType(int index) const {
  return properties[index].type;
}

bool NodeGroupModel::IsInherited(int index) const {
  return false;
}

void NodeGroupModel::SetValue(int index, const std::u16string& value) {
  const auto& prop = properties[index];
  if (prop.def)
    prop.def->SetText(property_model_, property_model_.node_, prop.prop_decl_id,
                      value);
  else {
    scada::NodeAttributes attributes;
    // TODO: Other attributes.
    switch (prop.attribute_id) {
      case scada::AttributeId::BrowseName:
        attributes.browse_name = ToString(value);
        break;
      case scada::AttributeId::DisplayName:
        attributes.display_name = scada::ToLocalizedText(value);
        break;
    }

    if (attributes.empty())
      return;

    property_model_.task_manager_.PostUpdateTask(
        property_model_.node_.node_id(), attributes, {});
  }
}

scada::aui::EditData NodeGroupModel::GetEditData(int index) const {
  auto& prop = properties[index];
  if (!prop.def) {
    // A plain attribute is modifiable only where SetValue above actually
    // writes it. Every other attribute used to be offered a text editor whose
    // result SetValue dropped on the floor, so the operator retyped a
    // NodeClass or a TypeDefinition and saw it revert with no explanation.
    switch (prop.attribute_id) {
      case scada::AttributeId::BrowseName:
      case scada::AttributeId::DisplayName:
        return {};
      default:
        return {.editor_type = scada::aui::EditData::EditorType::NONE};
    }
  }

  return prop.def->GetPropertyEditor(property_model_, property_model_.node_,
                                     prop.prop_decl_id);
}

void NodeGroupModel::HandleEditButton(int index) const {
  auto& prop = properties[index];
  if (!prop.def)
    return;

  return prop.def->HandleEditButton(property_model_, property_model_.node_,
                                    prop.prop_decl_id);
}
