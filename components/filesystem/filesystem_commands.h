#pragma once

#include <memory>

class DialogService;
class Executor;
class FileRegistry;
class MainWindow;
class NodeRef;
class TaskManager;

bool ExecuteFileCommand(MainWindow* main_window,
                        const std::shared_ptr<Executor>& executor,
                        const FileRegistry& file_registry,
                        const NodeRef& file_node,
                        unsigned shift);

void AddFile(NodeRef parent_directory,
             DialogService& dialog_service,
             TaskManager& task_manager);

void CreateFileDirectory(NodeRef parent_directory,
                         DialogService& dialog_service,
                         TaskManager& task_manager);
