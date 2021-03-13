#include "property_defs.h"

#include "base/string_piece_util.h"
#include "base/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "common/format.h"
#include "common/formula_util.h"
#include "components/transport/transport_dialog.h"
#include "controls/color.h"
#include "core/node_management_service.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/history_node_ids.h"
#include "model/node_id_util.h"
#include "model/security_node_ids.h"
#include "net/transport_string.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "services/task_manager.h"

#include <iterator>
#include <map>

namespace {

static const wchar_t kDefaultColorString[] = L"<Стандартный>";
static const wchar_t kChoiceNone[] = L"<Нет>";

NodeRef FindNodeByNameAndType(const NodeRef& parent_node,
                              const std::wstring_view& name,
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
                           std::vector<std::wstring>& names) {
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

std::pair<scada::NodeId /*parent_id*/, std::string /*component_name*/>
ParseChannelPath(std::string_view channel_path) {
  scada::NodeId node_id;
  scada::NodeId parent_id;
  std::string_view component_name;
  if (!IsNodeIdFormula(channel_path, node_id) ||
      !IsNestedNodeId(node_id, parent_id, component_name)) {
    parent_id = scada::NodeId();
    component_name = channel_path;
  }
  // Note: can't return string_view since |node_id| goes out of scope.
  return {parent_id, std::string{component_name}};
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
    {data_items::id::DataItemType_Severity, &kIntPropDef},
    {devices::id::DeviceType_Disabled, &kBoolPropDef},
    {data_items::id::HasSimulationSignal, &kRefPropDef},
    // DataGroup
    {data_items::id::DataGroupType_Simulated, &kBoolPropDef},
    // DataItem
    {data_items::id::DataItemType_Simulated, &kBoolPropDef},
    {data_items::id::DataItemType_Alias, &kStringPropDef},
    {data_items::id::DataItemType_Input1, &kObjectInput1PropDef},
    {data_items::id::DataItemType_Input2, &kObjectInput2PropDef},
    {data_items::id::DataItemType_Output, &kObjectOutputPropDef},
    {data_items::id::DataItemType_OutputCondition, &kStringPropDef},
    {data_items::id::DataItemType_OutputTwoStaged, &kBoolPropDef},
    {data_items::id::DiscreteItemType_Inversion, &kBoolPropDef},
    {data_items::id::AnalogItemType_DisplayFormat,
     &kStringPropDef},  // TODO: Editor
    {data_items::id::AnalogItemType_EngineeringUnits,
     &kStringPropDef},  // TODO: Combo
    {data_items::id::AnalogItemType_Aperture, &kDoublePropDef},
    {data_items::id::AnalogItemType_Deadband, &kDoublePropDef},
    {data_items::id::HasTsFormat, &kRefPropDef},
    {data_items::id::AnalogItemType_Conversion, &kEnumPropDef},
    {data_items::id::AnalogItemType_EuLo, &kDoublePropDef},
    {data_items::id::AnalogItemType_EuHi, &kDoublePropDef},
    {data_items::id::AnalogItemType_IrLo, &kDoublePropDef},
    {data_items::id::AnalogItemType_IrHi, &kDoublePropDef},
    {data_items::id::AnalogItemType_LimitLoLo, &kDoublePropDef},
    {data_items::id::AnalogItemType_LimitLo, &kDoublePropDef},
    {data_items::id::AnalogItemType_LimitHi, &kDoublePropDef},
    {data_items::id::AnalogItemType_LimitHiHi, &kDoublePropDef},
    {data_items::id::DataItemType_StalePeriod, &kIntPropDef},
    {history::id::HasHistoricalDatabase, &kRefPropDef},
    {data_items::id::AnalogItemType_Clamping, &kEnumPropDef},
    {data_items::id::HasDevice, &kRefPropDef},
    // Link
    {devices::id::LinkType_Transport, &kLinkTransportPropDef},
    {devices::id::Iec60870LinkType_SendQueueSize, &kIntPropDef},
    {devices::id::Iec60870LinkType_ReceiveQueueSize, &kIntPropDef},
    {devices::id::Iec60870LinkType_ConfirmationTimeout,
     &kIntPropDef},  // time delta
    {devices::id::Iec60870LinkType_TerminationTimeout,
     &kIntPropDef},  // time delta
    // IEC-60870 Link
    {devices::id::Iec60870LinkType_Protocol, &kEnumPropDef},
    {devices::id::Iec60870LinkType_Mode, &kEnumPropDef},
    {devices::id::Iec60870LinkType_ConnectTimeout, &kIntPropDef},
    {devices::id::Iec60870LinkType_DeviceAddressSize, &kIntPropDef},
    {devices::id::Iec60870LinkType_COTSize, &kIntPropDef},
    {devices::id::Iec60870LinkType_InfoAddressSize, &kIntPropDef},
    {devices::id::Iec60870LinkType_DataCollection, &kBoolPropDef},
    {devices::id::Iec60870LinkType_SendRetryCount, &kIntPropDef},
    {devices::id::Iec60870LinkType_CRCProtection, &kBoolPropDef},
    {devices::id::Iec60870LinkType_SendTimeout, &kIntPropDef},
    {devices::id::Iec60870LinkType_ReceiveTimeout, &kIntPropDef},
    {devices::id::Iec60870LinkType_IdleTimeout, &kIntPropDef},
    {devices::id::Iec60870LinkType_AnonymousMode, &kBoolPropDef},
    // IEC-60870 Device
    {devices::id::Iec60870DeviceType_Address, &kIntPropDef},
    {devices::id::Iec60870DeviceType_LinkAddress, &kIntPropDef},
    {devices::id::Iec60870DeviceType_StartupInterrogation, &kBoolPropDef},
    {devices::id::Iec60870DeviceType_InterrogationPeriod,
     &kIntPropDef},  // time delta
    {devices::id::Iec60870DeviceType_StartupClockSync, &kBoolPropDef},
    {devices::id::Iec60870DeviceType_ClockSyncPeriod, &kIntPropDef},
    {devices::id::Iec60870DeviceType_UtcTime, &kBoolPropDef},
    {devices::id::Iec60870DeviceType_InterrogationPeriodGroup1, &kIntPropDef},
    {devices::id::Iec60870DeviceType_InterrogationPeriodGroup2, &kIntPropDef},
    {devices::id::Iec60870DeviceType_InterrogationPeriodGroup3, &kIntPropDef},
    {devices::id::Iec60870DeviceType_InterrogationPeriodGroup4, &kIntPropDef},
    {devices::id::Iec60870DeviceType_InterrogationPeriodGroup5, &kIntPropDef},
    {devices::id::Iec60870DeviceType_InterrogationPeriodGroup6, &kIntPropDef},
    {devices::id::Iec60870DeviceType_InterrogationPeriodGroup7, &kIntPropDef},
    {devices::id::Iec60870DeviceType_InterrogationPeriodGroup8, &kIntPropDef},
    {devices::id::Iec60870DeviceType_InterrogationPeriodGroup9, &kIntPropDef},
    {devices::id::Iec60870DeviceType_InterrogationPeriodGroup10, &kIntPropDef},
    {devices::id::Iec60870DeviceType_InterrogationPeriodGroup11, &kIntPropDef},
    {devices::id::Iec60870DeviceType_InterrogationPeriodGroup12, &kIntPropDef},
    {devices::id::Iec60870DeviceType_InterrogationPeriodGroup13, &kIntPropDef},
    {devices::id::Iec60870DeviceType_InterrogationPeriodGroup14, &kIntPropDef},
    {devices::id::Iec60870DeviceType_InterrogationPeriodGroup15, &kIntPropDef},
    {devices::id::Iec60870DeviceType_InterrogationPeriodGroup16, &kIntPropDef},
    // Modbus Link
    {devices::id::ModbusLinkType_Protocol, &kEnumPropDef},
    {devices::id::ModbusLinkType_RequestDelay, &kIntPropDef},
    // Modbus Dev
    {devices::id::ModbusDeviceType_Address, &kIntPropDef},
    {devices::id::ModbusDeviceType_SendRetryCount, &kIntPropDef},
    {devices::id::ModbusDeviceType_ResponseTimeout, &kIntPropDef},
    // IEC-81650 Device
    {devices::id::Iec61850DeviceType_Host, &kStringPropDef},
    {devices::id::Iec61850DeviceType_Port, &kIntPropDef},
    // IEC-81650 ConfigurableObject
    {devices::id::Iec61850ConfigurableObjectType_Reference, &kStringPropDef},
    // Sim Signal
    {data_items::id::SimulationSignalType_Function, &kEnumPropDef},
    {data_items::id::SimulationSignalType_Period, &kIntPropDef},  // time delta
    {data_items::id::SimulationSignalType_Phase, &kIntPropDef},   // time delta
    {data_items::id::SimulationSignalType_UpdateInterval,
     &kIntPropDef},  // time delta
    // User
    {security::id::UserType_AccessRights, &kIntPropDef},  // TODO: Set
    // TsFormat
    {data_items::id::TsFormatType_OpenLabel, &kStringPropDef},
    {data_items::id::TsFormatType_CloseLabel, &kStringPropDef},
    {data_items::id::TsFormatType_OpenColor, &kColorPropDef},
    {data_items::id::TsFormatType_CloseColor, &kColorPropDef},
    // Historical DB
    {history::id::HistoricalDatabaseType_Depth, &kIntPropDef},  // time delta
    // Transmission Item
    {devices::id::TransmissionItemType_SourceAddress, &kIntPropDef},
    {devices::id::HasTransmissionSource, &kRefPropDef},
    {devices::id::HasTransmissionTarget, &kRefPropDef},
};

}  // namespace

const PropertyDefinition* GetPropertyDef(const scada::NodeId& prop_decl_id) {
  auto i = kPropertyDefinitionMap.find(prop_decl_id);
  return i == kPropertyDefinitionMap.end() ? nullptr : i->second;
}

PropertyDefs GetTypeProperties(const NodeRef& type_definition) {
  PropertyDefs properties;
  properties.reserve(32);

  std::vector<NodeRef> type_definitions;
  for (auto supertype = type_definition; supertype;
       supertype = supertype.supertype())
    type_definitions.emplace_back(supertype);

  std::reverse(type_definitions.begin(), type_definitions.end());

  for (const auto supertype : type_definitions) {
    assert(supertype.fetched());
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

std::wstring PropertyDefinition::GetTitle(
    const PropertyContext& context,
    const NodeRef& property_declaration) const {
  return ToString16(property_declaration.display_name());
}

bool PropertyDefinition::IsReadOnly(const NodeRef& node,
                                    const scada::NodeId& prop_decl_id) const {
  return !node[prop_decl_id];
}

std::wstring PropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  auto value = node[prop_decl_id].value();
  return {ToString16(value), false};
}

void PropertyDefinition::SetText(const PropertyContext& context,
                                 const NodeRef& node,
                                 const scada::NodeId& prop_decl_id,
                                 const std::wstring& text) const {
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
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  const auto& type_definition = node.type_definition();
  const auto& property_declaration = type_definition[prop_decl_id];
  if (!property_declaration)
    return ui::EditData{ui::EditData::EditorType::NONE};

  assert(property_declaration.node_class() == scada::NodeClass::Variable);
  return ui::EditData{ui::EditData::EditorType::TEXT};
}

std::wstring ReferencePropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  auto target_type_definition =
      GetTargetTypeDefinition(node.type_definition(), prop_decl_id);
  if (!target_type_definition)
    return std::wstring();

  if (auto target = node.target(prop_decl_id))
    return target.display_name();
  else
    return kChoiceNone;
}

void ReferencePropertyDefinition::SetText(const PropertyContext& context,
                                          const NodeRef& node,
                                          const scada::NodeId& prop_decl_id,
                                          const std::wstring& text) const {
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
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  ui::EditData result{ui::EditData::EditorType::DROPDOWN};

  const auto& type_definition = node.type_definition();
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

std::wstring BoolPropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  auto prop = node[prop_decl_id];
  if (!prop)
    return std::wstring();

  auto bool_value = prop.value().get_or(false);
  return bool_value ? scada::Variant::kTrueString
                    : scada::Variant::kFalseString;
}

ui::EditData BoolPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  ui::EditData result{ui::EditData::EditorType::DROPDOWN};
  result.choices = {scada::Variant::kFalseString, scada::Variant::kTrueString};
  return result;
}

