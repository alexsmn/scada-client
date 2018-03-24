#pragma once

namespace scada {
class NodeManagementService;
}  // namespace scada

class LocalEvents;
class NodeRef;
class Profile;

void ShowChangePasswordDialog(
    const NodeRef& user,
    scada::NodeManagementService& node_management_service,
    LocalEvents& local_events,
    Profile& profile);
