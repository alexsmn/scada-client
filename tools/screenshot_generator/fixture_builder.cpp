#include "fixture_builder.h"
#include "base/utf_convert.h"
#include "scada/access_rights.h"
#include "scada/user_management_encoding.h"
#include "scada/authorization.h"
#include "model/security_node_ids.h"
#include "base/time/calendar.h"

#include "graph_capture.h"
#include "screenshot_config.h"

#include "address_space/address_space_impl.h"
#include "address_space/address_space_util.h"
#include "address_space/generic_node_factory.h"
#include "address_space/type_definition.h"
#include "base/check.h"
#include "common/node_state.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "scada/standard_node_ids.h"

#include <boost/json.hpp>

#include <span>
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
    return scada::data_items::id::AnalogItemType_DisplayFormat;
  // Analog-limit bands, so a fixture node can drive the trend's limit markers.
  if (name == "limit_lolo")
    return scada::data_items::id::AnalogItemType_LimitLoLo;
  if (name == "limit_lo")
    return scada::data_items::id::AnalogItemType_LimitLo;
  if (name == "limit_hi")
    return scada::data_items::id::AnalogItemType_LimitHi;
  if (name == "limit_hihi")
    return scada::data_items::id::AnalogItemType_LimitHiHi;
  if (LooksLikeJsonNodeId(name))
    return NodeIdFromScadaString(name);
  return std::nullopt;
}

scada::NodeId ParseJsonTypeDefinition(const boost::json::object& node) {
  if (const auto* type = node.if_contains("type_definition")) {
    auto type_name = std::string_view(type->as_string());
    if (type_name == "analog_item")
      return scada::data_items::id::AnalogItemType;
    if (type_name == "discrete_item")
      return scada::data_items::id::DiscreteItemType;
    if (LooksLikeJsonNodeId(type_name))
      return NodeIdFromScadaString(type_name);
  }

  const bool is_variable = node.at("class").as_string() == "variable";
  return is_variable ? scada::NodeId{scada::id::BaseVariableType, 0}
                     : scada::NodeId{scada::id::FolderType, 0};
}

