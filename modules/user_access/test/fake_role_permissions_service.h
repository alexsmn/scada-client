#pragma once

#include "base/awaitable.h"
#include "model/data_items_node_ids.h"
#include "scada/attribute_service.h"
#include "scada/authorization.h"
#include "scada/role_permission_encoding.h"
#include "scada/standard_node_ids.h"

#include <optional>
#include <utility>
#include <vector>

// An AttributeService that answers exactly two things: the RolePermissions
// attribute of the Server object — the server's published role -> permission
// map (OPC UA Part 3 §5.2.9), which is what the user-access surfaces read
// instead of carrying their own copy of it — and the same attribute of the
// data item Control object's Operate method, which carries the process map
// (scada::ProcessRolePermissions) that decides the Control capability.
//
// This is a purpose-built fake rather than the real `AttributeServiceImpl`
// because the behaviour under test is the CLIENT's, and the real service gates
// the attribute on a ServiceContext the client never fills in — in production
// the server supplies the caller's identity from the authenticated session, so
// wiring the real one here would only test a server-side gate the client does
// not drive.
//
// Constructed with `std::nullopt` it fails the read, which is how the "the map
// could not be read" path is exercised: the surfaces must report unknown, not
// fall back to an assumed map. Constructed with a map and no process map it
// stands for a server that predates the process map.
class FakeRolePermissionsService : public scada::AttributeService {
 public:
  // Serves the server's real default map — the common case.
  FakeRolePermissionsService()
      : entries_{scada::DefaultRolePermissions()},
        process_entries_{scada::ProcessRolePermissions()} {}

  explicit FakeRolePermissionsService(
      std::optional<std::vector<scada::RolePermissionType>> entries,
      std::optional<std::vector<scada::RolePermissionType>> process_entries =
          std::nullopt)
      : entries_{std::move(entries)},
        process_entries_{std::move(process_entries)} {}

  // scada::AttributeService
  scada::CoStatusOr<std::vector<scada::DataValue>> Read(
      scada::ServiceContext context,
      std::vector<scada::ReadValueId> inputs) override {
    std::vector<scada::DataValue> results;
    results.reserve(inputs.size());
    for (const scada::ReadValueId& input : inputs) {
      const bool is_role_permissions =
          input.node_id == scada::NodeId{scada::id::Server, 0} &&
          input.attribute_id == scada::AttributeId::RolePermissions;
      const bool is_process_role_permissions =
          input.node_id == scada::data_items::id::DataItemControlType_Operate &&
          input.attribute_id == scada::AttributeId::RolePermissions;
      if (is_role_permissions && entries_) {
        results.push_back(
            scada::MakeReadResult(scada::EncodeRolePermissions(*entries_)));
      } else if (is_process_role_permissions && process_entries_) {
        results.push_back(scada::MakeReadResult(
            scada::EncodeRolePermissions(*process_entries_)));
      } else {
        // The status an unauthorised caller gets for this attribute
        // (Part 3 §5.2.9 requires the ReadRolePermissions permission).
        results.push_back(
            scada::MakeReadError(scada::StatusCode::Bad_UserAccessDenied));
      }
    }
    co_return results;
  }

  scada::CoStatusOr<std::vector<scada::StatusCode>> Write(
      scada::ServiceContext context,
      std::vector<scada::WriteValue> inputs) override {
    co_return std::vector<scada::StatusCode>(
        inputs.size(), scada::StatusCode::Bad_NotWritable);
  }

 private:
  const std::optional<std::vector<scada::RolePermissionType>> entries_;
  const std::optional<std::vector<scada::RolePermissionType>> process_entries_;
};
