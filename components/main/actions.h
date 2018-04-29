#pragma once

#include "core/node_id.h"

class ActionManager;
class NodeService;

void AddGlobalActions(ActionManager& action_manager, NodeService& node_service);

scada::NodeId GetNewCommandTypeId(unsigned command_id);
