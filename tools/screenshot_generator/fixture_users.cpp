// Projects the fixture's UserType instances onto the OPC UA standard user
// model — the UserManagement.Users property and the RoleSet membership rules
// that carry each account's Roles.
//
// Separate from the rest of the fixture builder because it is a second pass
// over an address space the first one has already populated, and because it is
// the only part that encodes a standard-model representation rather than
// transcribing JSON.

#include "fixture_builder.h"

#include "address_space/address_space_impl.h"
#include "address_space/address_space_util.h"
#include "address_space/generic_node_factory.h"
#include "base/utf_convert.h"
#include "common/node_state.h"
#include "model/node_id_util.h"
#include "model/security_node_ids.h"
#include "scada/access_rights.h"
#include "scada/authorization.h"
#include "scada/standard_node_ids.h"
#include "scada/user_management_encoding.h"

#include <boost/json.hpp>

#include <string>
#include <vector>

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
