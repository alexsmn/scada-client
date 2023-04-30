#pragma once

#include "base/promise.h"

class NodeService;
class DialogService;
class TaskManager;

promise<> ExportConfigurationToExcel(NodeService& node_service,
                                     DialogService& dialog_service);
promise<> ImportConfigurationFromExcel(NodeService& node_service,
                                       TaskManager& task_manager,
                                       DialogService& dialog_service);
