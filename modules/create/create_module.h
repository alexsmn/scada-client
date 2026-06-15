#pragma once

#include "scada/node_id.h"

class NodeService;
class UiCommandRegistry;

struct CreateModuleContext {
  NodeService& node_service_;
  UiCommandRegistry& ui_command_registry_;
};

class CreateModule : private CreateModuleContext {
 public:
  explicit CreateModule(CreateModuleContext&& context);
};

// Returns the node type created by a dynamic `ID_NEW + n` command.
scada::NodeId GetCreateCommandTypeId(unsigned command_id);
