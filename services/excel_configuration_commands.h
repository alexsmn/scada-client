#pragma once

class NodeService;
class DialogService;
class TaskManager;

void ExportConfigurationToExcel(NodeService& node_service,
                                DialogService& dialog_service);
void ImportConfigurationFromExcel(NodeService& node_service,
                                  TaskManager& task_manager,
                                  DialogService& dialog_service);
