#pragma once

#include "common/aliases.h"

#include <memory>

class Logger;
class NodeService;

AliasResolver CreateAliasResolver(NodeService& node_service,
                                  const std::shared_ptr<const Logger>& logger);