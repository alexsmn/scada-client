#include "property_defs.h"

#include "base/range_util.h"
#include "base/string_piece_util.h"
#include "base/string_util.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "common/format.h"
#include "common/formula_util.h"
#include "common/scada_expression.h"
#include "components/transport/transport_dialog.h"
#include "controls/color.h"
#include "core/node_management_service.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "model/scada_node_ids.h"
#include "net/transport_string.h"
#include "node_service/node_promises.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "services/task_manager.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <iterator>

namespace {

static const char16_t kDefaultColorString[] = u"<Стандартный>";
static const char16_t kChoiceNone[] = u"<Нет>";

NodeRef FindNodeByNameAndType(const NodeRef& parent_node,
                              const std::u16string_view& name,
                              const scada::NodeId& node_type_id) {
  for (const auto& node : parent_node.targets(scada::id::Organizes)) {
    if (IsInstanceOf(node, node_type_id)) {
      const auto& node_name = GetFullDisplayName(node);
      if (boost::algorithm::iequals(node_name, name))
        return node;
    }
    if (auto n = FindNodeByNameAndType(node, name, node_type_id))
      return n;
  }
  return nullptr;
}

promise<> FetchNodeNamesRecursive(
    const NodeRef& parent_node,
    const scada::NodeId& type_definition_id,
    const aui::EditData::AsyncChoiceCallback& callback) {
  return FetchChildren(parent_node).then([=] {
    auto children = parent_node.targets(scada::id::Organizes);

    auto child_names =
        children |
        boost::adaptors::filtered([type_definition_id](const NodeRef& node) {
          return IsInstanceOf(node, type_definition_id);
        }) |
        boost::adaptors::transformed(&GetFullDisplayName) | to_vector;
    callback(child_names, false);

    auto recursive_promises =
        children | boost::adaptors::transformed([type_definition_id, callback](
                                                    const NodeRef& node) {
          return FetchNodeNamesRecursive(node, type_definition_id, callback);
        });

    return make_all_promise_void(recursive_promises);
  });
}

aui::EditData::AsyncChoiceHandler MakeAsyncChoiceHandler(
    const NodeRef& parent,
    const scada::NodeId& type_definition_id) {
  return [parent, type_definition_id](
             const aui::EditData::AsyncChoiceCallback& callback) {
    callback({kChoiceNone}, false);
    FetchNodeNamesRecursive(parent, type_definition_id, callback)
        .then([callback] { callback({}, true); });
  };
}

NodeRef GetTargetTypeDefinition(const NodeRef& type_definition,
                                const scada::NodeId& reference_type_id) {
  for (auto t = type_definition; t; t = t.supertype()) {
    if (auto target_type_definition = t.target(reference_type_id))
      return target_type_definition;
  }
  return nullptr;
}

std::pair<scada::NodeId /*parent_id*/, std::string /*component_name*/>
ParseChannelPath(std::string_view channel_path) {
  scada::NodeId parent_id;
  std::string_view component_name;
  auto node_id = GetFormulaSingleNodeId(channel_path);
  if (node_id.is_null() ||
      !IsNestedNodeId(node_id, parent_id, component_name)) {
    parent_id = scada::NodeId();
    component_name = channel_path;
  }
  // Note: can't return string_view since |node_id| goes out of scope.
  return {parent_id, std::string{component_name}};
}

promise<> GetAllSubtypesProperties(
    const NodeRef& type_definition,
    const std::shared_ptr<std::unordered_set<NodeRef>>& property_decls) {
  return FetchNode(type_definition).then([=] {
    GetTypeProperties(type_definition, *property_decls);

    auto subtypes = type_definition.targets(scada::id::HasSubtype);
    auto subtype_fetch_promises =
        subtypes | boost::adaptors::transformed([&](const NodeRef& node) {
          return GetAllSubtypesProperties(node, property_decls);
        });

    return make_all_promise_void(subtype_fetch_promises);
  });
}

const PropertyDefinition kNamePropDef(aui::TableColumn::LEFT, 150);
const PropertyDefinition kStringPropDef(aui::TableColumn::LEFT);
const PropertyDefinition kIntPropDef(aui::TableColumn::RIGHT);
const PropertyDefinition kDoublePropDef(aui::TableColumn::RIGHT);
const BoolPropertyDefinition kBoolPropDef;
const ReferencePropertyDefinition kRefPropDef;
const ColorPropertyDefinition kColorPropDef;
const EnumPropertyDefinition kEnumPropDef;

const ChannelPropertyDefinition kObjectInput1DevicePropDef(u"Устройство", true);
const ChannelPropertyDefinition kObjectInput1ChannelPropDef(u"Канал", false);
const HierachicalPropertyDefinition kObjectInput1PropDef(
    {&kObjectInput1DevicePropDef, &kObjectInput1ChannelPropDef});

const ChannelPropertyDefinition kObjectInput2DevicePropDef(
    u"Устройство (Резерв)",
    true);
const ChannelPropertyDefinition kObjectInput2ChannelPropDef(u"Канал (Резерв)",
                                                            false);
const HierachicalPropertyDefinition kObjectInput2PropDef(
    {&kObjectInput2DevicePropDef, &kObjectInput2ChannelPropDef});

const ChannelPropertyDefinition kObjectOutputDevicePropDef(
    u"Устройство (Управление)",
    true);
const ChannelPropertyDefinition kObjectOutputChannelPropDef(
    u"Канал (Управление)",
    false);
const HierachicalPropertyDefinition kObjectOutputPropDef(
    {&kObjectOutputDevicePropDef, &kObjectOutputChannelPropDef});

const TransportPropertyDefinition kLinkTransportPropDef;

std::unordered_map<scada::NodeId, const PropertyDefinition*>
    kPropertyDefinitionMap = {
        {data_items::id::DataItemType_Input1, &kObjectInput1PropDef},
        {data_items::id::DataItemType_Input2, &kObjectInput2PropDef},
        {data_items::id::DataItemType_Output, &kObjectOutputPropDef},
        {devices::id::LinkType_Transport, &kLinkTransportPropDef},
        {data_items::id::TsFormatType_OpenColor, &kColorPropDef},
        {data_items::id::TsFormatType_CloseColor, &kColorPropDef},
};

}  // namespace

