#include "property_defs.h"

#include <iterator>
#include <map>

#include "base/color.h"
#include "base/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "common/format.h"
#include "common/formula_util.h"
#include "common/node_id_util.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "common/scada_node_ids.h"
#include "core/node_management_service.h"
#include "services/task_manager.h"

namespace {

static const base::char16 kDefaultColorString[] = L"<Стандартный>";
static const base::char16 kChoiceNone[] = L"<Нет>";

NodeRef FindNodeByNameAndType(const NodeRef& parent_node,
                              const base::StringPiece16& name,
                              const scada::NodeId& node_type_id) {
  for (auto& node : parent_node.organizes()) {
    if (IsInstanceOf(node, node_type_id)) {
      const auto& node_name = GetFullDisplayName(node);
      if (IsEqualNoCase(node_name, name))
        return node;
    }
    if (auto n = FindNodeByNameAndType(node, name, node_type_id))
      return n;
  }
  return nullptr;
}

void GetNodeNamesRecursive(const NodeRef& parent_node,
                           const scada::NodeId& type_definition_id,
                           std::vector<base::string16>& names) {
  for (auto& node : parent_node.organizes()) {
    if (IsInstanceOf(node, type_definition_id))
      names.emplace_back(GetFullDisplayName(node));
    GetNodeNamesRecursive(node, type_definition_id, names);
  }
}

NodeRef GetTargetTypeDefinition(const NodeRef& type_definition,
                                const scada::NodeId& reference_type_id) {
  for (auto t = type_definition; t; t = t.supertype()) {
    if (auto target_type_definition = t.target(reference_type_id))
      return target_type_definition;
  }
  return nullptr;
}

const PropertyDefinition kNamePropDef(ui::TableColumn::LEFT, 150);
const PropertyDefinition kStringPropDef(ui::TableColumn::LEFT);
const PropertyDefinition kIntPropDef(ui::TableColumn::RIGHT);
const PropertyDefinition kDoublePropDef(ui::TableColumn::RIGHT);
const BoolPropertyDefinition kBoolPropDef;
const ReferencePropertyDefinition kRefPropDef;
const ColorPropertyDefinition kColorPropDef;
const EnumPropertyDefinition kEnumPropDef;

const ChannelPropertyDefinition kObjectInput1DevicePropDef(L"Устройство", true);
const ChannelPropertyDefinition kObjectInput1ChannelPropDef(L"Канал", false);
const HierachicalPropertyDefinition kObjectInput1PropDef(
    {&kObjectInput1DevicePropDef, &kObjectInput1ChannelPropDef});

const ChannelPropertyDefinition kObjectInput2DevicePropDef(
    L"Устройство (Резерв)",
    true);
const ChannelPropertyDefinition kObjectInput2ChannelPropDef(L"Канал (Резерв)",
                                                            false);
const HierachicalPropertyDefinition kObjectInput2PropDef(
    {&kObjectInput2DevicePropDef, &kObjectInput2ChannelPropDef});

const ChannelPropertyDefinition kObjectOutputDevicePropDef(
    L"Устройство (Управление)",
    true);
const ChannelPropertyDefinition kObjectOutputChannelPropDef(
    L"Канал (Управление)",
    false);
const HierachicalPropertyDefinition kObjectOutputPropDef(
    {&kObjectOutputDevicePropDef, &kObjectOutputChannelPropDef});

const TransportPropertyDefinition kLinkTransportPropDef;

// TODO: Avoid property definitions.
std::map<scada::NodeId, const PropertyDefinition*> kPropertyDefinitionMap = {
    // Default
    {kObjectSeverityPropTypeId, &kIntPropDef},
    {id::DeviceType_Disabled, &kBoolPropDef},
    {id::HasSimulationSignal, &kRefPropDef},
    // DataGroup
    {id::DataGroupType_Simulated, &kBoolPropDef},
    // DataItem
    {id::DataItemType_Simulated, &kBoolPropDef},
    {id::DataItemType_Alias, &kStringPropDef},
    {kObjectInput1PropTypeId, &kObjectInput1PropDef},
    {kObjectInput2PropTypeId, &kObjectInput2PropDef},
    {kObjectOutputPropTypeId, &kObjectOutputPropDef},
    {kObjectOutputConditionPropTypeId, &kStringPropDef},
    {kTsInvertedPropTypeId, &kBoolPropDef},
    {id::AnalogItemType_DisplayFormat, &kStringPropDef},     // TODO: Editor
    {id::AnalogItemType_EngineeringUnits, &kStringPropDef},  // TODO: Combo
    {id::AnalogItemType_Aperture, &kDoublePropDef},
    {id::AnalogItemType_Deadband, &kDoublePropDef},
    {id::HasTsFormat, &kRefPropDef},
    {kTitConversionPropTypeId, &kEnumPropDef},  // TODO: Enum
    {kTitEuLoPropTypeId, &kDoublePropDef},
    {kTitEuHiPropTypeId, &kDoublePropDef},
    {kTitIrLoPropTypeId, &kDoublePropDef},
    {kTitIrHiPropTypeId, &kDoublePropDef},
    {kTitLimitLoLoPropTypeId, &kDoublePropDef},
    {kTitLimitLoPropTypeId, &kDoublePropDef},
    {kTitLimitHiPropTypeId, &kDoublePropDef},
    {kTitLimitHiHiPropTypeId, &kDoublePropDef},
    {kObjectStalePeriodTypeId, &kIntPropDef},
    {kObjectHistoricalDbPropTypeId, &kRefPropDef},
    {kTitClampingPropTypeId, &kEnumPropDef},  // TODO: Enum
    // Link
    {kLinkTransportStringPropTypeId, &kLinkTransportPropDef},
    {kIec60870LinkSendQueueSizePropTypeId, &kIntPropDef},
    {kIec60870LinkReceiveQueueSizePropTypeId, &kIntPropDef},
    {kIec60870LinkConfirmationTimeoutPropTypeId, &kIntPropDef},  // time delta
    {kIec60870LinkTerminationTimeoutProtocolPropTypeId,
     &kIntPropDef},  // time delta
    // IEC-60870 Link
    {id::Iec60870LinkType_Protocol, &kIntPropDef},  // TODO: Enum
    {kIec60870LinkModePropTypeId, &kIntPropDef},    // TODO: Enum
    {kIec60870LinkT0PropTypeId, &kIntPropDef},
    {kIec60870LinkDeviceAddressSizePropTypeId, &kIntPropDef},
    {kIec60870LinkCotSizePropTypeId, &kIntPropDef},
    {kIec60870LinkInfoAddressSizePropTypeId, &kIntPropDef},
    {kIec60870LinkCollectDataPropTypeId, &kBoolPropDef},
    {kIec60870LinkSendRetriesPropTypeId, &kIntPropDef},
    {kIec60870LinkCrcProtectionPropTypeId, &kBoolPropDef},
    {kIec60870LinkSendTimeoutPropTypeId, &kIntPropDef},
    {kIec60870LinkReceiveTimeoutPropTypeId, &kIntPropDef},
    {kIec60870LinkIdleTimeoutPropTypeId, &kIntPropDef},
    {kIec60870LinkAnonymousPropTypeId, &kBoolPropDef},
    // IEC-60870 Device
    {kIec60870DeviceAddressPropTypeId, &kIntPropDef},
    {kIec60870DeviceLinkAddressPropTypeId, &kIntPropDef},
    {kIec60870DeviceInterrogateOnStartPropTypeId, &kBoolPropDef},
    {kIec60870DeviceInterrogationPeriodPropTypeId, &kIntPropDef},  // time delta
    {kIec60870DeviceSynchronizeClockOnStartPropTypeId, &kBoolPropDef},
    {kIec60870DeviceClockSynchronizationPeriodPropTypeId, &kIntPropDef},
    {id::Iec60870DeviceType_UtcTime, &kBoolPropDef},
    {kIec60870DeviceGroup1PollPeriodPropTypeId, &kIntPropDef},
    {kIec60870DeviceGroup2PollPeriodPropTypeId, &kIntPropDef},
    {kIec60870DeviceGroup3PollPeriodPropTypeId, &kIntPropDef},
    {kIec60870DeviceGroup4PollPeriodPropTypeId, &kIntPropDef},
    {kIec60870DeviceGroup5PollPeriodPropTypeId, &kIntPropDef},
    {kIec60870DeviceGroup6PollPeriodPropTypeId, &kIntPropDef},
    {kIec60870DeviceGroup7PollPeriodPropTypeId, &kIntPropDef},
    {kIec60870DeviceGroup8PollPeriodPropTypeId, &kIntPropDef},
    {kIec60870DeviceGroup9PollPeriodPropTypeId, &kIntPropDef},
    {kIec60870DeviceGroup10PollPeriodPropTypeId, &kIntPropDef},
    {kIec60870DeviceGroup11PollPeriodPropTypeId, &kIntPropDef},
    {kIec60870DeviceGroup12PollPeriodPropTypeId, &kIntPropDef},
    {kIec60870DeviceGroup13PollPeriodPropTypeId, &kIntPropDef},
    {kIec60870DeviceGroup14PollPeriodPropTypeId, &kIntPropDef},
    {kIec60870DeviceGroup15PollPeriodPropTypeId, &kIntPropDef},
    {kIec60870DeviceGroup16PollPeriodPropTypeId, &kIntPropDef},
    // Modbus Link
    {kModbusPortModePropTypeId, &kIntPropDef},  // TODO: Enum
    // Modbus Dev
    {kModbusDeviceAddressPropTypeId, &kIntPropDef},
    {kModbusDeviceRepeatCountPropTypeId, &kIntPropDef},
    // IEC-81650 Device
    {id::Iec61850DeviceType_Host, &kStringPropDef},
    {id::Iec61850DeviceType_Port, &kIntPropDef},
    // IEC-81650 ConfigurableObject
    {id::Iec61850ConfigurableObjectType_Reference, &kStringPropDef},
    // Sim Signal
    {kSimulationSignalTypePropTypeId, &kIntPropDef},            // TODO: Enum
    {kSimulationSignalPeriodPropTypeId, &kIntPropDef},          // time delta
    {kSimulationSignalPhasePropTypeId, &kIntPropDef},           // time delta
    {kSimulationSignalUpdateIntervalPropTypeId, &kIntPropDef},  // time delta
    // User
    {kUserAccessRightsPropTypeId, &kIntPropDef},  // TODO: Set
    // TsFormat
    {id::TsFormatType_OpenLabel, &kStringPropDef},
    {id::TsFormatType_CloseLabel, &kStringPropDef},
    {kTsFormatOpenColorPropTypeId, &kColorPropDef},
    {kTsFormatCloseColorPropTypeId, &kColorPropDef},
    // Historical DB
    {kHistoricalDbDepthPropTypeId, &kIntPropDef},  // time delta
};

}  // namespace

