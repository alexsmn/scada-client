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
#include "components/transport/transport_dialog.h"
#include "core/node_management_service.h"
#include "net/transport_string.h"
#include "services/task_manager.h"

namespace {

static const base::char16 kDefaultColorString[] = L"<Стандартный>";
static const base::char16 kChoiceNone[] = L"<Нет>";

NodeRef FindNodeByNameAndType(const NodeRef& parent_node,
                              const base::StringPiece16& name,
                              const scada::NodeId& node_type_id) {
  for (const auto& node : parent_node.targets(scada::id::Organizes)) {
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
  for (const auto& node : parent_node.targets(scada::id::Organizes)) {
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
    {id::DataItemType_Severity, &kIntPropDef},
    {id::DeviceType_Disabled, &kBoolPropDef},
    {id::HasSimulationSignal, &kRefPropDef},
    // DataGroup
    {id::DataGroupType_Simulated, &kBoolPropDef},
    // DataItem
    {id::DataItemType_Simulated, &kBoolPropDef},
    {id::DataItemType_Alias, &kStringPropDef},
    {id::DataItemType_Input1, &kObjectInput1PropDef},
    {id::DataItemType_Input2, &kObjectInput2PropDef},
    {id::DataItemType_Output, &kObjectOutputPropDef},
    {id::DataItemType_OutputCondition, &kStringPropDef},
    {id::DiscreteItemType_Inversion, &kBoolPropDef},
    {id::AnalogItemType_DisplayFormat, &kStringPropDef},     // TODO: Editor
    {id::AnalogItemType_EngineeringUnits, &kStringPropDef},  // TODO: Combo
    {id::AnalogItemType_Aperture, &kDoublePropDef},
    {id::AnalogItemType_Deadband, &kDoublePropDef},
    {id::HasTsFormat, &kRefPropDef},
    {id::AnalogItemType_Conversion, &kEnumPropDef},  // TODO: Enum
    {id::AnalogItemType_EuLo, &kDoublePropDef},
    {id::AnalogItemType_EuHi, &kDoublePropDef},
    {id::AnalogItemType_IrLo, &kDoublePropDef},
    {id::AnalogItemType_IrHi, &kDoublePropDef},
    {id::AnalogItemType_LimitLoLo, &kDoublePropDef},
    {id::AnalogItemType_LimitLo, &kDoublePropDef},
    {id::AnalogItemType_LimitHi, &kDoublePropDef},
    {id::AnalogItemType_LimitHiHi, &kDoublePropDef},
    {id::DataItemType_StalePeriod, &kIntPropDef},
    {id::HasHistoricalDatabase, &kRefPropDef},
    {id::AnalogItemType_Clamping, &kEnumPropDef},  // TODO: Enum
    // Link
    {id::LinkType_Transport, &kLinkTransportPropDef},
    {id::Iec60870LinkType_SendQueueSize, &kIntPropDef},
    {id::Iec60870LinkType_ReceiveQueueSize, &kIntPropDef},
    {id::Iec60870LinkType_ConfirmationTimeout, &kIntPropDef},  // time delta
    {id::Iec60870LinkType_TerminationTimeout, &kIntPropDef},   // time delta
    // IEC-60870 Link
    {id::Iec60870LinkType_Protocol, &kIntPropDef},  // TODO: Enum
    {id::Iec60870LinkType_Mode, &kIntPropDef},      // TODO: Enum
    {id::Iec60870LinkType_ConnectTimeout, &kIntPropDef},
    {id::Iec60870LinkType_DeviceAddressSize, &kIntPropDef},
    {id::Iec60870LinkType_COTSize, &kIntPropDef},
    {id::Iec60870LinkType_InfoAddressSize, &kIntPropDef},
    {id::Iec60870LinkType_DataCollection, &kBoolPropDef},
    {id::Iec60870LinkType_SendRetryCount, &kIntPropDef},
    {id::Iec60870LinkType_CRCProtection, &kBoolPropDef},
    {id::Iec60870LinkType_SendTimeout, &kIntPropDef},
    {id::Iec60870LinkType_ReceiveTimeout, &kIntPropDef},
    {id::Iec60870LinkType_IdleTimeout, &kIntPropDef},
    {id::Iec60870LinkType_AnonymousMode, &kBoolPropDef},
    // IEC-60870 Device
    {id::Iec60870DeviceType_Address, &kIntPropDef},
    {id::Iec60870DeviceType_LinkAddress, &kIntPropDef},
    {id::Iec60870DeviceType_StartupInterrogation, &kBoolPropDef},
    {id::Iec60870DeviceType_InterrogationPeriod, &kIntPropDef},  // time delta
    {id::Iec60870DeviceType_StartupClockSync, &kBoolPropDef},
    {id::Iec60870DeviceType_ClockSyncPeriod, &kIntPropDef},
    {id::Iec60870DeviceType_UtcTime, &kBoolPropDef},
    {id::Iec60870DeviceType_InterrogationPeriodGroup1, &kIntPropDef},
    {id::Iec60870DeviceType_InterrogationPeriodGroup2, &kIntPropDef},
    {id::Iec60870DeviceType_InterrogationPeriodGroup3, &kIntPropDef},
    {id::Iec60870DeviceType_InterrogationPeriodGroup4, &kIntPropDef},
    {id::Iec60870DeviceType_InterrogationPeriodGroup5, &kIntPropDef},
    {id::Iec60870DeviceType_InterrogationPeriodGroup6, &kIntPropDef},
    {id::Iec60870DeviceType_InterrogationPeriodGroup7, &kIntPropDef},
    {id::Iec60870DeviceType_InterrogationPeriodGroup8, &kIntPropDef},
    {id::Iec60870DeviceType_InterrogationPeriodGroup9, &kIntPropDef},
    {id::Iec60870DeviceType_InterrogationPeriodGroup10, &kIntPropDef},
    {id::Iec60870DeviceType_InterrogationPeriodGroup11, &kIntPropDef},
    {id::Iec60870DeviceType_InterrogationPeriodGroup12, &kIntPropDef},
    {id::Iec60870DeviceType_InterrogationPeriodGroup13, &kIntPropDef},
    {id::Iec60870DeviceType_InterrogationPeriodGroup14, &kIntPropDef},
    {id::Iec60870DeviceType_InterrogationPeriodGroup15, &kIntPropDef},
    {id::Iec60870DeviceType_InterrogationPeriodGroup16, &kIntPropDef},
    // Modbus Link
    {id::ModbusLinkType_Protocol, &kIntPropDef},  // TODO: Enum
    // Modbus Dev
    {id::ModbusDeviceType_Address, &kIntPropDef},
    {id::ModbusDeviceType_SendRetryCount, &kIntPropDef},
    {id::ModbusDeviceType_ResponseTimeout, &kIntPropDef},
    // IEC-81650 Device
    {id::Iec61850DeviceType_Host, &kStringPropDef},
    {id::Iec61850DeviceType_Port, &kIntPropDef},
    // IEC-81650 ConfigurableObject
    {id::Iec61850ConfigurableObjectType_Reference, &kStringPropDef},
    // Sim Signal
    {id::SimulationSignalType_Type, &kIntPropDef},            // TODO: Enum
    {id::SimulationSignalType_Period, &kIntPropDef},          // time delta
    {id::SimulationSignalType_Phase, &kIntPropDef},           // time delta
    {id::SimulationSignalType_UpdateInterval, &kIntPropDef},  // time delta
    // User
    {id::UserType_AccessRights, &kIntPropDef},  // TODO: Set
    // TsFormat
    {id::TsFormatType_OpenLabel, &kStringPropDef},
    {id::TsFormatType_CloseLabel, &kStringPropDef},
    {id::TsFormatType_OpenColor, &kColorPropDef},
    {id::TsFormatType_CloseColor, &kColorPropDef},
    // Historical DB
    {id::HistoricalDatabaseType_Depth, &kIntPropDef},  // time delta
};

}  // namespace

const PropertyDefinition* GetPropertyDef(const scada::NodeId& prop_decl_id) {
  auto i = kPropertyDefinitionMap.find(prop_decl_id);
  return i == kPropertyDefinitionMap.end() ? nullptr : i->second;
}

PropertyDefs GetTypeProperties(const NodeRef& type_definition) {
  PropertyDefs properties;
  properties.reserve(32);
  for (auto supertype = type_definition; supertype;
       supertype = supertype.supertype()) {
    for (const auto& p : supertype.targets(scada::id::HasProperty)) {
      if (auto* def = GetPropertyDef(p.node_id()))
        properties.emplace_back(p, def);
    }
    for (const auto& r : supertype.references()) {
      if (IsSubtypeOf(r.reference_type, scada::id::HasProperty))
        continue;
      if (auto* def = GetPropertyDef(r.reference_type.node_id()))
        properties.emplace_back(r.reference_type, def);
    }
  }
  return properties;
}

PropertyDefinition::PropertyDefinition(ui::TableColumn::Alignment alignment,
                                       int width)
    : alignment_(alignment), width_(width) {}

base::string16 PropertyDefinition::GetTitle(
    const PropertyContext& context,
    const NodeRef& property_declaration) const {
  return ToString16(property_declaration.display_name());
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
  const auto& property = node[prop_decl_id];
  if (!property)
    return;

  scada::Variant value;
  if (!StringToValue(text, property.data_type().node_id(), value))
    return;

  context.task_manager_.PostUpdateTask(node.node_id(), {},
                                       {{prop_decl_id, std::move(value)}});
}

ui::EditData PropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_decl_id) const {
  const auto& property_declaration = type_definition[prop_decl_id];
  if (!property_declaration)
    return ui::EditData{ui::EditData::EditorType::NONE};

  assert(property_declaration.node_class() == scada::NodeClass::Variable);
  return ui::EditData{ui::EditData::EditorType::TEXT};
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

ui::EditData ReferencePropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_decl_id) const {
  ui::EditData result{ui::EditData::EditorType::DROPDOWN};

  if (auto target_type_definition =
          GetTargetTypeDefinition(type_definition, prop_decl_id)) {
    result.choices.emplace_back(kChoiceNone);
    GetNodeNamesRecursive(
        context.node_service_.GetNode(scada::id::ObjectsFolder),
        target_type_definition.node_id(), result.choices);
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

ui::EditData BoolPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_decl_id) const {
  ui::EditData result{ui::EditData::EditorType::DROPDOWN};
  result.choices = {scada::Variant::kFalseString, scada::Variant::kTrueString};
  return result;
}

base::string16 EnumPropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  const auto& property = node[prop_decl_id];
  if (!property)
    return base::string16();

  int int_value;
  if (!property.value().get(int_value))
    return base::string16();

  auto enum_strings_value =
      property.data_type()[scada::id::EnumStrings].value();
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
  const auto& property = node[prop_decl_id];
  if (!property)
    return;

  auto enum_strings_value =
      property.data_type()[scada::id::EnumStrings].value();
  auto* enum_strings =
      enum_strings_value.get_if<std::vector<scada::LocalizedText>>();
  if (!enum_strings)
    return;

  auto i = std::find(enum_strings->begin(), enum_strings->end(), text);
  if (i == enum_strings->end())
    return;

  int int_value = i - enum_strings->begin();
  context.task_manager_.PostUpdateTask(node.node_id(), {},
                                       {{prop_decl_id, int_value}});
}

ui::EditData EnumPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_decl_id) const {
  const auto& property_declaration = type_definition[prop_decl_id];
  if (!property_declaration)
    return ui::EditData{ui::EditData::EditorType::NONE};

  ui::EditData result{ui::EditData::EditorType::DROPDOWN};

  auto enum_strings_value =
      property_declaration.data_type()[scada::id::EnumStrings].value();
  if (auto* enum_strings =
          enum_strings_value.get_if<std::vector<scada::LocalizedText>>()) {
    result.choices = *enum_strings;
  }

  return result;
}

