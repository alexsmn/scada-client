#include "properties/property_definition.h"

#include "node_service/node_service.h"
#include "properties/property_util.h"

// PropertyDefinition

PropertyDefinition::PropertyDefinition(aui::TableColumn::Alignment alignment,
                                       int width)
    : alignment_(alignment), width_(width) {}

std::u16string PropertyDefinition::GetTitle(
    const PropertyContext& context,
    const NodeRef& property_declaration) const {
  return ToString16(property_declaration.display_name());
}

bool PropertyDefinition::IsReadOnly(const NodeRef& node,
                                    const scada::NodeId& prop_decl_id) const {
  return !node[prop_decl_id];
}

std::u16string PropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  auto value = node[prop_decl_id].value();
  return {ToString16(value), false};
}

void PropertyDefinition::SetText(const PropertyContext& context,
                                 const NodeRef& node,
                                 const scada::NodeId& prop_decl_id,
                                 const std::u16string& text) const {
  SetTextHelper(context, node, prop_decl_id, text);
}

aui::EditData PropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  const auto& type_definition = node.type_definition();
  const auto& property_declaration = type_definition[prop_decl_id];
  if (!property_declaration)
    return aui::EditData{aui::EditData::EditorType::NONE};

  assert(property_declaration.node_class() == scada::NodeClass::Variable);
  return aui::EditData{aui::EditData::EditorType::TEXT};
}

void PropertyDefinition::HandleEditButton(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {}

void PropertyDefinition::GetAdditionalTargets(
    const NodeRef& node,
    const scada::NodeId& prop_decl_id,
    std::vector<scada::NodeId>& targets) const {}
