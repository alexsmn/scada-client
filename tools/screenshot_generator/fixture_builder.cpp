#include "fixture_builder.h"

#include "graph_capture.h"
#include "screenshot_config.h"

#include "address_space/address_space_impl.h"
#include "address_space/generic_node_factory.h"
#include "common/node_state.h"
#include "model/data_items_node_ids.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "scada/standard_node_ids.h"

#include <boost/json.hpp>

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

// Parses "ns.id" (e.g. "0.84") or a bare "id" (implies ns=1) into a
// scada::NodeId. Mirrors the format used in `screenshot_data.json`.
scada::NodeId ParseJsonNodeId(std::string_view s) {
  scada::NumericId id = 0;
  scada::NamespaceIndex ns = 1;
  if (auto dot = s.find('.'); dot != std::string_view::npos) {
    ns = static_cast<scada::NamespaceIndex>(
        std::stoi(std::string(s.substr(0, dot))));
    id = static_cast<scada::NumericId>(
        std::stoi(std::string(s.substr(dot + 1))));
  } else {
    id = static_cast<scada::NumericId>(std::stoi(std::string(s)));
  }
  return scada::NodeId{id, ns};
}

scada::NodeId ParseJsonChildNodeId(const boost::json::value& child) {
  if (child.is_string())
    return ParseJsonNodeId(std::string_view(child.as_string()));
  return scada::NodeId{static_cast<scada::NumericId>(child.as_int64()), 1};
}

std::optional<scada::NodeId> ParseJsonPropertyId(std::string_view name) {
  if (name == "display_format")
    return data_items::id::AnalogItemType_DisplayFormat;
  return std::nullopt;
}

scada::NodeId ParseJsonTypeDefinition(const boost::json::object& node) {
  if (const auto* type = node.if_contains("type_definition")) {
    auto type_name = std::string_view(type->as_string());
    if (type_name == "analog_item")
      return data_items::id::AnalogItemType;
    if (type_name == "discrete_item")
      return data_items::id::DiscreteItemType;
  }

  const bool is_variable = node.at("class").as_string() == "variable";
  return is_variable ? scada::NodeId{scada::id::BaseVariableType, 0}
                     : scada::NodeId{scada::id::FolderType, 0};
}

}  // namespace

Page MakeScreenshotPage(const std::vector<ScreenshotSpec>& specs,
                        const boost::json::value& json) {
  Page page;
  for (const auto& spec : specs) {
    if (spec.window_type == "Graph")
      page.AddWindow(MakeGraphDefinition(json));
    else
      page.AddWindow(WindowDefinition{spec.window_type});
  }
  return page;
}

void PopulateFixtureNodes(AddressSpaceImpl& address_space,
                          const boost::json::value& root) {
  // Build child→parent map from the JSON tree so each instance can find
  // the existing node it should attach to.
  std::unordered_map<scada::NodeId, scada::NodeId> parent_map;
  for (const auto& [parent_str, children] : root.at("tree").as_object()) {
    auto parent = ParseJsonNodeId(std::string_view(parent_str));
    for (const auto& child : children.as_array()) {
      auto child_id = ParseJsonChildNodeId(child);
      parent_map[child_id] = parent;
    }
  }

  GenericNodeFactory factory{address_space};

  // Multi-pass: a child can only be created after its parent already
  // exists in the address space. Standard SCADA folders (ns=0/7) are
  // pre-built by AddressSpaceImpl3, but ns=1 parents aren't, so we may
  // need to defer until a previous pass placed them. Bounded by the
  // number of ns=1 entries to avoid runaway loops on broken fixtures.
  std::vector<const boost::json::value*> pending;
  for (const auto& jn : root.at("nodes").as_array()) {
    auto ns = static_cast<scada::NamespaceIndex>(jn.at("ns").as_int64());
    // Skip ns=0 (standard OPC UA) and ns=7 (standard SCADA) — already
    // populated by AddressSpaceImpl3.
    if (ns != 1)
      continue;
    pending.push_back(&jn);
  }

  bool progressed = true;
  while (progressed && !pending.empty()) {
    progressed = false;
    std::vector<const boost::json::value*> next;
    for (const auto* jn_ptr : pending) {
      const auto& jn = *jn_ptr;
      auto id = static_cast<scada::NumericId>(jn.at("id").as_int64());
      scada::NodeId node_id{id, 1};

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
      auto display_name =
          scada::ToLocalizedText(std::string_view(jn.at("name").as_string()));

      scada::NodeState state;
      state.node_id = node_id;
      state.node_class = is_variable ? scada::NodeClass::Variable
                                     : scada::NodeClass::Object;
      state.type_definition_id = ParseJsonTypeDefinition(jn.as_object());
      state.parent_id = parent_id;
      state.reference_type_id = scada::NodeId{scada::id::Organizes, 0};
      state.attributes.browse_name =
          scada::QualifiedName{std::string(jn.at("name").as_string())};
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
          if (value.is_string())
            state.properties.emplace_back(*property_id,
                                          std::string(value.as_string()));
        }
      }

      factory.CreateNode(state);
      progressed = true;
    }
    pending = std::move(next);
  }
}
