#pragma once

#include "common/aliases.h"

class NodeService;

// Builds an alias resolver backed by an AliasService. Diagnostic logging is
// emitted on the "AliasService" Boost.Log channel only when the
// `log-alias-service` command-line option is set; otherwise it is suppressed.
AliasResolver CreateAliasResolver(NodeService& node_service);
