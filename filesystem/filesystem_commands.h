#pragma once

#include "aui/key_codes.h"
#include "node_service/node_ref.h"
#include "scada/status_promise.h"

#include <memory>

class DialogService;
class Executor;
class FileManager;
class FileRegistry;
class MainWindow;
class NodeRef;
class TaskManager;

struct OpenFileCommandContext {
  MainWindow* main_window;
  std::shared_ptr<Executor> executor;
  NodeRef file_node;
  aui::KeyModifiers key_modifiers;
};

using OpenFileCommand =
    std::function<void(const OpenFileCommandContext& context)>;

class OpenFileCommandImpl {
 public:
  scada::status_promise<void> Execute(
      const OpenFileCommandContext& context) const;

  const FileRegistry& file_registry;
  FileManager& file_manager;

 private:
  scada::status_promise<void> OpenFile(
      const OpenFileCommandContext& context) const;
};

promise<> AddFile(NodeRef parent_directory,
                  DialogService& dialog_service,
                  TaskManager& task_manager);

promise<> CreateFileDirectory(NodeRef parent_directory,
                              DialogService& dialog_service,
                              TaskManager& task_manager);
