#pragma once

#include "base/any_executor.h"

namespace scada {
class AttributeService;
}

struct ScreenshotSpec;
class NodeService;

// Renders the reshelled users-admin RBAC inspector — the right region of
// users-admin.html — from a fixture user, and saves it under
// `OutputPathFor(spec.filename)`.
//
// Standalone like SaveDeviceDiagnosticsScreenshot: it makes the fixture user
// (and its type) resident, builds a fresh UserAccessPanel, drives it via
// ShowUser with the real node and attribute services, then grabs the widget —
// exercising the real RoleSet browse and the real read of the server's
// published role → permission map.
void SaveUserAccessScreenshot(const ScreenshotSpec& spec,
                              NodeService& node_service,
                              scada::AttributeService& attribute_service,
                              AnyExecutor executor);

// Renders the Roles view — every Role of the RoleSet and the accounts it is
// granted to — and saves it under `OutputPathFor(spec.filename)`.
//
// Standalone for the same reason as SaveUserAccessScreenshot, and it is the
// reason rather than a convenience: reading the published role -> permission
// map needs the ReadRolePermissions permission (OPC UA Part 3 §5.2.9), and the
// generator's ordinary session is anonymous. Opened through the client's view
// path the panel therefore rendered "no data" — correctly, but the capture
// documented an empty grid. Driving it here with the administrator identity
// the fixture depicts is what a real server would have supplied.
void SaveRolesScreenshot(const ScreenshotSpec& spec,
                         NodeService& node_service,
                         scada::AttributeService& attribute_service,
                         AnyExecutor executor);

// Renders the users-admin grid — every account, its status, and the Roles it
// holds — and saves it under `OutputPathFor(spec.filename)`.
//
// Standalone for the same reason as SaveRolesScreenshot: the Roles column is
// joined from the same RoleSet read, which an anonymous session may not
// perform. Through the view path every account's Roles cell read "no data".
void SaveUsersGridScreenshot(const ScreenshotSpec& spec,
                             NodeService& node_service,
                             scada::AttributeService& attribute_service,
                             AnyExecutor executor);
