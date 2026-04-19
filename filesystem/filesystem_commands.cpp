#include "filesystem/filesystem_commands.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "aui/prompt_dialog.h"
#include "base/awaitable_promise.h"
#include "resources/common_resources.h"
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
#include "scada/status_exception.h"
#include "scada/status_promise.h"
#include "services/task_manager.h"

#include <cassert>
#include <filesystem>
#include <fstream>

namespace {

const char16_t kAddFileTitle[] = u"Add File";
const char16_t kOpenFileTitle[] = u"Open File";

}  // namespace

Awaitable<void> OpenJsonFileAsync(std::filesystem::path path,
                                  MainWindowInterface* main_window,
                                  DialogService& dialog_service,
                                  aui::KeyModifiers /*key_modifiers*/,
                                  std::shared_ptr<Executor> executor) {
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
    co_await AwaitPromise(
        NetExecutorAdapter{executor},
        ToVoidPromise(dialog_service.RunMessageBox(
            Translate("The file has an invalid format."), kOpenFileTitle,
            MessageBoxMode::Error)));
    co_return;
  }

  co_await AwaitPromise(
      NetExecutorAdapter{executor},
      ToVoidPromise(main_window->OpenView(*window_def, /*activate=*/true)));
}

Awaitable<void> OpenFileCommandImpl::OpenFileAsync(
    OpenFileCommandContext context) const {
  if (!context.main_window) {
    throw scada::status_exception{scada::StatusCode::Bad};
  }

  auto path = GetFilePath(context.file_node);
  if (path.empty()) {
    throw scada::status_exception{scada::StatusCode::Bad};
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
    co_await AwaitPromise(
        NetExecutorAdapter{context.executor},
        ToVoidPromise(context.dialog_service.RunMessageBox(
            Translate("Unknown file type."), kOpenFileTitle,
            MessageBoxMode::Error)));
    co_return;
  }

  auto window_def = WindowDefinition{*window_info}.set_path(std::move(path));
  co_await AwaitPromise(
      NetExecutorAdapter{context.executor},
      ToVoidPromise(context.main_window->OpenView(std::move(window_def),
                                                  /*activate=*/true)));
}

Awaitable<void> OpenFileCommandImpl::ExecuteAsync(
    OpenFileCommandContext context) const {
  // MSVC rejects `co_await` inside a `catch` block, so the error-path
  // message box is awaited after the `try` has unwound.
  bool open_failed = false;
  try {
    co_await OpenFileAsync(context);
  } catch (...) {
    open_failed = true;
  }
  if (!open_failed) {
    co_return;
  }
  co_await AwaitPromise(
      NetExecutorAdapter{context.executor},
      ToVoidPromise(context.dialog_service.RunMessageBox(
          Translate("Failed to download file from server."), kOpenFileTitle,
          MessageBoxMode::Error)));
  throw scada::status_exception{scada::StatusCode::Bad};
}

promise<void> OpenFileCommandImpl::Execute(
    const OpenFileCommandContext& context) const {
  auto executor = context.executor;
  return ToPromise(NetExecutorAdapter{executor}, ExecuteAsync(context));
}

namespace {

Awaitable<void> AddFileAsync(NodeRef parent_directory,
                             DialogService& dialog_service,
                             TaskManager& task_manager,
                             std::shared_ptr<Executor> executor) {
  auto path = co_await AwaitPromise(
      NetExecutorAdapter{executor},
      dialog_service.SelectOpenFile(kAddFileTitle));

  std::ifstream ifs{path, std::ios::binary};
  std::string contents_string{std::istreambuf_iterator<char>{ifs}, {}};
  if (!ifs.is_open()) {
    co_await AwaitPromise(
        NetExecutorAdapter{executor},
        ToVoidPromise(dialog_service.RunMessageBox(
            Translate("Failed to read file."), kAddFileTitle,
            MessageBoxMode::Error)));
    throw scada::status_exception{scada::StatusCode::Bad};
  }

  scada::LocalizedText new_file_name = path.filename().u16string();
  scada::ByteString contents{contents_string.begin(), contents_string.end()};

  co_await AwaitPromise(
      NetExecutorAdapter{executor},
      ToVoidPromise(task_manager.PostInsertTask(
          {.type_definition_id = filesystem::id::FileType,
           .parent_id = parent_directory.node_id(),
           .attributes = {.display_name = std::move(new_file_name),
                          .value = std::move(contents)}})));
}

}  // namespace

promise<> AddFile(NodeRef parent_directory,
                  DialogService& dialog_service,
                  TaskManager& task_manager,
                  std::shared_ptr<Executor> executor) {
  auto exec = executor;
  return ToPromise(NetExecutorAdapter{exec},
                   AddFileAsync(std::move(parent_directory), dialog_service,
                                task_manager, std::move(executor)));
}
