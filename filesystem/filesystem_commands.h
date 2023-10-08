#pragma once

#include "base/promise.h"
#include "aui/key_codes.h"

#include <memory>

class DialogService;
class Executor;
class FileRegistry;
class MainWindow;
class NodeRef;
class TaskManager;

promise<> ExecuteFileCommand(MainWindow* main_window,
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
