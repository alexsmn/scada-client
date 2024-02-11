#include "filesystem/filesystem_component.h"

#include "controller/command_registry.h"
#include "controller/controller_registry.h"
#include "filesystem/file_manager.h"
#include "filesystem/file_synchronizer.h"
#include "filesystem/filesystem_commands.h"
#include "filesystem/filesystem_view.h"
#include "main_window/selection_command_context.h"
#include "node_service/node_service.h"
#include "services/create_tree.h"

const WindowInfo kWindowInfo = {
    ID_FILE_SYSTEM_VIEW, "FileSystemView", u"Файлы", WIN_SING, 200, 400};

REGISTER_CONTROLLER(FileSystemView, kWindowInfo);

FileSystemComponent::FileSystemComponent(FileSystemComponentContext&& context)
    : FileSystemComponentContext{std::move(context)} {
  file_manager_ = std::make_unique<FileManager>(
      FileManagerContext{attribute_service_, view_service_});

  /*std::filesystem::path public_dir;
  if (base::PathService::Get(client::DIR_PUBLIC, &public_dir)) {
    file_synchronizer_ =
        std::make_unique<FileSynchronizer>(FileSynchronizerContext{
            std::make_shared<NestedLogger>(logger_, "FileSynchronizer"),
            *node_service_,
            public_dir.value(),
        });
  }*/

  AddFileCommand(ID_ADD_FILE, filesystem::id::FileType);
  AddFileCommand(ID_CREATE_FILE_DIRECTORY, filesystem::id::FileDirectoryType);
}

FileSystemComponent::~FileSystemComponent() {}

void FileSystemComponent::AddFileCommand(
    unsigned command_id,
    const scada::NodeId& type_definition_id) {
  const auto& file_type = node_service_.GetNode(type_definition_id);
  file_type.Fetch(NodeFetchStatus::NodeOnly());

  selection_commands_.AddCommand(
      BasicCommand<SelectionCommandContext>{command_id}
          .set_execute_handler([this](const SelectionCommandContext& context) {
            return AddFile(context.selection.node(), context.dialog_service,
                           task_manager_);
          })
          .set_available_handler([this, file_type](
                                     const SelectionCommandContext& context) {
            return create_tree_.CanCreate(context.selection.node(), file_type);
          }));
}