std::optional<scada::Variant> ParseJsonVariant(
    const boost::json::value& value) {
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

scada::Time FixtureNow(const boost::json::value& json) {
  const auto* jnow = json.as_object().if_contains("now");
  if (!jnow)
    return scada::kNullTime;
  return scada::base::TimeFromString(std::string(jnow->as_string()),
                                     /*is_local=*/true)
      .value_or(scada::kNullTime);
}

Page MakeScreenshotPage(const std::vector<ScreenshotSpec>& specs,
                        const boost::json::value& json) {
  Page page;
  for (const auto& spec : specs) {
    // A `capture` spec is rendered standalone by its own routine, so it never
    // belongs on the page. Most name chrome with no registered window type at
    // all: adding a WindowDefinition for one only made ViewManager::OpenView
    // log "Window type not found" and return nullptr. The few that do name a
    // real type still build their own fixture (an authenticated identity, or
    // NodeRefs that only resolve after the page is assembled), so the page view
    // would sit there unused.
    if (!spec.capture.empty())
      continue;
    if (spec.window_type == "Graph")
      page.AddWindow(MakeGraphDefinition(json));
    else {
      WindowDefinition window{spec.window_type};
      auto add_item = [&spec, &window](const std::string& item_path) {
        WindowItem& item = window.AddItem("Item");
        item.SetString("path", item_path);
        if (spec.column_width > 0)
          item.SetInt("width", spec.column_width);
      };
      if (!spec.path.empty())
        add_item(spec.path);
      for (const auto& item_path : spec.paths)
        add_item(item_path);

      // A spreadsheet view keeps its content in the saved window, one item per
      // cell — there is no node path to seed it from.
      for (const auto& cell : spec.cells) {
        WindowItem& item = window.AddItem("SheetCell");
        item.SetInt("row", cell.row);
        item.SetInt("col", cell.column);
        item.SetString("text", cell.text);
        if (!cell.align.empty())
          item.SetString("align", cell.align);
        if (!cell.color.empty())
          item.SetString("color", cell.color);
      }
      for (size_t i = 0; i < spec.column_widths.size(); ++i) {
        WindowItem& item = window.AddItem("Column");
        item.SetInt("ix", static_cast<int>(i) + 1);
        item.SetInt("width", spec.column_widths[i]);
      }

      page.AddWindow(std::move(window));
    }
  }
  return page;
}

void PopulateFixtureNodes(AddressSpaceImpl& address_space,
                          const boost::json::value& root) {
  // Build child→parents map from the JSON tree so each instance can find the
  // existing node it should attach to.
  //
  // Every declared parent is kept, and a child with several is attached to all
  // of them. This was a `map<NodeId, NodeId>` with `parent_map[child] = parent`
  // until 2026-09-19, so a second declaration silently overwrote the first and
  // the address space received one edge of the two — `tree` declares 153 edges
  // over 141 distinct children, and 12 children carry two parents, so 12 edges
  // were being dropped on every run. The loss was invisible because the
  // survivor is whichever parent appears LAST in the file (boost::json::object
  // preserves insertion order), and in both affected captures that parent was
  // somewhere the image could not show: TC1/TC2/TC8 kept `TS.109` КРУ, which
  // renders collapsed, so they vanished from devices.png's top level; ЭНИП-2
  // and ЭНМВ-1 kept `TS.112`, which belongs to the Objects tree and appears
  // nowhere in the hardware tree, so they vanished from hardware-tree.png
  // entirely. Five rows missing across two tracked captures, and seven more
  // nodes affected where nothing rendered them either way (backlog 783).
  //
  // A node reachable through two hierarchical references is ordinary OPC UA,
  // and the fixture means it: those 40 extra edges are deliberate.
  std::unordered_map<scada::NodeId, std::vector<scada::NodeId>> parent_map;
  for (const auto& [parent_str, children] : root.at("tree").as_object()) {
    auto parent = NodeIdFromScadaString(std::string_view(parent_str));
    for (const auto& child : children.as_array()) {
      auto child_id = ParseJsonChildNodeId(child);
      parent_map[child_id].push_back(parent);
    }
  }

  // Every `nodes` entry must be placed somewhere in `tree`: a node without a
  // parent cannot be created, so it exists nowhere in the address space. That
  // used to be a silent drop, which let four fully-specified fixture nodes —
  // and an event referencing one of them — sit unnoticed. Report all of them
  // at once so a fixture edit is fixed in one pass.
  std::string orphans;
  for (const auto& jn : root.at("nodes").as_array()) {
    auto node_id = ParseJsonChildNodeId(jn.as_object().at("id"));
    if (parent_map.contains(node_id) || address_space.GetNode(node_id))
      continue;
    if (!orphans.empty())
      orphans += ", ";
    orphans += NodeIdToScadaString(node_id);
  }
  scada::base::Check(orphans.empty(),
                     "fixture nodes missing a `tree` parent (add them under a "
                     "parent or delete them from `nodes`): " +
                         orphans);

  GenericNodeFactory factory{address_space};
  std::vector<PendingReference> pending_references;

  // Multi-pass: a child can only be created after its parent already
  // exists in the address space. Standard OPC UA / SCADA nodes
  // (ns=0/7) are pre-built by ScadaTestAddressSpace, but fixture instances
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

      // Guaranteed present by the orphan check above.
      auto parent_it = parent_map.find(node_id);
      scada::base::Check(parent_it != parent_map.end(),
                         "fixture node lost its `tree` parent: " +
                             NodeIdToScadaString(node_id));
      // The node is CREATED under its first declared parent; every other
      // declared parent becomes an ordinary reference once all the nodes
      // exist, below. Defer until the creation parent is resident — the
      // others need not be yet, since a pending reference is applied after
      // the whole multi-pass loop has finished.
      const std::vector<scada::NodeId>& parent_ids = parent_it->second;
      scada::base::Check(!parent_ids.empty(),
                         "fixture node has an empty parent list: " +
                             NodeIdToScadaString(node_id));
      const scada::NodeId& parent_id = parent_ids.front();
      if (!address_space.GetNode(parent_id)) {
        next.push_back(jn_ptr);
        continue;
      }
      for (const scada::NodeId& extra_parent :
           std::span{parent_ids}.subspan(1)) {
        pending_references.push_back(PendingReference{
            .source_id = extra_parent,
            .reference_type_id = scada::NodeId{scada::id::Organizes, 0},
            .target_id = node_id,
        });
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
      state.node_class =
          is_variable ? scada::NodeClass::Variable : scada::NodeClass::Object;
      state.type_definition_id = ParseJsonTypeDefinition(jn.as_object());
      state.parent_id = parent_id;
      state.reference_type_id = scada::NodeId{scada::id::Organizes, 0};
      state.attributes.browse_name = scada::QualifiedName{browse_name};
      state.attributes.display_name = display_name;

      if (is_variable) {
        // Typed, not coerced to a double. A diagnostic variable's *shape* is
        // part of what reads it: the device-diagnostics panel asks a link state
        // for an Int32 and a t1 flag for a bool (FormatShaped), and a double
        // answers neither — the row renders blank rather than wrong, which is
        // the harder kind of empty to notice. JSON already distinguishes 4 from
        // 4.0, so the fixture can say which it means.
        if (const auto* bv = jn.as_object().if_contains("base_value")) {
          if (auto value = ParseJsonVariant(*bv))
            state.attributes.value = std::move(*value);
        } else if (const auto* bt = jn.as_object().if_contains("base_time")) {
          // DateTime has no JSON literal of its own, so a time-valued variable
          // spells its value the way the fixture's `now` does.
          if (auto stamp = scada::base::TimeFromString(
                  std::string(bt->as_string()), /*is_local=*/true)) {
            state.attributes.value = scada::Variant{*stamp};
          }
        }
      }

      if (const auto* properties = jn.as_object().if_contains("properties")) {
        for (const auto& [name, value] : properties->as_object()) {
          // Engineering units must be stored as LocalizedText: the format
          // path (GetTitFormatParams) reads the property with
          // get_or(LocalizedText{}), so a plain String value would render
          // no units at all.
          if (name == "units" && value.is_string()) {
            state.properties.emplace_back(
                scada::data_items::id::AnalogItemType_EngineeringUnits,
                scada::ToLocalizedText(std::string(value.as_string())));
            continue;
          }
          auto property_id = ParseJsonPropertyId(name);
          if (!property_id)
            continue;
          // TsFormat state labels are declared LocalizedText; a plain String
          // value would not read back through the grid's
          // get_or(LocalizedText{}), leaving the column blank.
          if (value.is_string() &&
              (*property_id == scada::data_items::id::TsFormatType_OpenLabel ||
               *property_id ==
                   scada::data_items::id::TsFormatType_CloseLabel)) {
            state.properties.emplace_back(
                *property_id,
                scada::ToLocalizedText(std::string(value.as_string())));
            continue;
          }
          // The transmission source link is a NodeId-valued property
          // (SourceNode — the OPC UA alignment replaced the retired
          // HasTransmissionSource reference). A plain String value would not
          // read back through the model's get_or(scada::NodeId{}).
          if (value.is_string() &&
              *property_id ==
                  scada::devices::id::TransmissionItemType_SourceNode) {
            state.properties.emplace_back(
                *property_id,
                NodeIdFromScadaString(std::string_view(value.as_string())));
            continue;
          }
          if (auto prop_value = ParseJsonVariant(value))
            state.properties.emplace_back(*property_id, std::move(*prop_value));
        }
      }

      if (const auto* references = jn.as_object().if_contains("references")) {
        for (const auto& ref : references->as_array()) {
          const auto& ref_obj = ref.as_object();
          pending_references.push_back(PendingReference{
              .source_id = node_id,
              .reference_type_id = NodeIdFromScadaString(
                  std::string_view(ref_obj.at("type").as_string())),
              .target_id = ParseJsonChildNodeId(ref_obj.at("target")),
          });
        }
      }

      // A fixture node that fails to build (usually a type definition
      // missing from ScadaTestAddressSpace) must abort the run: a silent
      // drop cascades into empty screenshots that still "pass".
      const auto [status, node] = factory.CreateNode(state);
      scada::base::Check(status, "fixture node creation failed: " +
                                     NodeIdToScadaString(node_id) + " | " +
                                     ToString(status));
      progressed = true;
    }
    pending = std::move(next);
  }

  scada::base::Check(pending.empty(),
                     "fixture nodes left unresolved (parent missing from the "
                     "address space)");

  for (const auto& ref : pending_references) {
    scada::base::Check(address_space.GetNode(ref.source_id) &&
                           address_space.GetNode(ref.target_id),
                       "fixture reference endpoints missing: " +
                           NodeIdToScadaString(ref.source_id) + " -> " +
                           NodeIdToScadaString(ref.target_id));
    scada::AddReference(address_space, ref.reference_type_id, ref.source_id,
                        ref.target_id);
  }
}