const PropertyDefinition* GetPropertyDef(const scada::NodeId& prop_decl_id) {
  auto i = kPropertyDefinitionMap.find(prop_decl_id);
  return i == kPropertyDefinitionMap.end() ? nullptr : i->second;
}

PropertyDefs GetTypeProperties(const NodeRef& type_definition) {
  PropertyDefs properties;
  properties.reserve(32);
  for (auto t = type_definition; t; t = t.supertype()) {
    for (auto p : t.properties()) {
      if (auto* def = GetPropertyDef(p.id()))
        properties.emplace_back(p.id(), def);
    }
    for (auto r : t.references()) {
      if (IsSubtypeOf(r.reference_type, scada::id::HasProperty))
        break;
      if (auto* def = GetPropertyDef(r.reference_type.id()))
        properties.emplace_back(r.reference_type.id(), def);
    }
  }
  return properties;
}

PropertyDefinition::PropertyDefinition(ui::TableColumn::Alignment alignment,
                                       int width)
    : alignment_(alignment), width_(width) {}

base::string16 PropertyDefinition::GetTitle(
    const PropertyContext& context,
    const scada::NodeId& prop_decl_id) const {
  auto type = context.node_service_.GetNode(prop_decl_id);
  assert(type);
  return ToString16(type.display_name());
}

