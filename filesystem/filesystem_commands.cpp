#include "filesystem/filesystem_commands.h"

#include "base/file_path_util.h"
#include "base/promise_executor.h"
#include "common_resources.h"
#include "components/main/main_window.h"
#include "components/main/main_window_util.h"
#include "components/main/opened_view.h"
#include "components/prompt/prompt_dialog.h"
#include "contents_model.h"
#include "controller/window_definition_util.h"
#include "controller/window_info.h"
#include "filesystem/file_registry.h"
#include "filesystem/file_util.h"
#include "filesystem/filesystem_util.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "services/dialog_service.h"
#include "services/task_manager.h"

#include <cassert>
#include <filesystem>

namespace {

const char16_t kAddFileTitle[] = u"Добавить файл";
const char16_t kOpenFileTitle[] = u"Открыть файл";
const char16_t kAddFileDirectoryPrompt[] = u"Имя папки:";
const char16_t kAddFileDirectoryTitle[] = u"Создать папку";

}  // namespace

void OpenJsonFile(const std::filesystem::path& path,
                  MainWindow* main_window,
                  aui::KeyModifiers key_modifiers) {
  if (!main_window)
    return;

  std::optional<WindowDefinition> window;

  std::string error_string;
  auto json = LoadJsonFromFile(path, &error_string);
  if (json)
    window = FromJson<WindowDefinition>(*json);

  if (!window) {
    main_window->GetDialogService().RunMessageBox(
        u"Файл имеет неверный формат.", kOpenFileTitle, MessageBoxMode::Error);
    return;
  }

  OpenView(main_window, *window);
}

void OpenFile(const std::filesystem::path& path,
              const FileRegistry& file_registry,
              MainWindow* main_window,
              aui::KeyModifiers key_modifiers) {
  if (!main_window)
    return;

  if (path.extension() == ".workplace") {
    OpenJsonFile(path, main_window, key_modifiers);
    return;
  }

  auto* file_type =
      file_registry.FindTypeByExtension(path.extension().string());
  auto* window_info = file_type ? FindWindowInfo(file_type->type_id) : nullptr;
  if (!window_info) {
    main_window->GetDialogService().RunMessageBox(
        u"Неизвестный тип файла.", kOpenFileTitle, MessageBoxMode::Error);
    return;
  }

  WindowDefinition window{*window_info};
  window.path = path;
  OpenView(main_window, window);
}

// Returns a relative path from public path.
promise<std::filesystem::path> DownloadFile(const NodeRef& file_node) {
  auto path = GetFilePath(file_node);
  if (path.empty()) {
    return scada::MakeRejectedStatusPromise<std::filesystem::path>(
        scada::StatusCode::Bad);
  }

  return file_node.scada_node()
      .read(scada::AttributeId::Value)
      .then(
          [path = std::move(path)](const scada::DataValue& data_value) mutable {
            const auto* contents = data_value.value.get_if<scada::ByteString>();
            if (!contents) {
              throw scada::status_exception{scada::StatusCode::Bad};
            }

            int written = base::WriteFile(AsFilePath(GetPublicFilePath(path)),
                                          contents->data(), contents->size());
            if (written != static_cast<int>(contents->size())) {
              throw scada::status_exception{scada::StatusCode::Bad};
            }

            return path;
          });
}

promise<> ExecuteFileCommand(MainWindow* main_window,
                             const std::shared_ptr<Executor>& executor,
                             const FileRegistry& file_registry,
                             const NodeRef& file_node,
                             aui::KeyModifiers key_modifiers) {
  if (!main_window) {
    return scada::MakeRejectedStatusPromise(scada::StatusCode::Bad);
  }

  // TODO: `weak ptr` for main window.
  return DownloadFile(file_node)
      .then(BindPromiseExecutor(executor,
                                [&file_registry, main_window, key_modifiers](
                                    const std::filesystem::path& path) {
                                  OpenFile(path, file_registry, main_window,
                                           key_modifiers);
                                }))
      .except(BindPromiseExecutorWithResult(executor, [main_window](
                                                          std::exception_ptr) {
        return main_window->GetDialogService()
            .RunMessageBox(u"Не удалось загрузить файл с сервера.",
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
        std::string contents_string;
        if (!base::ReadFileToString(AsFilePath(path), &contents_string)) {
          return ToRejectedPromise(dialog_service.RunMessageBox(
              u"Не удалось считать файл.", kAddFileTitle,
              MessageBoxMode::Error));
        }

        scada::LocalizedText new_file_name = path.filename().u16string();
        scada::ByteString contents{contents_string.begin(),
                                   contents_string.end()};

        return ToVoidPromise(task_manager.PostInsertTask(
            {}, parent_directory.node_id(), filesystem::id::FileType,
            scada::NodeAttributes{.display_name = std::move(new_file_name),
                                  .value = std::move(contents)},
            {}, {}));
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
            {}, parent_directory.node_id(), filesystem::id::FileDirectoryType,
            {.display_name = new_directory_name}, {}, {}));
      });
}
