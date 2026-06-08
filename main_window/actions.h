#pragma once

#include "scada/node_id.h"

class NodeService;
class UiCommandRegistry;

void AddGlobalActions(UiCommandRegistry& ui_command_registry,
                      NodeService& node_service);
void AddDefaultMenuContributions(UiCommandRegistry& ui_command_registry);

scada::NodeId GetNewCommandTypeId(unsigned command_id);
