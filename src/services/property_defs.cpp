#include "services/property_defs.h"

#include <algorithm>
#include <iterator>
#include <map>

#include "base/color.h"
#include "base/strings/sys_string_conversions.h"
#include "common/format.h"
#include "common/formula_util.h"
#include "common/node_id_util.h"
#include "common/node_ref.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "common/scada_node_ids.h"
#include "core/node_management_service.h"
#include "services/task_manager.h"
#include "translation.h"

namespace {

// TODO:
static const scada::QualifiedName kEnumStrings{"EnumStrings", 0};

static const base::char16 kDefaultColorString[] = L"<Стандартный>";
static const base::char16 kChoiceNone[] = L"<Нет>";

std::vector<base::StringPiece> SplitEnumStrings(base::StringPiece enum_string) {
  // TODO:
  return {};
}

bool ParseEnumStrings(const std::vector<base::StringPiece>& enum_strings,
                      int& value) {
  // TODO:
  return false;
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

std::map<scada::NodeId, const PropertyDefinition*> kPropertyDefinitionMap = {
    // Default
    {id::DataItemType_Severity, &kIntPropDef},
    {id::DeviceType_Disabled, &kBoolPropDef},
    {id::DataGroupType_Simulated, &kBoolPropDef},
    {id::DataItemType_Simulated, &kBoolPropDef},
    {id::HasSimulationSignal, &kRefPropDef},
    // Item
    {id::DataItemType_Alias, &kStringPropDef},
    {id::DataItemType_Input1, &kObjectInput1PropDef},
    {id::DataItemType_Input2, &kObjectInput2PropDef},
    {id::DataItemType_Output, &kObjectOutputPropDef},
    {id::DataItemType_OutputCondition, &kStringPropDef},
    {id::DiscreteItemType_Inversion, &kBoolPropDef},
    {id::AnalogItemType_DisplayFormat, &kStringPropDef},     // TODO: Editor
    {id::AnalogItemType_EngineeringUnits, &kStringPropDef},  // TODO: Combo
    {id::HasTsFormat, &kRefPropDef},
    {id::AnalogItemType_Conversion, &kEnumPropDef},
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
    {id::AnalogItemType_Clamping, &kEnumPropDef},
    // Link
    {id::LinkType_Transport, &kLinkTransportPropDef},
    {id::Iec60870LinkType_SendQueueSize, &kIntPropDef},
    {id::Iec60870LinkType_ReceiveQueueSize, &kIntPropDef},
    {id::Iec60870LinkType_ConfirmationTimeout, &kIntPropDef},  // time delta
    {id::Iec60870LinkType_TerminationTimeout, &kIntPropDef},   // time delta
    // IEC-61870 Link
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
    // IEC-61870 Device
    {id::Iec60870DeviceType_Address, &kIntPropDef},
    {id::Iec60870DeviceType_LinkAddress, &kIntPropDef},
    {id::Iec60870DeviceType_StartupInterrogation, &kBoolPropDef},
    {id::Iec60870DeviceType_InterrogationPeriod, &kIntPropDef},  // time delta
    {id::Iec60870DeviceType_StartupClockSync, &kBoolPropDef},
    {id::Iec60870DeviceType_ClockSyncPeriod, &kIntPropDef},
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
    {id::ModbusLinkType_Mode, &kIntPropDef},  // TODO: Enum
    // Modbus Dev
    {id::ModbusDeviceType_Address, &kIntPropDef},
    {id::ModbusDeviceType_SendRetryCount, &kIntPropDef},
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

const PropertyDefinition* GetPropertyDef(const scada::NodeId& prop_type_id) {
  auto i = kPropertyDefinitionMap.find(prop_type_id);
  return i == kPropertyDefinitionMap.end() ? nullptr : i->second;
}

PropertyDefs GetTypeProperties(const NodeRef& type_definition) {
  PropertyDefs properties;
  for (auto supertype = type_definition; supertype;
       supertype = supertype.supertype()) {
    for (auto& p : supertype.properties()) {
      if (auto* def = GetPropertyDef(p.id()))
        properties.emplace_back(p, def);
    }
    for (auto& r : supertype.references()) {
      if (auto* def = GetPropertyDef(r.reference_type.id()))
        properties.emplace_back(r.reference_type, def);
    }
  }
  return properties;
}

PropertyDefinition::PropertyDefinition(ui::TableColumn::Alignment alignment,
                                       int width)
    : alignment_(alignment), width_(width) {}

base::string16 PropertyDefinition::GetTitle(
    PropertyContext& context,
    const NodeRef& property_declaration) const {
  return ToString16(property_declaration.display_name());
}

bool PropertyDefinition::IsReadOnly(const NodeRef& node,
                                    const scada::NodeId& prop_decl_id) const {
  auto type_definition = node.type_definition();
  if (!type_definition)
    return true;

  return !type_definition[prop_decl_id] &&
         !type_definition.target(prop_decl_id);
}

base::string16 PropertyDefinition::GetText(
    PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_type_id) const {
  auto value = node[prop_type_id].value();
  return {base::SysNativeMBToWide(value.get_or(std::string{})), false};
}

void PropertyDefinition::SetText(PropertyContext& context,
                                 const NodeRef& node,
                                 const scada::NodeId& prop_decl_id,
                                 const base::string16& text) const {
  const auto& type_definition = node.type_definition();
  if (type_definition)
    return;

  const auto& property_declaration =
      type_definition.GetAggregateDeclaration(prop_decl_id);
  if (!property_declaration)
    return;

  scada::Variant value;
  if (!StringToValue(base::SysWideToNativeMB(text).c_str(),
                     property_declaration.data_type().id(), value))
    return;

  context.task_manager_.PostUpdateTask(node.id(), {},
                                       {{prop_decl_id, std::move(value)}});
}

PropertyEditor PropertyDefinition::GetPropertyEditor(
    PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_decl_id) const {
  auto property_declaration =
      type_definition.GetAggregateDeclaration(prop_decl_id);
  if (!property_declaration)
    return PropertyEditor(PropertyEditor::NONE);

  assert(property_declaration.node_class() == scada::NodeClass::Variable);
  return PropertyEditor(PropertyEditor::SIMPLE);
}

base::string16 ReferencePropertyDefinition::GetText(
    PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_type_id) const {
  auto type_definition = node.type_definition();
  auto reference_target_type = type_definition.target(prop_type_id);
  if (!reference_target_type)
    return base::string16();

  auto referenced_node = node.target(prop_type_id);
  if (referenced_node)
    return ToString16(referenced_node.display_name());
  else
    return kChoiceNone;
}

void ReferencePropertyDefinition::SetText(PropertyContext& context,
                                          const NodeRef& node,
                                          const scada::NodeId& prop_type_id,
                                          const base::string16& text) const {
  const auto& type_definition = node.type_definition();
  const auto& reference_type = type_definition.target(prop_type_id);
  if (!reference_type)
    return;

  /*auto& node_management_service = context.node_management_service_;
  FindNodeRecursive(context.node_service_, id::RootFolder,
  base::SysWideToNativeMB(text), reference_type.id(), [node, prop_type_id,
  &node_management_service](NodeRef new_target) { const auto& old_target =
  node.reference(prop_type_id); if (old_target == new_target) return;

        if (old_target) {
          node_management_service.DeleteReference(prop_type_id, node.id(),
  old_target.id(),
              [](const scada::Status& status) {});
        }
        if (new_target) {
          node_management_service.AddReference(prop_type_id, node.id(),
  new_target.id(),
              [](const scada::Status& status) {});
        }
      });*/
}

PropertyEditor ReferencePropertyDefinition::GetPropertyEditor(
    PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_type_id) const {
  PropertyEditor result(PropertyEditor::DROPDOWN);

  if (auto& reference_type = type_definition.target(prop_type_id)) {
    result.choices.emplace_back(kChoiceNone);
    /*auto nodes = SyncFindNodesRecursive(context.node_service_, id::RootFolder,
    reference_type.id()); for (auto& node : nodes)
      result.choices.emplace_back(base::SysNativeMBToWide(node.browse_name()));*/
  }

  return result;
}

PropertyEditor BoolPropertyDefinition::GetPropertyEditor(
    PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_decl_id) const {
  PropertyEditor result(PropertyEditor::DROPDOWN);
  result.choices.emplace_back(scada::Variant::kFalseString);
  result.choices.emplace_back(scada::Variant::kTrueString);
  return result;
}

base::string16 EnumPropertyDefinition::GetText(
    PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) const {
  const auto& property_declaration = node.GetAggregateDeclaration(prop_decl_id);
  if (!property_declaration)
    return base::string16();

  const auto& value = node[prop_decl_id].value();
  const auto& enum_strings = SplitEnumStrings(
      property_declaration.data_type()[kEnumStrings].value().get_or(
          scada::String{}));

  int int_value;
  if (!value.get(int_value))
    return base::string16();

  if (int_value < 0 || int_value >= static_cast<int>(enum_strings.size()))
    return base::string16();

  return base::SysNativeMBToWide(enum_strings[static_cast<size_t>(int_value)]);
}

void EnumPropertyDefinition::SetText(PropertyContext& context,
                                     const NodeRef& node,
                                     const scada::NodeId& prop_decl_id,
                                     const base::string16& text) const {
  const auto& property_declaration = node.GetAggregateDeclaration(prop_decl_id);
  if (!property_declaration)
    return;

  const auto& enum_strings = SplitEnumStrings(
      property_declaration.data_type()[kEnumStrings].value().get_or(
          scada::String{}));
  int value;
  if (!ParseEnumStrings(enum_strings, value))
    return;

  context.task_manager_.PostUpdateTask(node.id(), {}, {{prop_decl_id, value}});
}

PropertyEditor EnumPropertyDefinition::GetPropertyEditor(
    PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_decl_id) const {
  const auto& property_declaration =
      type_definition.GetAggregateDeclaration(prop_decl_id);
  if (!property_declaration)
    return PropertyEditor(PropertyEditor::NONE);

  PropertyEditor result(PropertyEditor::DROPDOWN);
  const auto& enum_strings = SplitEnumStrings(
      property_declaration.data_type()[kEnumStrings].value().as_string());
  std::transform(
      enum_strings.begin(), enum_strings.end(),
      std::back_inserter(result.choices),
      [](base::StringPiece choice) { return base::SysNativeMBToWide(choice); });
  return result;
}

base::string16 ChannelPropertyDefinition::GetTitle(
    PropertyContext& context,
    const NodeRef& property_declaration) const {
  return title_;
}

base::string16 ChannelPropertyDefinition::GetText(
    PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_type_id) const {
  if (!IsInstanceOf(node, id::DataItemType))
    return base::string16();

  auto channel_path = node[prop_type_id].value().get_or(std::string{});

  scada::NodeId parent_id;
  scada::NodeId node_id;
  base::StringPiece component_name;
  if (IsNodeIdFormula(channel_path, node_id) &&
      IsNestedNodeId(node_id, parent_id, component_name)) {
    if (!device_)
      return base::SysNativeMBToWide(component_name);
    /*auto&& parent = SyncRequestNode(context.node_service_, node_id);
    return base::SysNativeMBToWide(parent.browse_name());*/
    return {};
  } else {
    if (device_)
      return base::string16();
    else
      return base::SysNativeMBToWide(channel_path);
  }
}

void ChannelPropertyDefinition::SetText(PropertyContext& context,
                                        const NodeRef& node,
                                        const scada::NodeId& prop_type_id,
                                        const base::string16& text) const {
  if (!IsInstanceOf(node, id::DataItemType))
    return;

  scada::NodeId new_device_id;
  if (device_) {
    // new_device_id = SyncFindNodeRecursive(context.node_service_, id::Devices,
    // base::SysWideToNativeMB(text), id::DeviceType).id();
  }

  auto channel_path = node[prop_type_id].value().get_or(std::string{});
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

  auto component_id =
      item_path.empty()
          ? std::string()
          : MakeNodeIdFormula(MakeNestedNodeId(parent_id, item_path));

  context.task_manager_.PostUpdateTask(
      node.id(), {}, {{prop_type_id, std::move(component_id)}});
}

PropertyEditor ChannelPropertyDefinition::GetPropertyEditor(
    PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_type_id) const {
  if (device_) {
    PropertyEditor result(PropertyEditor::DROPDOWN);
    result.choices.emplace_back(kChoiceNone);
    /*auto&& nodes = SyncFindNodesRecursive(context.node_service_, id::Devices,
    id::DeviceType); for (auto& node : nodes)
      result.choices.emplace_back(base::SysNativeMBToWide(node.browse_name()));*/
    return result;
  }

  return PropertyEditor(PropertyEditor::SIMPLE);
}

PropertyEditor TransportPropertyDefinition::GetPropertyEditor(
    PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_type_id) const {
  return PropertyEditor(PropertyEditor::BUTTON);
}

base::string16 ColorPropertyDefinition::GetText(
    PropertyContext& context,
    const NodeRef& node,
    const scada::NodeId& prop_type_id) const {
  auto value = node[prop_type_id].value();
  auto color_index = value.get_or(-1);
  if (color_index >= 0 && color_index < palette::GetColorCount())
    return palette::GetColorName(color_index);
  else
    return kDefaultColorString;
}

void ColorPropertyDefinition::SetText(PropertyContext& context,
                                      const NodeRef& node,
                                      const scada::NodeId& prop_type_id,
                                      const base::string16& text) const {
  if (text.empty())
    return;

  int color = palette::FindColorName(text.c_str());
  context.task_manager_.PostUpdateTask(node.id(), {}, {{prop_type_id, color}});
}

PropertyEditor ColorPropertyDefinition::GetPropertyEditor(
    PropertyContext& context,
    const NodeRef& type_definition,
    const scada::NodeId& prop_type_id) const {
  PropertyEditor result(PropertyEditor::DROPDOWN);
  result.choices.reserve(1 + palette::GetColorCount());
  result.choices.emplace_back(kDefaultColorString);
  for (size_t i = 0; i < palette::GetColorCount(); i++)
    result.choices.emplace_back(palette::GetColorName(i));
  return result;
}
