#pragma once

#include "aui/key_codes.h"
#include "scada/status_promise.h"

#include <memory>

class DialogService;
class Executor;
class FileRegistry;
class MainWindow;
class NodeRef;
class TaskManager;

scada::status_promise<void> ExecuteFileCommand(
    MainWindow* main_window,
    const std::shared_ptr<Executor>& executor,
    const FileRegistry& file_registry,
    const NodeRef& file_node,
    aui::KeyModifiers key_modifiers);

promise<> AddFile(NodeRef parent_directory,
                  DialogService& dialog_service,
                  TaskManager& task_manager);

promise<> CreateFileDirectory(NodeRef parent_directory,
                              DialogService& dialog_service,
                              TaskManager& task_manager);
