#include "filesystem/filesystem_commands.h"

#include "aui/dialog_service.h"
#include "aui/prompt_dialog.h"
#include "aui/translation.h"
#include "controller/contents_model.h"
#include "controller/window_info.h"
#include "filesystem/file_manager.h"
#include "filesystem/file_registry.h"
#include "filesystem/file_util.h"
#include "filesystem/filesystem_util.h"
#include "main_window/main_window_interface.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "profile/window_definition_util.h"
#include "resources/common_resources.h"
#include "services/task_manager.h"

#include <cassert>
#include <filesystem>
#include <fstream>

namespace {

// Message-box title. A function, not a constant: Translate() reads the
// installed catalog and so needs a running QApplication.
std::u16string AddFileTitle() {
  return Translate("Add File");
}
std::u16string OpenFileTitle() {
  return Translate("Open File");
}

}  // namespace

Awaitable<void> OpenJsonFileAsync(std::filesystem::path path,
                                  MainWindowInterface* main_window,
                                  DialogService& dialog_service,
                                  scada::aui::KeyModifiers /*key_modifiers*/,
                                  AnyExecutor executor) {
  if (!main_window) {
    co_return;
  }

  std::string error_string;
  auto json = LoadJsonFromFile(path, &error_string);

  std::optional<WindowDefinition> window_def;
  if (json) {
    window_def = FromJson<WindowDefinition>(*json);
  }

  if (!window_def) {
    co_await dialog_service.RunMessageBox(
        Translate("The file has an invalid format."), OpenFileTitle(),
        MessageBoxMode::Error);
    co_return;
  }

  // FromJson<WindowDefinition> accepts any object, so a .workplace with no
  // usable `type` parses here and fails in OpenView, which logs "Window type
  // not found" and returns null. Report it the way a syntax error is: to the
  // operator, silence reads as a click that did nothing.
  OpenedViewInterface* view =
      co_await main_window->OpenView(*window_def, /*activate=*/true);
  if (!view) {
    co_await dialog_service.RunMessageBox(
        Translate("The file has an invalid format."), OpenFileTitle(),
        MessageBoxMode::Error);
  }
  co_return;
}

Awaitable<void> OpenFileCommandImpl::OpenFileAsync(
    OpenFileCommandContext context) const {
  if (!context.main_window) {
    co_return;
  }

  auto path = GetFilePath(context.file_node);
  if (path.empty()) {
    co_return;
  }

  if (path.extension() == ".workplace") {
    co_await OpenJsonFileAsync(std::move(path), context.main_window,
                               context.dialog_service, context.key_modifiers,
                               context.executor);
    co_return;
  }

  auto* file_type =
      file_registry.FindTypeByExtension(path.extension().string());
  auto* window_info = file_type ? FindWindowInfo(file_type->type_id) : nullptr;
  if (!window_info) {
    co_await context.dialog_service.RunMessageBox(
        Translate("Unknown file type."), OpenFileTitle(), MessageBoxMode::Error);
    co_return;
  }

  co_await file_manager.DownloadFileFromServer(context.file_node, path);
  if (!std::filesystem::exists(GetPublicFilePath(path))) {
    co_await context.dialog_service.RunMessageBox(
        Translate("Failed to download file."), OpenFileTitle(),
        MessageBoxMode::Error);
    co_return;
  }

  auto window_def = WindowDefinition{*window_info}.set_path(std::move(path));
  co_await context.main_window->OpenView(std::move(window_def),
                                         /*activate=*/true);
  co_return;
}

Awaitable<void> OpenFileCommandImpl::ExecuteAsync(
    OpenFileCommandContext context) const {
  co_await OpenFileAsync(context);
}

Awaitable<void> OpenFileCommandImpl::Execute(
    const OpenFileCommandContext& context) const {
  co_await ExecuteAsync(context);
}

namespace {

Awaitable<void> AddFileAsync(NodeRef parent_directory,
                             DialogService& dialog_service,
                             TaskManager& task_manager,
                             AnyExecutor executor) {
  auto path = co_await dialog_service.SelectOpenFile(AddFileTitle());

  std::ifstream ifs{path, std::ios::binary};
  std::string contents_string{std::istreambuf_iterator<char>{ifs}, {}};
  if (!ifs.is_open()) {
    co_await dialog_service.RunMessageBox(Translate("Failed to read file."),
                                          AddFileTitle(), MessageBoxMode::Error);
    co_return;
  }

  scada::LocalizedText new_file_name = path.filename().u16string();
  scada::ByteString contents{contents_string.begin(), contents_string.end()};

  (void)co_await task_manager.PostInsertTask(
      {.type_definition_id = scada::filesystem::id::FileType,
       .parent_id = parent_directory.node_id(),
       .attributes = {.display_name = std::move(new_file_name),
                      .value = std::move(contents)}});
  co_return;
}

}  // namespace

Awaitable<void> AddFile(NodeRef parent_directory,
                        DialogService& dialog_service,
                        TaskManager& task_manager,
                        AnyExecutor executor) {
  co_await AddFileAsync(std::move(parent_directory), dialog_service,
                        task_manager, std::move(executor));
}