bool PropertyDefinition::IsReadOnly(const NodeRef& node,
                                    const scada::NodeId& prop_decl_id) const {
  return !node[prop_decl_id];
}

base::string16 PropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  auto value = node[prop_decl_id].value();
  return {ToString16(value), false};
}

void PropertyDefinition::SetText(const PropertyContext& context,
                                 const NodeRef& node,
                                 const scada::NodeId& prop_decl_id,
                                 const base::string16& text) const {
  auto prop = node[prop_decl_id];
  if (!prop)
    return;

  scada::Variant value;
  if (!StringToValue(text, prop.data_type().id(), value))
    return;

  context.task_manager_.PostUpdateTask(node.id(), {},
                                       {{prop_decl_id, std::move(value)}});
}

PropertyEditor PropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_decl_id) const {
  auto prop_decl = type_definition[prop_decl_id];
  if (!prop_decl)
    return PropertyEditor(PropertyEditor::NONE);

  assert(prop_decl.node_class() == scada::NodeClass::Variable);
  return PropertyEditor(PropertyEditor::SIMPLE);
}

base::string16 ReferencePropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  auto target_type_definition =
      GetTargetTypeDefinition(node.type_definition(), prop_decl_id);
  if (!target_type_definition)
    return base::string16();

  if (auto target = node.target(prop_decl_id))
    return target.display_name();
  else
    return kChoiceNone;
}

