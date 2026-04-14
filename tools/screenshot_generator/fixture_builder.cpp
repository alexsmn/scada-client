#include "fixture_builder.h"

#include "graph_capture.h"
#include "screenshot_config.h"

#include "address_space/address_space_impl.h"
#include "address_space/address_space_util.h"
#include "address_space/generic_node_factory.h"
#include "common/node_state.h"
#include "model/data_items_node_ids.h"
#include "model/node_id_util.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "scada/standard_node_ids.h"

#include <boost/json.hpp>

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

struct PendingReference {
  scada::NodeId source_id;
  scada::NodeId reference_type_id;
  scada::NodeId target_id;
};

bool LooksLikeJsonNodeId(std::string_view s) {
  if (s.empty())
    return false;
  if (!NodeIdFromScadaString(s).is_null())
    return true;
  return std::ranges::all_of(s, [](char c) { return c >= '0' && c <= '9'; });
}

scada::NodeId ParseJsonChildNodeId(const boost::json::value& child) {
  if (child.is_string())
    return NodeIdFromScadaString(std::string_view(child.as_string()));
  return scada::NodeId{static_cast<scada::NumericId>(child.as_int64()), 1};
}

std::optional<scada::NodeId> ParseJsonPropertyId(std::string_view name) {
  if (name == "display_format")
    return data_items::id::AnalogItemType_DisplayFormat;
  if (LooksLikeJsonNodeId(name))
    return NodeIdFromScadaString(name);
  return std::nullopt;
}

scada::NodeId ParseJsonTypeDefinition(const boost::json::object& node) {
  if (const auto* type = node.if_contains("type_definition")) {
    auto type_name = std::string_view(type->as_string());
    if (type_name == "analog_item")
      return data_items::id::AnalogItemType;
    if (type_name == "discrete_item")
      return data_items::id::DiscreteItemType;
    if (LooksLikeJsonNodeId(type_name))
      return NodeIdFromScadaString(type_name);
  }

  const bool is_variable = node.at("class").as_string() == "variable";
  return is_variable ? scada::NodeId{scada::id::BaseVariableType, 0}
                     : scada::NodeId{scada::id::FolderType, 0};
}

std::optional<scada::Variant> ParseJsonVariant(const boost::json::value& value) {
  if (value.is_bool())
    return scada::Variant{value.as_bool()};
  if (value.is_int64())
    return scada::Variant{static_cast<scada::Int32>(value.as_int64())};
  if (value.is_uint64())
    return scada::Variant{static_cast<scada::UInt32>(value.as_uint64())};
  if (value.is_double())
    return scada::Variant{value.as_double()};
  if (value.is_string())
    return scada::Variant{std::string(value.as_string())};
  return std::nullopt;
}

std::string GetJsonString(const boost::json::object& node,
                          std::string_view primary_key,
                          std::string_view fallback_key) {
  if (const auto* value = node.if_contains(primary_key))
    return std::string(value->as_string());
  if (!fallback_key.empty()) {
    if (const auto* value = node.if_contains(fallback_key))
      return std::string(value->as_string());
  }
  return {};
}

}  // namespace

Page MakeScreenshotPage(const std::vector<ScreenshotSpec>& specs,
                        const boost::json::value& json) {
  Page page;
  for (const auto& spec : specs) {
    if (spec.window_type == "Graph")
      page.AddWindow(MakeGraphDefinition(json));
    else {
      WindowDefinition window{spec.window_type};
      if (!spec.path.empty())
        window.AddItem("Item").SetString("path", spec.path);
      page.AddWindow(std::move(window));
    }
  }
  return page;
}

