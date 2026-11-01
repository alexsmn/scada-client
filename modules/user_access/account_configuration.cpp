#include "user_access/account_configuration.h"

#include "model/security_node_ids.h"
#include "node_service/node_util.h"

std::optional<scada::UserConfiguration> ResidentUserConfiguration(
    const NodeRef& user) {
  if (!user || !IsInstanceOf(user, scada::security::id::UserType)) {
    return std::nullopt;
  }
  NodeRef property = user[scada::security::id::UserType_UserConfiguration];
  scada::Int32 mask = 0;
  if (!property || !property.value().get(mask)) {
    return std::nullopt;
  }
  return static_cast<scada::UserConfiguration>(mask);
}

bool IsUndeletableAccount(const NodeRef& user) {
  const auto mask = ResidentUserConfiguration(user);
  return mask && scada::HasUserConfiguration(
                     *mask, scada::UserConfiguration::kNoDelete);
}
