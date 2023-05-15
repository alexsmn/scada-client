#include "services/properties/property_defs.h"

#include "base/strings/utf_string_conversions.h"
#include "components/transport/transport_dialog.h"
#include "controls/color.h"
#include "core/node_management_service.h"
#include "model/data_items_node_ids.h"
#include "net/transport_string.h"
#include "node_service/node_service.h"
#include "services/properties/property_context.h"
#include "services/properties/property_util.h"
#include "services/task_manager.h"

namespace {

const char16_t kDefaultColorString[] = u"<Стандартный>";

NodeRef GetTargetTypeDefinition(const NodeRef& type_definition,
                                const scada::NodeId& reference_type_id) {
  for (auto t = type_definition; t; t = t.supertype()) {
    if (auto target_type_definition = t.target(reference_type_id))
      return target_type_definition;
  }
  return nullptr;
}

}  // namespace

// ReferencePropertyDefinition

std::u16string ReferencePropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  auto target_type_definition =
      GetTargetTypeDefinition(node.type_definition(), prop_decl_id);
  if (!target_type_definition) {
    return std::u16string();
  }

  if (auto target = node.target(prop_decl_id))
    return target.display_name();
  else
    return std::u16string{kChoiceNone};
}

void ReferencePropertyDefinition::SetText(const PropertyContext& context,
                                          const NodeRef& node,
                                          const scada::NodeId& prop_decl_id,
                                          const std::u16string& text) const {
  auto target_type_definition =
      GetTargetTypeDefinition(node.type_definition(), prop_decl_id);
  if (!target_type_definition)
    return;

  NodeRef target;
  if (text != kChoiceNone) {
    target = FindNodeByNameAndType(
        context.node_service_.GetNode(scada::id::ObjectsFolder), text,
        target_type_definition.node_id());
  }

  auto old_ref_node = node.target(prop_decl_id);
  if (old_ref_node == target)
    return;

  if (old_ref_node) {
    context.task_manager_.PostDeleteReference(prop_decl_id, node.node_id(),
                                              old_ref_node.node_id());
  }
  if (target) {
    context.task_manager_.PostAddReference(prop_decl_id, node.node_id(),
                                           target.node_id());
  }
}

aui::EditData ReferencePropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  aui::EditData result{aui::EditData::EditorType::DROPDOWN};

  auto objects = context.node_service_.GetNode(scada::id::ObjectsFolder);
  auto target_type_definition =
      GetTargetTypeDefinition(node.type_definition(), prop_decl_id);

  result.async_choice_handler = MakeAsyncChoiceHandler(
      context.node_service_.GetNode(scada::id::ObjectsFolder),
      target_type_definition.node_id());

  return result;
}

bool ReferencePropertyDefinition::IsReadOnly(
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  return !GetTargetTypeDefinition(node.type_definition(), prop_decl_id);
}

std::u16string BoolPropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  auto prop = node[prop_decl_id];
  if (!prop)
    return std::u16string();

  auto bool_value = prop.value().get_or(false);
  return bool_value ? std::u16string{scada::Variant::kTrueString}
                    : std::u16string{scada::Variant::kFalseString};
}

aui::EditData BoolPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  aui::EditData result{aui::EditData::EditorType::DROPDOWN};
  result.choices = {std::u16string{scada::Variant::kFalseString},
                    std::u16string{scada::Variant::kTrueString}};
  return result;
}

std::u16string EnumPropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  const auto& property = node[prop_decl_id];
  if (!property)
    return std::u16string();

  int int_value;
  if (!property.value().get(int_value))
    return std::u16string();

  auto enum_strings_value = property.data_type()["EnumStrings"].value();
  auto* enum_strings =
      enum_strings_value.get_if<std::vector<scada::LocalizedText>>();
  if (!enum_strings || int_value < 0 ||
      int_value >= static_cast<int>(enum_strings->size()))
    return std::u16string();

  return (*enum_strings)[int_value];
}

void EnumPropertyDefinition::SetText(const PropertyContext& context,
                                     const NodeRef& node,
                                     const scada::NodeId& prop_decl_id,
                                     const std::u16string& text) const {
  const auto& property = node[prop_decl_id];
  if (!property)
    return;

  auto enum_strings_value = property.data_type()["EnumStrings"].value();
  auto* enum_strings =
      enum_strings_value.get_if<std::vector<scada::LocalizedText>>();
  if (!enum_strings)
    return;

  auto i = std::find(enum_strings->begin(), enum_strings->end(), text);
  if (i == enum_strings->end())
    return;

  int int_value = static_cast<int>(i - enum_strings->begin());
  context.task_manager_.PostUpdateTask(node.node_id(), {},
                                       {{prop_decl_id, int_value}});
}

aui::EditData EnumPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  const auto& type_definition = node.type_definition();
  const auto& property_declaration = type_definition[prop_decl_id];
  if (!property_declaration)
    return aui::EditData{aui::EditData::EditorType::NONE};

  aui::EditData result{aui::EditData::EditorType::DROPDOWN};

  auto enum_strings_value =
      property_declaration.data_type()["EnumStrings"].value();
  if (const auto* enum_strings =
          enum_strings_value.get_if<std::vector<scada::LocalizedText>>()) {
    result.choices = *enum_strings;
  }

  return result;
}

// TransportPropertyDefinition

aui::EditData TransportPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  return {.editor_type = aui::EditData::EditorType::BUTTON};
}

void TransportPropertyDefinition::HandleEditButton(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  auto text = GetText(context, node, prop_decl_id);

  net::TransportString transport_string{base::UTF16ToUTF8(text)};
  ShowTransportDialog(context.dialog_service_, transport_string)
      .then([context, node,
             prop_decl_id](const net::TransportString& transport_string) {
        auto text = base::UTF8ToUTF16(transport_string.ToString());
        SetTextHelper(context, node, prop_decl_id, text);
      });
}

// ColorPropertyDefinition

std::u16string ColorPropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  auto value = node[prop_decl_id].value();
  auto color_index = value.get_or(-1);
  if (color_index >= 0 && color_index < static_cast<int>(aui::GetColorCount()))
    return std::u16string{aui::GetColorName(color_index)};
  else
    return kDefaultColorString;
}

void ColorPropertyDefinition::SetText(const PropertyContext& context,
                                      const NodeRef& node,
                                      const scada::NodeId& prop_decl_id,
                                      const std::u16string& text) const {
  if (text.empty())
    return;

  int color = aui::FindColorName(text.c_str());
  context.task_manager_.PostUpdateTask(node.node_id(), {},
                                       {{prop_decl_id, color}});
}

aui::EditData ColorPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  aui::EditData result{aui::EditData::EditorType::DROPDOWN};
  result.choices.reserve(1 + aui::GetColorCount());
  result.choices.emplace_back(kDefaultColorString);
  for (size_t i = 0; i < aui::GetColorCount(); i++)
    result.choices.emplace_back(aui::GetColorName(i));
  return result;
}
