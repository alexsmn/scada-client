#pragma once

#include "scada/access_rights.h"
#include "scada/attribute_service.h"
#include "scada/node_id.h"

#include <utility>
#include <vector>

// Presents the generator's in-process attribute service as an AUTHENTICATED
// ADMINISTRATOR session.
//
// Why this exists: a client never fills in the ServiceContext it passes. It
// sends an empty one and the SERVER substitutes the identity of the
// authenticated session (see the remote/gRPC path), so every client call site
// passes `scada::ServiceContext{}` and that is correct.
//
// The screenshot generator has no server. It wires the real
// `AttributeServiceImpl` directly into the client, so the client's empty
// context IS the effective one — and an empty context is ANONYMOUS. Any
// attribute the address space gates on a permission is then refused, which is
// not what the captured screen is supposed to be documenting: the RBAC
// inspector reads the server's role -> permission map off the RolePermissions
// attribute (OPC UA Part 3 §5.2.9), which requires ReadRolePermissions, and
// without it the panel correctly but uselessly renders "no data".
//
// So substitute what a real server would have supplied for the administrator
// the fixtures depict. This is a FIXTURE concern, not a client one — it must
// never be worked around by weakening the client's context handling or the
// address space's gate, both of which are right as they stand.
class AuthenticatedAttributeService : public scada::AttributeService {
 public:
  explicit AuthenticatedAttributeService(scada::AttributeService& inner)
      : inner_{inner} {}

  // scada::AttributeService
  scada::CoStatusOr<std::vector<scada::DataValue>> Read(
      scada::ServiceContext context,
      std::vector<scada::ReadValueId> inputs) override {
    return inner_.Read(WithAdminIdentity(std::move(context)), std::move(inputs));
  }

  scada::CoStatusOr<std::vector<scada::StatusCode>> Write(
      scada::ServiceContext context,
      std::vector<scada::WriteValue> inputs) override {
    return inner_.Write(WithAdminIdentity(std::move(context)),
                        std::move(inputs));
  }

 private:
  // A non-null user id (so the session does not read as anonymous) with both
  // access-right bits, which `RolesForUser` maps to every well-known Role bar
  // Anonymous — the administrator the fixture's screens are captured as.
  static scada::ServiceContext WithAdminIdentity(scada::ServiceContext context) {
    return context.with_user_id(scada::NodeId{1, 0})
        .with_user_rights(
            scada::AccessRightBit(scada::AccessRight::kConfigure) |
            scada::AccessRightBit(scada::AccessRight::kControl));
  }

  scada::AttributeService& inner_;
};
