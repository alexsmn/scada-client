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
#include "base/check.h"
#include "common/node_state.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
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
      state.node_class =
          is_variable ? scada::NodeClass::Variable : scada::NodeClass::Object;
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

void ProjectFixtureUsersOntoStandardModel(AddressSpaceImpl& address_space,
                                          const boost::json::value& root) {
  namespace sec = scada::security::id;

  const scada::NodeId objects_folder{scada::id::ObjectsFolder,
                                     scada::NamespaceIndexes::NS0};
  const scada::NodeId organizes{scada::id::Organizes,
                                scada::NamespaceIndexes::NS0};
  const scada::NodeId role_set{scada::id::Server_ServerCapabilities_RoleSet,
                               scada::NamespaceIndexes::NS0};
  const scada::NodeId user_management{
      scada::id::Server_ServerConfiguration_UserManagement,
      scada::NamespaceIndexes::NS0};
  const scada::NodeId users_property{scada::id::UserManagement_Users,
                                     scada::NamespaceIndexes::NS0};

  GenericNodeFactory factory{address_space};
  std::vector<scada::UserManagementDataType> users;
  scada::NumericId next_rule_id = 1;

  // Parts of the NS0 tree (RoleSet among them) already exist in the standard
  // address space, so create only what is missing rather than failing on a
  // duplicate.
  const auto ensure_node = [&](scada::NodeState state) {
    const scada::NodeId node_id = state.node_id;
    if (address_space.GetNode(node_id)) {
      return;
    }
    // Check rather than ignore: a silently dropped node here leaves the grid
    // reporting "no data" for every account, which reads as a product bug.
    const auto result = factory.CreateNode(std::move(state));
    scada::base::Check(result.first, "fixture node not created: " +
                                         NodeIdToScadaString(node_id));
  };

  // The RoleSet and the two Roles that carry a coarse right. The rest are
  // implied at request time and would add nothing a fixture can show.
  ensure_node(
      scada::NodeState{.node_id = role_set,
                       .node_class = scada::NodeClass::Object,
                       .type_definition_id = {scada::id::FolderType,
                                              scada::NamespaceIndexes::NS0},
                       .parent_id = objects_folder,
                       .reference_type_id = organizes,
                       .attributes = scada::NodeAttributes{
                           .browse_name = "RoleSet",
                           .display_name = u"RoleSet"}});
  ensure_node(scada::NodeState{
      .node_id = scada::WellKnownRoleId(scada::WellKnownRole::kOperator),
      .node_class = scada::NodeClass::Object,
      .type_definition_id = {scada::id::FolderType,
                             scada::NamespaceIndexes::NS0},
      .parent_id = role_set,
      .reference_type_id = organizes,
      .attributes = scada::NodeAttributes{.browse_name = "Operator",
                                          .display_name = u"Operator"}});
  ensure_node(scada::NodeState{
      .node_id = scada::WellKnownRoleId(scada::WellKnownRole::kConfigureAdmin),
      .node_class = scada::NodeClass::Object,
      .type_definition_id = {scada::id::FolderType,
                             scada::NamespaceIndexes::NS0},
      .parent_id = role_set,
      .reference_type_id = organizes,
      .attributes =
          scada::NodeAttributes{.browse_name = "ConfigureAdmin",
                                .display_name = u"ConfigureAdmin"}});
  ensure_node(scada::NodeState{
      .node_id = sec::IdentityMappingRuleType,
      .node_class = scada::NodeClass::ObjectType,
      .parent_id = {scada::id::BaseObjectType, scada::NamespaceIndexes::NS0},
      .reference_type_id = {scada::id::HasSubtype,
                            scada::NamespaceIndexes::NS0},
      .attributes =
          scada::NodeAttributes{.browse_name = "IdentityMappingRuleType"},
      .supertype_id = {scada::id::BaseObjectType,
                       scada::NamespaceIndexes::NS0}});
  // The rule's property DECLARATIONS: without them SetPropertyValue has
  // nothing to resolve an instance property against.
  ensure_node(scada::NodeState{
      .node_id = sec::IdentityMappingRuleType_CriteriaType,
      .node_class = scada::NodeClass::Variable,
      .type_definition_id = {scada::id::PropertyType,
                             scada::NamespaceIndexes::NS0},
      .parent_id = sec::IdentityMappingRuleType,
      .reference_type_id = {scada::id::HasProperty,
                            scada::NamespaceIndexes::NS0},
      .attributes = scada::NodeAttributes{
          .browse_name = "CriteriaType",
          .data_type = {scada::id::Int32, scada::NamespaceIndexes::NS0}}});
  ensure_node(scada::NodeState{
      .node_id = sec::IdentityMappingRuleType_Criteria,
      .node_class = scada::NodeClass::Variable,
      .type_definition_id = {scada::id::PropertyType,
                             scada::NamespaceIndexes::NS0},
      .parent_id = sec::IdentityMappingRuleType,
      .reference_type_id = {scada::id::HasProperty,
                            scada::NamespaceIndexes::NS0},
      .attributes = scada::NodeAttributes{
          .browse_name = "Criteria",
          .data_type = {scada::id::String, scada::NamespaceIndexes::NS0}}});

  // Read the accounts straight from the fixture JSON rather than from the
  // address space: it is the same source PopulateFixtureNodes used, and it
  // keeps this projection independent of the node API.
  for (const auto& entry : root.at("nodes").as_array()) {
    const auto& node = entry.as_object();
    const auto* type = node.if_contains("type_definition");
    if (!type || !type->is_string() ||
        NodeIdFromScadaString(std::string_view{type->as_string()}) !=
            sec::UserType) {
      continue;
    }

    const auto* display = node.if_contains("display_name");
    if (!display || !display->is_string()) {
      continue;
    }
    const std::string name{display->as_string()};
    if (name.empty()) {
      continue;
    }
    users.push_back(scada::UserManagementDataType{.user_name = name});

    // The account's legacy bits, projected onto Role membership exactly as
    // the server's one-shot MigrateAccessRightsToRoles does.
    std::int64_t access_rights = 0;
    if (const auto* properties = node.if_contains("properties");
        properties && properties->is_object()) {
      if (const auto* rights = properties->as_object().if_contains(
              NodeIdToScadaString(sec::UserType_AccessRights));
          rights && rights->is_number()) {
        access_rights = rights->to_number<std::int64_t>();
      }
    }

    const auto add_membership = [&](scada::WellKnownRole role) {
      const auto rule_result = factory.CreateNode(scada::NodeState{
          .node_id = scada::NodeId{next_rule_id++,
                                   scada::NamespaceIndexes::ROLE_IDENTITY},
          .node_class = scada::NodeClass::Object,
          .type_definition_id = sec::IdentityMappingRuleType,
          .parent_id = scada::WellKnownRoleId(role),
          .reference_type_id = {scada::id::Organizes,
                                scada::NamespaceIndexes::NS0},
          .attributes = scada::NodeAttributes{.browse_name = name},
          .properties = {
              {sec::IdentityMappingRuleType_CriteriaType,
               static_cast<scada::Int32>(
                   scada::IdentityCriteriaType::kUserName)},
              {sec::IdentityMappingRuleType_Criteria, name}}});
      scada::base::Check(rule_result.first,
                         "fixture identity rule not created for " + name);
    };
    if (access_rights & scada::AccessRightBit(scada::AccessRight::kControl)) {
      add_membership(scada::WellKnownRole::kOperator);
    }
    if (access_rights & scada::AccessRightBit(scada::AccessRight::kConfigure)) {
      add_membership(scada::WellKnownRole::kConfigureAdmin);
    }
  }

  // The UserManagement object and its Users property, created last so the
  // account list is written in as the property's static value.
  ensure_node(scada::NodeState{
      .node_id = user_management,
      .node_class = scada::NodeClass::Object,
      .type_definition_id = {scada::id::BaseObjectType,
                             scada::NamespaceIndexes::NS0},
      .parent_id = objects_folder,
      .reference_type_id = organizes,
      .attributes = scada::NodeAttributes{.browse_name = "UserManagement",
                                          .display_name = u"UserManagement"}});
  ensure_node(scada::NodeState{
      .node_id = users_property,
      .node_class = scada::NodeClass::Variable,
      .type_definition_id = {scada::id::PropertyType,
                             scada::NamespaceIndexes::NS0},
      .parent_id = user_management,
      .reference_type_id = {scada::id::HasProperty,
                            scada::NamespaceIndexes::NS0},
      .attributes = scada::NodeAttributes{
          .browse_name = "Users",
          .display_name = u"Users",
          .value = scada::EncodeUserManagementUsers(users)}});
}
