#pragma once

#include "base/any_executor.h"

namespace scada {
class AttributeService;
}

struct ScreenshotSpec;
class NodeService;

// Renders the reshelled users-admin RBAC inspector — the right region of
// users-admin.html — from a fixture user, and saves it under
// `GetOutputDir() / spec.filename`.
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
