#pragma once

struct ScreenshotSpec;
class NodeService;

// Renders the reshelled users-admin RBAC inspector — the right region of
// users-admin.html — from a fixture user, and saves it under
// `GetOutputDir() / spec.filename`.
//
// Standalone like SaveDeviceDiagnosticsScreenshot: it makes the fixture user
// (and its type) resident, builds a fresh UserAccessPanel, drives it via
// ShowUser with the real node service, then grabs the widget — exercising the
// real AccessRights read + role/permission derivation.
void SaveUserAccessScreenshot(const ScreenshotSpec& spec,
                              NodeService& node_service);
