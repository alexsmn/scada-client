#pragma once

#include "scada/node_id.h"

class ActionManager;
class NodeService;
class UiCommandRegistry;

void AddGlobalActions(ActionManager& action_manager, NodeService& node_service);
void AddDefaultMenuContributions(UiCommandRegistry& ui_command_registry);

scada::NodeId GetNewCommandTypeId(unsigned command_id);
