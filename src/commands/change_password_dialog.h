#pragma once

#include "core/configuration_types.h"

namespace scada {
class NodeManagementService;
}

class LocalEvents;
class NodeRef;
class Profile;

void ShowChangePasswordDialog(const NodeRef& user, LocalEvents& local_events, Profile& profile,
    scada::NodeManagementService& node_management_service);