void PopulateFixtureNodes(AddressSpaceImpl& address_space,
                          const boost::json::value& root) {
  // Build child→parent map from the JSON tree so each instance can find
  // the existing node it should attach to.
  std::unordered_map<scada::NodeId, scada::NodeId> parent_map;
  for (const auto& [parent_str, children] : root.at("tree").as_object()) {
    auto parent = NodeIdFromScadaString(std::string_view(parent_str));
    for (const auto& child : children.as_array()) {
      auto child_id = ParseJsonChildNodeId(child);
      parent_map[child_id] = parent;
    }
  }

  GenericNodeFactory factory{address_space};
  std::vector<PendingReference> pending_references;

  // Multi-pass: a child can only be created after its parent already
  // exists in the address space. Standard OPC UA / SCADA nodes
  // (ns=0/7) are pre-built by AddressSpaceImpl3, but fixture instances
  // in other namespaces are not, so we may need to defer until a
  // previous pass placed their parents. Bounded by the number of
  // fixture entries to avoid runaway loops on broken fixtures.
  std::vector<const boost::json::value*> pending;
  for (const auto& jn : root.at("nodes").as_array())
    pending.push_back(&jn);

  bool progressed = true;
  while (progressed && !pending.empty()) {
    progressed = false;
    std::vector<const boost::json::value*> next;
    for (const auto* jn_ptr : pending) {
      const auto& jn = *jn_ptr;
      auto node_id = ParseJsonChildNodeId(jn.as_object().at("id"));

      if (address_space.GetNode(node_id))
        continue;

      auto parent_it = parent_map.find(node_id);
      if (parent_it == parent_map.end()) {
        // Orphan instance — drop it; nothing in the running app will
        // navigate to it.
        continue;
      }
      const auto& parent_id = parent_it->second;
      if (!address_space.GetNode(parent_id)) {
        next.push_back(jn_ptr);
        continue;
      }

      const auto& cls = jn.at("class").as_string();
      const bool is_variable = (cls == "variable");
      auto browse_name = GetJsonString(jn.as_object(), "browse_name", {});
      if (browse_name.empty())
        browse_name = NodeIdToScadaString(node_id);
      auto display_name_string =
          GetJsonString(jn.as_object(), "display_name", {});
      if (display_name_string.empty())
        display_name_string = browse_name;
      auto display_name = scada::ToLocalizedText(display_name_string);

      scada::NodeState state;
      state.node_id = node_id;
      state.node_class = is_variable ? scada::NodeClass::Variable
                                     : scada::NodeClass::Object;
      state.type_definition_id = ParseJsonTypeDefinition(jn.as_object());
      state.parent_id = parent_id;
      state.reference_type_id = scada::NodeId{scada::id::Organizes, 0};
      state.attributes.browse_name = scada::QualifiedName{browse_name};
      state.attributes.display_name = display_name;

      if (is_variable) {
        if (const auto* bv = jn.as_object().if_contains("base_value"))
          state.attributes.value = scada::Variant{bv->to_number<double>()};
      }

      if (const auto* properties = jn.as_object().if_contains("properties")) {
        for (const auto& [name, value] : properties->as_object()) {
          auto property_id = ParseJsonPropertyId(name);
          if (!property_id)
            continue;
          if (auto prop_value = ParseJsonVariant(value))
            state.properties.emplace_back(*property_id, std::move(*prop_value));
        }
      }

      if (const auto* references = jn.as_object().if_contains("references")) {
        for (const auto& ref : references->as_array()) {
          const auto& ref_obj = ref.as_object();
          pending_references.push_back(PendingReference{
              .source_id = node_id,
              .reference_type_id =
                  NodeIdFromScadaString(std::string_view(ref_obj.at("type").as_string())),
              .target_id = ParseJsonChildNodeId(ref_obj.at("target")),
          });
        }
      }

      factory.CreateNode(state);
      progressed = true;
    }
    pending = std::move(next);
  }

  for (const auto& ref : pending_references) {
    if (!address_space.GetNode(ref.source_id) || !address_space.GetNode(ref.target_id))
      continue;
    scada::AddReference(address_space, ref.reference_type_id, ref.source_id,
                        ref.target_id);
  }
}
