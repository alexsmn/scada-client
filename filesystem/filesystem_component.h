#pragma once

#include "controller/command_registry.h"

#include <memory>

namespace scada {
class AttributeService;
class NodeId;
class ViewService;
}  // namespace scada

class CreateTree;
class FileSynchronizer;
class FileManager;
class NodeService;
class TaskManager;
struct SelectionCommandContext;

struct FileSystemComponentContext {
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
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

  FileManager& file_manager() { return *file_manager_; }

 private:
  void AddFileCommand(unsigned command_id,
                      const scada::NodeId& type_definition_id);

  std::unique_ptr<FileManager> file_manager_;
  std::unique_ptr<FileSynchronizer> file_synchronizer_;
};