std::wstring EnumPropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  const auto& property = node[prop_decl_id];
  if (!property)
    return std::wstring();

  int int_value;
  if (!property.value().get(int_value))
    return std::wstring();

  auto enum_strings_value = property.data_type()["EnumStrings"].value();
  auto* enum_strings =
      enum_strings_value.get_if<std::vector<scada::LocalizedText>>();
  if (!enum_strings || int_value < 0 ||
      int_value >= static_cast<int>(enum_strings->size()))
    return std::wstring();

  return (*enum_strings)[int_value];
}

void EnumPropertyDefinition::SetText(const PropertyContext& context,
                                     const NodeRef& node,
                                     const scada::NodeId& prop_decl_id,
                                     const std::wstring& text) const {
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

  int int_value = i - enum_strings->begin();
  context.task_manager_.PostUpdateTask(node.node_id(), {},
                                       {{prop_decl_id, int_value}});
}

ui::EditData EnumPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  const auto& type_definition = node.type_definition();
  const auto& property_declaration = type_definition[prop_decl_id];
  if (!property_declaration)
    return ui::EditData{ui::EditData::EditorType::NONE};

  ui::EditData result{ui::EditData::EditorType::DROPDOWN};

  auto enum_strings_value =
      property_declaration.data_type()["EnumStrings"].value();
  if (auto* enum_strings =
          enum_strings_value.get_if<std::vector<scada::LocalizedText>>()) {
    result.choices = *enum_strings;
  }

  return result;
}