void ReferencePropertyDefinition::SetText(const PropertyContext& context,
                                          const NodeRef& node,
                                          const scada::NodeId& prop_decl_id,
                                          const base::string16& text) const {
  auto target_type_definition =
      GetTargetTypeDefinition(node.type_definition(), prop_decl_id);
  if (!target_type_definition)
    return;

  NodeRef target;
  if (text != kChoiceNone) {
    target = FindNodeByNameAndType(
        context.node_service_.GetNode(scada::id::RootFolder), text,
        target_type_definition.id());
  }

  auto old_ref_node = node.target(prop_decl_id);
  if (old_ref_node == target)
    return;

  if (old_ref_node) {
    context.task_manager_.PostDeleteReference(prop_decl_id, node.id(),
                                              old_ref_node.id());
  }
  if (target) {
    context.task_manager_.PostAddReference(prop_decl_id, node.id(),
                                           target.id());
  }
}

PropertyEditor ReferencePropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_decl_id) const {
  PropertyEditor result(PropertyEditor::DROPDOWN);

  if (auto target_type_definition =
          GetTargetTypeDefinition(type_definition, prop_decl_id)) {
    result.choices.emplace_back(kChoiceNone);
    GetNodeNamesRecursive(context.node_service_.GetNode(scada::id::RootFolder),
                          target_type_definition.id(), result.choices);
  }

  return result;
}

bool ReferencePropertyDefinition::IsReadOnly(
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  return !GetTargetTypeDefinition(node.type_definition(), prop_decl_id);
}

base::string16 BoolPropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  auto prop = node[prop_decl_id];
  if (!prop)
    return base::string16();

  auto bool_value = prop.value().get_or(false);
  return bool_value ? scada::Variant::kTrueString
                    : scada::Variant::kFalseString;
}

PropertyEditor BoolPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_decl_id) const {
  PropertyEditor result(PropertyEditor::DROPDOWN);
  result.choices = {scada::Variant::kFalseString, scada::Variant::kTrueString};
  return result;
}

base::string16 EnumPropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  auto prop = node[prop_decl_id];
  if (!prop)
    return base::string16();

  int int_value;
  if (!prop.value().get(int_value))
    return base::string16();

  auto enum_strings_value = prop.data_type()[scada::id::EnumStrings].value();
  auto* enum_strings =
      enum_strings_value.get_if<std::vector<scada::LocalizedText>>();
  if (!enum_strings || int_value < 0 || int_value >= enum_strings->size())
    return base::string16();

  return (*enum_strings)[int_value];
}

void EnumPropertyDefinition::SetText(const PropertyContext& context,
                                     const NodeRef& node,
                                     const scada::NodeId& prop_decl_id,
                                     const base::string16& text) const {
  auto prop = node[prop_decl_id];
  if (!prop)
    return;

  auto enum_strings_value = prop.data_type()[scada::id::EnumStrings].value();
  auto* enum_strings =
      enum_strings_value.get_if<std::vector<scada::LocalizedText>>();
  if (!enum_strings)
    return;

  auto i = std::find(enum_strings->begin(), enum_strings->end(), text);
  if (i == enum_strings->end())
    return;

  int int_value = i - enum_strings->begin();
  context.task_manager_.PostUpdateTask(node.id(), {},
                                       {{prop_decl_id, int_value}});
}

