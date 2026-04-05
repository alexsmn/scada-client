#include "filesystem/filesystem_commands.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "aui/prompt_dialog.h"
#include "base/promise_executor.h"
#include "common_resources.h"
#include "controller/contents_model.h"
#include "controller/window_info.h"
#include "filesystem/file_manager.h"
#include "filesystem/file_registry.h"
#include "filesystem/file_util.h"
#include "filesystem/filesystem_util.h"
#include "main_window/main_window.h"
#include "main_window/main_window_util.h"
#include "main_window/opened_view.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "profile/window_definition_util.h"
#include "scada/status_promise.h"
#include "services/task_manager.h"

#include <cassert>
#include <filesystem>
#include <fstream>

namespace {

const char16_t kAddFileTitle[] = u"Add File";
const char16_t kOpenFileTitle[] = u"Open File";
const char16_t kAddFileDirectoryPrompt[] = u"Folder name:";
const char16_t kAddFileDirectoryTitle[] = u"Create Folder";

}  // namespace

promise<void> OpenJsonFile(const std::filesystem::path& path,
                           MainWindowInterface* main_window,
                           DialogService& dialog_service,
                           aui::KeyModifiers key_modifiers) {
  if (!main_window) {
    return make_resolved_promise();
  }

  std::string error_string;
  auto json = LoadJsonFromFile(path, &error_string);

  std::optional<WindowDefinition> window_def;
  if (json) {
    window_def = FromJson<WindowDefinition>(*json);
  }

  if (!window_def) {
    return ToVoidPromise(dialog_service.RunMessageBox(
        Translate("The file has an invalid format."), kOpenFileTitle, MessageBoxMode::Error));
  }

  return OpenView(main_window, *window_def);
}

promise<void> OpenFileCommandImpl::OpenFile(
    const OpenFileCommandContext& context) const {
  if (!context.main_window) {
    return scada::MakeRejectedStatusPromise(scada::StatusCode::Bad);
  }

  auto path = GetFilePath(context.file_node);
  if (path.empty()) {
    return scada::MakeRejectedStatusPromise(scada::StatusCode::Bad);
  }

  if (path.extension() == ".workplace") {
    return OpenJsonFile(path, context.main_window, context.dialog_service,
                        context.key_modifiers);
  }

  auto* file_type =
      file_registry.FindTypeByExtension(path.extension().string());
  auto* window_info = file_type ? FindWindowInfo(file_type->type_id) : nullptr;
  if (!window_info) {
    return ToVoidPromise(context.dialog_service.RunMessageBox(
        Translate("Unknown file type."), kOpenFileTitle, MessageBoxMode::Error));
  }

  auto window_def = WindowDefinition{*window_info}.set_path(path);

  return OpenView(context.main_window, std::move(window_def));
}

promise<void> OpenFileCommandImpl::Execute(
    const OpenFileCommandContext& context) const {
  return OpenFile(context).except(BindPromiseExecutor(
      context.executor,
      // TODO: `weak ptr` for main window.
      [&dialog_service = context.dialog_service](std::exception_ptr) {
        return dialog_service
            .RunMessageBox(Translate("Failed to download file from server."),
                           kOpenFileTitle, MessageBoxMode::Error)
            .then([](MessageBoxResult) {
              return scada::MakeRejectedStatusPromise(scada::StatusCode::Bad);
            });
      }));
}

promise<> AddFile(NodeRef parent_directory,
                  DialogService& dialog_service,
                  TaskManager& task_manager) {
  return dialog_service.SelectOpenFile(kAddFileTitle)
      .then([parent_directory, &dialog_service,
             &task_manager](const std::filesystem::path& path) {
        std::ifstream ifs{path, std::ios::binary};
        std::string contents_string{std::istreambuf_iterator<char>{ifs}, {}};
        if (!ifs.is_open()) {
          return ToRejectedPromise(dialog_service.RunMessageBox(
              Translate("Failed to read file."), kAddFileTitle,
              MessageBoxMode::Error));
        }

        scada::LocalizedText new_file_name = path.filename().u16string();
        scada::ByteString contents{contents_string.begin(),
                                   contents_string.end()};

        return ToVoidPromise(task_manager.PostInsertTask(
            {.type_definition_id = filesystem::id::FileType,
             .parent_id = parent_directory.node_id(),
             .attributes = {.display_name = std::move(new_file_name),
                            .value = std::move(contents)}}));
      });
}

promise<> CreateFileDirectory(NodeRef parent_directory,
                              DialogService& dialog_service,
                              TaskManager& task_manager) {
  return RunPromptDialog(dialog_service, kAddFileDirectoryPrompt,
                         kAddFileDirectoryTitle)
      .then([parent_directory = std::move(parent_directory),
             &task_manager](const std::u16string& new_directory_name) {
        return ToVoidPromise(task_manager.PostInsertTask(
            {.parent_id = parent_directory.node_id(),
             .reference_type_id = filesystem::id::FileDirectoryType,
             .attributes = {.display_name = new_directory_name}}));
      });
}