std::wstring ChannelPropertyDefinition::GetTitle(
    const PropertyContext& context,
    const NodeRef& property_declaration) const {
  return title_;
}

std::wstring ChannelPropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  if (!IsInstanceOf(node, data_items::id::DataItemType))
    return std::wstring();

  auto channel_path = node[prop_decl_id].value().get_or(std::string{});

  scada::NodeId parent_id;
  scada::NodeId node_id;
  std::string_view component_name;
  if (IsNodeIdFormula(channel_path, node_id) &&
      IsNestedNodeId(node_id, parent_id, component_name)) {
    return device_
               ? GetFullDisplayName(context.node_service_.GetNode(parent_id))
               : base::SysNativeMBToWide(ToStringPiece(component_name));
  } else {
    if (device_)
      return std::wstring{};
    else
      return base::SysNativeMBToWide(channel_path);
  }
}

void ChannelPropertyDefinition::SetText(const PropertyContext& context,
                                        const NodeRef& node,
                                        const scada::NodeId& prop_decl_id,
                                        const std::wstring& text) const {
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
    item_path = base::SysWideToNativeMB(text);

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

ui::EditData ChannelPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  ui::EditData result{ui::EditData::EditorType::DROPDOWN};

  if (device_) {
    result.choices.emplace_back(kChoiceNone);
    GetNodeNamesRecursive(context.node_service_.GetNode(devices::id::Devices),
                          devices::id::DeviceType, result.choices);

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

ui::EditData TransportPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  ui::EditData data{ui::EditData::EditorType::BUTTON};

  data.action_handler = [&dialog_service =
                             context.dialog_service_](std::wstring& text) {
    net::TransportString transport_string{base::SysWideToNativeMB(text)};
    if (!ShowTransportDialog(dialog_service, transport_string))
      return false;
    text = base::SysNativeMBToWide(transport_string.ToString());
    return true;
  };

  return data;
}

std::wstring ColorPropertyDefinition::GetText(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  auto value = node[prop_decl_id].value();
  auto color_index = value.get_or(-1);
  if (color_index >= 0 && color_index < static_cast<int>(aui::GetColorCount()))
    return std::wstring{aui::GetColorName(color_index)};
  else
    return kDefaultColorString;
}

void ColorPropertyDefinition::SetText(const PropertyContext& context,
                                      const NodeRef& node,
                                      const scada::NodeId& prop_decl_id,
                                      const std::wstring& text) const {
  if (text.empty())
    return;

  int color = aui::FindColorName(text.c_str());
  context.task_manager_.PostUpdateTask(node.node_id(), {},
                                       {{prop_decl_id, color}});
}

ui::EditData ColorPropertyDefinition::GetPropertyEditor(
    const PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  ui::EditData result{ui::EditData::EditorType::DROPDOWN};
  result.choices.reserve(1 + aui::GetColorCount());
  result.choices.emplace_back(kDefaultColorString);
  for (size_t i = 0; i < aui::GetColorCount(); i++)
    result.choices.emplace_back(aui::GetColorName(i));
  return result;
}