PropertyEditor EnumPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_decl_id) const {
  auto prop_decl = type_definition[prop_decl_id];
  if (!prop_decl)
    return PropertyEditor(PropertyEditor::NONE);

  PropertyEditor result(PropertyEditor::DROPDOWN);

  auto enum_strings_value =
      prop_decl.data_type()[scada::id::EnumStrings].value();
  if (auto* enum_strings =
          enum_strings_value.get_if<std::vector<scada::LocalizedText>>()) {
    result.choices = *enum_strings;
  }

  return result;
}

base::string16 ChannelPropertyDefinition::GetTitle(
    const PropertyContext& context,
    const scada::NodeId& prop_decl_id) const {
  return title_;
}

base::string16 ChannelPropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  if (!IsInstanceOf(node, id::DataItemType))
    return base::string16();

  auto channel_path = node[prop_decl_id].value().get_or(std::string());

  scada::NodeId parent_id;
  scada::NodeId node_id;
  base::StringPiece component_name;
  if (IsNodeIdFormula(channel_path, node_id) &&
      IsNestedNodeId(node_id, parent_id, component_name)) {
    return device_
               ? GetFullDisplayName(context.node_service_.GetNode(parent_id))
               : base::SysNativeMBToWide(component_name);
  } else {
    if (device_)
      return base::string16();
    else
      return base::SysNativeMBToWide(channel_path);
  }
}

void ChannelPropertyDefinition::SetText(const PropertyContext& context,
                                        const NodeRef& node,
                                        const scada::NodeId& prop_decl_id,
                                        const base::string16& text) const {
  if (!IsInstanceOf(node, id::DataItemType))
    return;

  scada::NodeId new_device_id;
  if (device_) {
    new_device_id =
        FindNodeByNameAndType(context.node_service_.GetNode(id::Devices), text,
                              id::DeviceType)
            .id();
  }

  auto channel_path = node[prop_decl_id].value().get_or(std::string());
  scada::NodeId node_id;
  scada::NodeId parent_id;
  base::StringPiece component_name;
  if (!IsNodeIdFormula(channel_path, node_id) ||
      !IsNestedNodeId(node_id, parent_id, component_name)) {
    parent_id = scada::NodeId();
    component_name = channel_path;
  }

  auto item_path = component_name.as_string();
  if (device_)
    parent_id = new_device_id;
  else
    item_path = base::SysWideToNativeMB(text);

  std::string formula;
  if (!parent_id.is_null()) {
    if (item_path.empty()) {
      item_path = ToString(
          context.node_service_.GetNode(id::DeviceType_Online).browse_name());
    }
    formula = MakeNodeIdFormula(MakeNestedNodeId(parent_id, item_path));
  } else {
    formula = item_path;
  }

  context.task_manager_.PostUpdateTask(node.id(), {},
                                       {{prop_decl_id, std::move(formula)}});
}

PropertyEditor ChannelPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_decl_id) const {
  if (device_) {
    PropertyEditor result(PropertyEditor::DROPDOWN);
    result.choices.emplace_back(kChoiceNone);
    GetNodeNamesRecursive(context.node_service_.GetNode(id::Devices),
                          id::DeviceType, result.choices);
    return result;
  }

  return PropertyEditor(PropertyEditor::SIMPLE);
}

PropertyEditor TransportPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_decl_id) const {
  return PropertyEditor(PropertyEditor::BUTTON);
}

base::string16 ColorPropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  auto value = node[prop_decl_id].value();
  auto color_index = value.get_or(-1);
  if (color_index >= 0 && color_index < palette::GetColorCount())
    return palette::GetColorName(color_index);
  else
    return kDefaultColorString;
}

void ColorPropertyDefinition::SetText(const PropertyContext& context,
                                      const NodeRef& node,
                                      const scada::NodeId& prop_decl_id,
                                      const base::string16& text) const {
  if (text.empty())
    return;

  int color = palette::FindColorName(text.c_str());
  context.task_manager_.PostUpdateTask(node.id(), {}, {{prop_decl_id, color}});
}

PropertyEditor ColorPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_decl_id) const {
  PropertyEditor result(PropertyEditor::DROPDOWN);
  result.choices.reserve(1 + palette::GetColorCount());
  result.choices.emplace_back(kDefaultColorString);
  for (size_t i = 0; i < palette::GetColorCount(); i++)
    result.choices.emplace_back(palette::GetColorName(i));
  return result;
}
