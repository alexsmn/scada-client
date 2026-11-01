#pragma once

#include "base/awaitable.h"
#include "scada/authorization.h"
#include "scada/status.h"

#include <string>

class LocalEvents;
class NodeService;
class Profile;

// Enables (`disabled` false) or disables the account `account` by the
// standard ModifyUser method on the UserManagement object, changing only the
// Disabled bit of `current` (Part 18 §5.2.7,
// https://reference.opcfoundation.org/Core/Part18/v105/docs/5.2.7). A
// disabled account may not sign in; its open sessions are not closed.
Awaitable<scada::Status> SetAccountDisabled(NodeService& node_service,
                                            std::u16string account,
                                            scada::UserConfiguration current,
                                            bool disabled);

// Reports the outcome of a change to an account in the event journal. A
// refusal with Bad_NotWritable is the server protecting an account it
// manages itself — the inter-process service account — and is said so,
// because the bare status name would not tell the operator where the account
// is changed instead.
void ReportAccountChangeResult(const std::u16string& title,
                               const scada::Status& status,
                               LocalEvents& local_events,
                               const Profile& profile);