base::string16 ChannelPropertyDefinition::GetTitle(
    const PropertyContext& context,
    const NodeRef& property_declaration) const {
  return title_;
}

base::string16 ChannelPropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  if (!IsInstanceOf(node, id::DataItemType))
    return base::string16();

  auto channel_path = node[prop_decl_id].value().get_or(std::string{});

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
            .node_id();
  }

  auto channel_path = node[prop_decl_id].value().get_or(std::string{});
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

  context.task_manager_.PostUpdateTask(node.node_id(), {},
                                       {{prop_decl_id, std::move(formula)}});
}

ui::EditData ChannelPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_decl_id) const {
  if (device_) {
    ui::EditData result{ui::EditData::EditorType::DROPDOWN};
    result.choices.emplace_back(kChoiceNone);
    GetNodeNamesRecursive(context.node_service_.GetNode(id::Devices),
                          id::DeviceType, result.choices);
    return result;
  }

  return ui::EditData{ui::EditData::EditorType::TEXT};
}

ui::EditData TransportPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_decl_id) const {
  ui::EditData data{ui::EditData::EditorType::BUTTON};

  data.action_handler = [& dialog_service =
                             context.dialog_service_](base::string16& text) {
    net::TransportString transport_string{base::SysWideToNativeMB(text)};
    if (!ShowTransportDialog(dialog_service, transport_string))
      return false;
    text = base::SysNativeMBToWide(transport_string.ToString());
    return true;
  };

  return data;
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
  context.task_manager_.PostUpdateTask(node.node_id(), {},
                                       {{prop_decl_id, color}});
}

ui::EditData ColorPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_decl_id) const {
  ui::EditData result{ui::EditData::EditorType::DROPDOWN};
  result.choices.reserve(1 + palette::GetColorCount());
  result.choices.emplace_back(kDefaultColorString);
  for (size_t i = 0; i < palette::GetColorCount(); i++)
    result.choices.emplace_back(palette::GetColorName(i));
  return result;
}