const PropertyDefinition* GetPropertyDef(const NodeRef& prop_decl) {
  auto i = kPropertyDefinitionMap.find(prop_decl.node_id());
  if (i != kPropertyDefinitionMap.end())
    return i->second;

  if (prop_decl.node_class() == scada::NodeClass::ReferenceType)
    return &kRefPropDef;

  static const scada::NumericId kIntDataTypeIds[] = {
      scada::id::Int8,  scada::id::UInt8,  scada::id::Int16, scada::id::UInt16,
      scada::id::Int32, scada::id::UInt32, scada::id::Int64, scada::id::UInt64};

  auto data_type = prop_decl.data_type();

  if (IsSubtypeOf(data_type, scada::id::Boolean))
    return &kBoolPropDef;
  if (IsSubtypeOf(data_type, scada::id::Double))
    return &kIntPropDef;
  if (IsSubtypeOf(data_type, scada::id::String) ||
      IsSubtypeOf(data_type, scada::id::LocalizedText))
    return &kStringPropDef;
  if (IsSubtypeOf(data_type, scada::id::Enumeration))
    return &kEnumPropDef;

  // NOTE: Enums are subtypes of INT.
  for (const auto data_type_id : kIntDataTypeIds) {
    if (IsSubtypeOf(data_type, data_type_id))
      return &kIntPropDef;
  }

  return nullptr;
}

