#pragma once

#include "scada/node_id.h"

class NodeService;
class OpenedViewCommandRegistry;
class UiCommandRegistry;

struct CreateModuleContext {
  NodeService& node_service_;
  UiCommandRegistry& ui_command_registry_;
  OpenedViewCommandRegistry& opened_view_commands_;
};

class CreateModule : private CreateModuleContext {
 public:
  explicit CreateModule(CreateModuleContext&& context);
};

// Returns the node type created by a dynamic `ID_NEW + n` command.
scada::NodeId GetCreateCommandTypeId(unsigned command_id);
