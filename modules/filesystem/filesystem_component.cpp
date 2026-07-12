#include "filesystem/filesystem_component.h"

#include "aui/translation.h"
#include "base/check.h"
#include "base/client_paths.h"
#include "base/path_service.h"
#include "controller/action.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_registry.h"
#include "core/default_node_command_registry.h"
#include "core/global_command_context.h"
#include "core/selection_command_context.h"
#include "filesystem/file_cache.h"
#include "filesystem/file_manager_impl.h"
#include "filesystem/file_registry.h"
#include "filesystem/file_synchronizer.h"
#include "filesystem/filesystem_commands.h"
#include "filesystem/filesystem_view.h"
#include "main_window/main_window_interface.h"
#include "model/filesystem_node_ids.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "resources/common_resources.h"
#include "services/create_tree.h"

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#endif
#include <cstdlib>
#include <filesystem>

namespace {

const WindowInfo kWindowInfo = {
    ID_FILE_SYSTEM_VIEW, "FileSystemView", u"Files", WIN_SING, 200, 400};

REGISTER_CONTROLLER(FileSystemView, kWindowInfo);

void OpenPublicFolder() {
  std::filesystem::path path;
  if (!base::PathService::Get(client::DIR_PUBLIC, &path)) {
    return;
  }

#ifdef _WIN32
  ShellExecuteW(/*hwnd=*/nullptr, /*lpOperation=*/L"open",
                /*lpFile=*/path.wstring().c_str(), /*lpParameters=*/nullptr,
                /*lpDirectory=*/nullptr,
                /*nShowCmd=*/SW_SHOWNORMAL);
#else
  std::string command = "open '";
  for (char ch : path.string()) {
    if (ch == '\'') {
      command += "'\\''";
    } else {
      command += ch;
    }
  }
  command += "'";
  std::system(command.c_str());
#endif
}

}  // namespace

// FileSystemComponent

FileSystemComponent::FileSystemComponent(FileSystemComponentContext&& context)
    : FileSystemComponentContext{std::move(context)} {
  file_registry_ = std::make_unique<FileRegistry>();
  file_cache_ = std::make_unique<FileCache>(*file_registry_);

  file_manager_ = std::make_unique<FileManagerImpl>(FileManagerContext{
      .executor_ = executor_, .scada_client_ = scada_client_});

  ui_command_registry_.AddMenuItem({.menu_id = MainMenuId::More,
                                    .order = 130,
                                    .command_id = ID_FILE_SYSTEM_VIEW,
                                    .title = Translate("Files"),
                                    .checkable = true});
  ui_command_registry_.AddAction(
      Action{.command_id_ = ID_CREATE_FILE_DIRECTORY,
             .category_ = CATEGORY_CREATE,
             .title_ = Translate("Folder"),
             .short_title_ = Translate("Create Folder...")});
  ui_command_registry_.AddAction(
      Action{.command_id_ = ID_ADD_FILE,
             .category_ = CATEGORY_CREATE,
             .title_ = Translate("File"),
             .short_title_ = Translate("Add File...")});

  global_commands_.AddCommand(BasicCommand<GlobalCommandContext>{
      .command_id = ID_VIEW_PUBLIC_FOLDER,
      .title = Translate("Open Displays Folder"),
      .execute_handler =
          [](const GlobalCommandContext&) { OpenPublicFolder(); },
      .available_handler =
          [](const GlobalCommandContext& context) {
            return context.main_window.GetActiveView() != nullptr;
          }});
  ui_command_registry_.AddMenuItem({.menu_id = MainMenuId::Settings,
                                    .order = 400,
                                    .command_id = ID_VIEW_PUBLIC_FOLDER,
                                    .separator_before = true});

  auto open_file_command = std::bind_front(
      &OpenFileCommandImpl::Execute,
      std::make_shared<OpenFileCommandImpl>(*file_registry_, *file_manager_));
  default_node_commands_.AddHandler(
      [executor = executor_, open_file_command = std::move(open_file_command)](
          const NodeCommandContext& context) {
        if (!IsInstanceOf(context.node, filesystem::id::FileType)) {
          return false;
        }

        CoSpawn(
            executor,
            [open_file_command, main_window = context.main_window,
             &dialog_service = context.dialog_service, executor,
             node = context.node,
             key_modifiers =
                 context.key_modifiers]() mutable -> Awaitable<void> {
              co_await open_file_command(OpenFileCommandContext{
                  main_window, dialog_service, executor, node, key_modifiers});
            });
        return true;
      });

  /*std::filesystem::path public_dir;
  if (base::PathService::Get(client::DIR_PUBLIC, &public_dir)) {
    file_synchronizer_ =
        std::make_unique<FileSynchronizer>(FileSynchronizerContext{
            executor_,
            std::make_shared<BoostLogger>(LOG_NAME("FileSynchronizer")),
            *node_service_,
            public_dir.value(),
        });
  }*/
}

FileSystemComponent::~FileSystemComponent() {}

void FileSystemComponent::StartUp() {
  AddFileCommand(ID_ADD_FILE, filesystem::id::FileType);
  AddFileCommand(ID_CREATE_FILE_DIRECTORY, filesystem::id::FileDirectoryType);

  file_cache_->Init();
}

void FileSystemComponent::AddFileCommand(
    unsigned command_id,
    const scada::NodeId& type_definition_id) {
  base::Check(selection_commands_);

  const auto& file_type = node_service_.GetNode(type_definition_id);
  file_type.StartFetch(NodeFetchStatus::NodeOnly);

  selection_commands_->AddCommand(
      BasicCommand<SelectionCommandContext>{command_id}
          .set_execute_handler([this](const SelectionCommandContext& context) {
            return AddFile(context.selection.node(), context.dialog_service,
                           task_manager_, executor_);
          })
          .set_available_handler([this, file_type](
                                     const SelectionCommandContext& context) {
            return create_tree_.CanCreate(context.selection.node(), file_type);
          }));
}
