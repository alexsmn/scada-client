#pragma once

#include "controller/command_registry.h"

#include <memory>

class CreateTree;
class FileSynchronizer;
class NodeService;
class TaskManager;
struct SelectionCommandContext;

struct FileSystemComponentContext {
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  NodeService& node_service_;
  TaskManager& task_manager_;
  CreateTree& create_tree_;
};

// TODO: Rename to `FilesystemModule`.
class FileSystemComponent : private FileSystemComponentContext {
 public:
  explicit FileSystemComponent(FileSystemComponentContext&& context);
  ~FileSystemComponent();

 private:
  std::unique_ptr<FileSynchronizer> file_synchronizer_;
};