// Returns property declarations and forward reference types.
void GetTypeProperties(const NodeRef& type_definition,
                       std::unordered_set<NodeRef>& property_decls) {
  assert(type_definition.fetched());
  for (auto supertype_definition = type_definition; supertype_definition;
       supertype_definition = supertype_definition.supertype()) {
    for (const auto& p : supertype_definition.targets(scada::id::HasProperty))
      property_decls.emplace(p);
    for (const auto& r : supertype_definition.references(
             scada::id::NonHierarchicalReferences)) {
      // TODO: Introduce common base reference type.
      if (!IsSubtypeOf(r.reference_type, scada::id::Creates) &&
          !IsSubtypeOf(r.reference_type, scada::id::HasSubtype)) {
        property_decls.emplace(r.reference_type);
      }
    }
  }
}

PropertyDefs GetTypePropertyDefs(const NodeRef& type_definition) {
  assert(type_definition.fetched());

  std::unordered_set<NodeRef> prop_decls;
  GetTypeProperties(type_definition, prop_decls);

  PropertyDefs properties;
  properties.reserve(prop_decls.size());

  for (auto& prop_decl : prop_decls) {
    if (auto* def = GetPropertyDef(prop_decl))
      properties.emplace_back(prop_decl, def);
  }
  return properties;
}

void CollectCreates(const NodeRef& node,
                    std::unordered_set<NodeRef>& child_type_definitions) {
  for (auto&& creates : node.targets(scada::id::Creates))
    child_type_definitions.emplace(std::move(creates));
}

// Returns unfetched type definitions.
std::unordered_set<NodeRef> GetChildTypeDefinitions(
    const NodeRef& parent_node) {
  assert(parent_node.fetched());
  assert(parent_node.type_definition().fetched());

  std::unordered_set<NodeRef> child_type_definitions;

  CollectCreates(parent_node, child_type_definitions);

  for (auto node_type = parent_node.type_definition(); node_type;
       node_type = node_type.supertype()) {
    CollectCreates(node_type, child_type_definitions);
  }

  return child_type_definitions;
}

PropertyDefs GetPropertyDefs(
    const std::unordered_set<NodeRef>& property_decls) {
  PropertyDefs property_defs =
      property_decls |
      boost::adaptors::transformed([](const NodeRef& property_decl) {
        return std::make_pair(property_decl, GetPropertyDef(property_decl));
      }) |
      boost::adaptors::filtered([](const auto& p) { return !!p.second; }) |
      to_vector;
  std::sort(property_defs.begin(), property_defs.end());
  return property_defs;
}

promise<PropertyDefs> GetChildPropertyDefs(const NodeRef& parent_node) {
  auto property_decls = std::make_shared<std::unordered_set<NodeRef>>();
  return FetchNode(parent_node)
      .then([parent_node] { return GetChildTypeDefinitions(parent_node); })
      .then([property_decls](
                const std::unordered_set<NodeRef>& child_type_definitions) {
        auto child_promises = child_type_definitions |
                              boost::adaptors::transformed(
                                  [&](const NodeRef& child_type_definition) {
                                    return GetAllSubtypesProperties(
                                        child_type_definition, property_decls);
                                  });
        return make_all_promise_void(child_promises);
      })
      .then([property_decls] { return GetPropertyDefs(*property_decls); });
}

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
  const auto& property = node[prop_decl_id];
  if (!property)
    return;

  scada::Variant value;
  if (!StringToValue(text, property.data_type().node_id(), value))
    return;

  context.task_manager_.PostUpdateTask(node.node_id(), {},
                                       {{prop_decl_id, std::move(value)}});
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

std::u16string ReferencePropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  auto target_type_definition =
      GetTargetTypeDefinition(node.type_definition(), prop_decl_id);
  if (!target_type_definition)
    return std::u16string();

  if (auto target = node.target(prop_decl_id))
    return target.display_name();
  else
    return kChoiceNone;
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
  if (auto* enum_strings =
          enum_strings_value.get_if<std::vector<scada::LocalizedText>>()) {
    result.choices = *enum_strings;
  }

  return result;
}

