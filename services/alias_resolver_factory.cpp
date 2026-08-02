#include "services/alias_resolver_factory.h"

#include "base/boost_log.h"
#include "base/program_options.h"
#include "services/alias_service.h"

AliasResolver CreateAliasResolver(NodeService& node_service) {
  // Null logger suppresses AliasService diagnostics unless explicitly enabled.
  auto alias_logger =
      client::HasOption("log-alias-service")
          ? std::make_shared<BoostLogger>(LOG_NAME("AliasService"))
          : nullptr;

  auto alias_service = std::make_shared<AliasService>(
      AliasServiceContext{alias_logger, node_service});

  return std::bind_front(&AliasService::Resolve, alias_service);
}
