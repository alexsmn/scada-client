#pragma once

#include "filesystem/filesystem_commands.h"

#include <memory>

namespace scada {
class AttributeService;
class NodeId;
class ViewService;
}  // namespace scada

template <typename T>
class BasicCommandRegistry;

class CreateTree;
class FileCache;
class FileManager;
class FileRegistry;
class FileSynchronizer;
class NodeService;
class TaskManager;
struct SelectionCommandContext;

struct FileSystemComponentContext {
  NodeService& node_service_;
  TaskManager& task_manager_;
  CreateTree& create_tree_;
  scada::AttributeService& attribute_service_;
  scada::ViewService& view_service_;
};

// TODO: Rename to `FilesystemModule`.
class FileSystemComponent : private FileSystemComponentContext {
 public:
  explicit FileSystemComponent(FileSystemComponentContext&& context);
  ~FileSystemComponent();

  FileRegistry& file_registry() { return *file_registry_; }
  FileCache& file_cache() { return *file_cache_; }
  FileManager& file_manager() { return *file_manager_; }
  const OpenFileCommand& file_command() { return open_file_command_; }

  void set_selection_commands(
      BasicCommandRegistry<SelectionCommandContext>* selection_commands) {
    selection_commands_ = selection_commands;
  }

  // Must be called after:
  // - All file types are registered.
  // - `set_selection_commands` is called.
  void StartUp();

 private:
  void AddFileCommand(unsigned command_id,
                      const scada::NodeId& type_definition_id);

  BasicCommandRegistry<SelectionCommandContext>* selection_commands_ = nullptr;

  std::unique_ptr<FileRegistry> file_registry_;
  std::unique_ptr<FileCache> file_cache_;
  std::unique_ptr<FileManager> file_manager_;
  OpenFileCommand open_file_command_;
  std::unique_ptr<FileSynchronizer> file_synchronizer_;
};
