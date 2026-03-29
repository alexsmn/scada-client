#include "services/alias_resolver_factory.h"

#include "base/program_options.h"
#include "base/nested_logger.h"
#include "services/alias_service.h"

AliasResolver CreateAliasResolver(NodeService& node_service,
                                  const std::shared_ptr<const Logger>& logger) {
  auto alias_logger =
      client::HasOption("log-alias-service")
          ? static_cast<std::shared_ptr<Logger>>(
                std::make_shared<NestedLogger>(logger, "AliasService"))
          : static_cast<std::shared_ptr<Logger>>(
                std::make_shared<NullLogger>());

  auto alias_service = std::make_shared<AliasService>(
      AliasServiceContext{alias_logger, node_service});

  return std::bind_front(&AliasService::Resolve, alias_service);
}