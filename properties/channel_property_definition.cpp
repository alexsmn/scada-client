#include "properties/channel_property_definition.h"

#include "base/utf_convert.h"
#include "common/formula_util.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "properties/property_context.h"
#include "properties/property_util.h"
#include "services/task_manager.h"

namespace {

std::pair<scada::NodeId /*parent_id*/, std::string /*component_name*/>
ParseChannelPath(std::string_view channel_path) {
  scada::NodeId parent_id;
  std::string_view component_name;
  if (auto node_id = GetFormulaSingleNodeId(channel_path);
      node_id.is_null() ||
      !IsNestedNodeId(node_id, parent_id, component_name)) {
    parent_id = scada::NodeId();
    component_name = channel_path;
  }
  // Note: can't return string_view since |node_id| goes out of scope.
  return {parent_id, std::string{component_name}};
}

}  // namespace

const char16_t ChannelPropertyDefinition::kParentGroupDevice[] = u"<������>";

std::u16string ChannelPropertyDefinition::GetTitle(
    const PropertyContext& context,
    const NodeRef& property_declaration) const {
  return title_;
}

// static
scada::NodeId ChannelPropertyDefinition::GetDeviceId(
    const NodeRef& node,
    const scada::NodeId& prop_decl_id) {
  if (!IsInstanceOf(node, data_items::id::DataItemType))
    return {};

  auto channel_path = node[prop_decl_id].value().get_or(std::string{});

  std::string name = GetFormulaSingleName(channel_path);
  if (name.empty()) {
    return {};
  }

  auto node_id = NodeIdFromScadaString(name);
  if (node_id.is_null()) {
    return {};
  }

  scada::NodeId parent_id;
  std::string_view component_name;
  if (!IsNestedNodeId(node_id, parent_id, component_name)) {
    return {};
  }

  return parent_id;
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
                 : UtfConvert<char16_t>(component_name);
    } else if (auto path = GetParentGroupChannelPath(name); !path.empty()) {
      return device_ ? kParentGroupDevice : UtfConvert<char16_t>(path);
    }
  }

  return device_ ? std::u16string{} : UtfConvert<char16_t>(channel_path);
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
    item_path = UtfConvert<char>(text);

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
    for (const auto& component : GetDataVariables(parent)) {
      result.choices.emplace_back(
          scada::ToLocalizedText(component.browse_name().name()));
    }
  }

  return result;
}

void ChannelPropertyDefinition::GetAdditionalTargets(
    const NodeRef& node,
    const scada::NodeId& prop_decl_id,
    std::vector<scada::NodeId>& targets) const {
  if (auto device_id = GetDeviceId(node, prop_decl_id); !device_id.is_null()) {
    targets.emplace_back(std::move(device_id));
  }
}
