#pragma once

#include "node_service/node_ref.h"
#include "scada/authorization.h"

#include <optional>

// The account's UserConfigurationMask (OPC UA Part 18 §5.2.3,
// https://reference.opcfoundation.org/Core/Part18/v105/docs/5.2.3) as the node
// service holds it now, or nullopt when its property is not resident. Never
// waits: it answers the synchronous question a command's availability asks.
std::optional<scada::UserConfiguration> ResidentUserConfiguration(
    const NodeRef& user);

// True when `user` is an account the server will not delete — its mask
// carries NoDelete, which the server sets on the inter-process service
// account. False when the mask is not resident: the server still refuses.
bool IsUndeletableAccount(const NodeRef& user);