// ChannelPropertyDefinition

const char16_t ChannelPropertyDefinition::kParentGroupDevice[] = u"<Группа>";

std::u16string ChannelPropertyDefinition::GetTitle(
    const PropertyContext& context,
    const NodeRef& property_declaration) const {
  return title_;
}

std::u16string ChannelPropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  if (!IsInstanceOf(node, data_items::id::DataItemType))
    return std::u16string();

  auto channel_path = node[prop_decl_id].value().get_or(std::string{});

  std::string name = GetFormulaSingleName(channel_path);
  if (!name.empty()) {
    auto node_id = NodeIdFromScadaString(name);
    scada::NodeId parent_id;
    std::string_view component_name;
    if (!node_id.is_null() &&
        IsNestedNodeId(node_id, parent_id, component_name)) {
      return device_
                 ? GetFullDisplayName(context.node_service_.GetNode(parent_id))
                 : base::UTF8ToUTF16(AsStringPiece(component_name));
    } else if (auto path = GetParentGroupChannelPath(name); !path.empty()) {
      return device_ ? kParentGroupDevice
                     : base::UTF8ToUTF16(AsStringPiece(path));
    }
  }

  return device_ ? std::u16string{} : base::UTF8ToUTF16(channel_path);
}

void ChannelPropertyDefinition::SetText(const PropertyContext& context,
                                        const NodeRef& node,
                                        const scada::NodeId& prop_decl_id,
                                        const std::u16string& text) const {
  if (!IsInstanceOf(node, data_items::id::DataItemType))
    return;

  scada::NodeId new_device_id;
  if (device_) {
    new_device_id = FindNodeByNameAndType(
                        context.node_service_.GetNode(devices::id::Devices),
                        text, devices::id::DeviceType)
                        .node_id();
  }

  auto channel_path = node[prop_decl_id].value().get_or(std::string{});
  auto [parent_id, component_name] = ParseChannelPath(channel_path);

  auto item_path = std::string{component_name};
  if (device_)
    parent_id = new_device_id;
  else
    item_path = base::UTF16ToUTF8(text);

  std::string formula;
  if (!parent_id.is_null()) {
    if (item_path.empty()) {
      item_path =
          ToString(context.node_service_.GetNode(devices::id::DeviceType_Online)
                       .browse_name());
    }
    formula = MakeNodeIdFormula(MakeNestedNodeId(parent_id, item_path));
  } else {
    formula = item_path;
  }

  context.task_manager_.PostUpdateTask(node.node_id(), {},
                                       {{prop_decl_id, std::move(formula)}});
}

aui::EditData ChannelPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  aui::EditData result{aui::EditData::EditorType::DROPDOWN};

  if (device_) {
    result.async_choice_handler = MakeAsyncChoiceHandler(
        context.node_service_.GetNode(devices::id::Devices),
        devices::id::DeviceType);

  } else {
    auto channel_path = node[prop_decl_id].value().get_or(std::string{});
    auto [parent_id, component_name] = ParseChannelPath(channel_path);
    const auto& parent = context.node_service_.GetNode(parent_id);
    for (auto& component : GetDataVariables(parent)) {
      result.choices.emplace_back(
          scada::ToLocalizedText(component.browse_name().name()));
    }
  }

  return result;
}

aui::EditData TransportPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  aui::EditData data{aui::EditData::EditorType::BUTTON};

  data.action_handler = [&dialog_service =
                             context.dialog_service_](std::u16string& text) {
    net::TransportString transport_string{base::UTF16ToUTF8(text)};
    if (!ShowTransportDialog(dialog_service, transport_string))
      return false;
    text = base::UTF8ToUTF16(transport_string.ToString());
    return true;
  };

  return data;
}

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
