#include "components/filesystem/filesystem_commands.h"

#include "base/executor.h"
#include "client_utils.h"
#include "common_resources.h"
#include "components/filesystem/filesystem_util.h"
#include "components/main/main_window.h"
#include "components/main/main_window_util.h"
#include "components/main/opened_view.h"
#include "contents_model.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "services/dialog_service.h"
#include "services/file_registry.h"
#include "value_util.h"
#include "window_definition_util.h"
#include "window_info.h"

#include <cassert>
#include <filesystem>

namespace {
const wchar_t kOpenFileTitle[] = L"Открыть файл";
}

void OpenJsonFile(const std::filesystem::path& path,
                  MainWindow* main_window,
                  unsigned shift) {
  if (!main_window)
    return;

  std::optional<WindowDefinition> window;

  std::string error_string;
  auto json = LoadJsonFromFile(base::FilePath{path.native()}, &error_string);
  if (json)
    window = FromJson<WindowDefinition>(*json);

  if (!window) {
    main_window->GetDialogService().RunMessageBox(
        L"Файл имеет неверный формат.", kOpenFileTitle, MessageBoxMode::Error);
    return;
  }

  OpenView(main_window, *window);
}

void OpenFile(const std::filesystem::path& path,
              const FileRegistry& file_registry,
              MainWindow* main_window,
              unsigned shift) {
  if (!main_window)
    return;

  if (path.extension() == ".workplace") {
    OpenJsonFile(path, main_window, shift);
    return;
  }

  auto* file_type =
      file_registry.FindTypeByExtension(path.extension().string());
  auto* window_info = file_type ? FindWindowInfo(file_type->type_id) : nullptr;
  if (!window_info) {
    main_window->GetDialogService().RunMessageBox(
        L"Неизвестный тип файла.", kOpenFileTitle, MessageBoxMode::Error);
    return;
  }

  WindowDefinition window{*window_info};
  window.AddItem("Item").SetString("Path", path.native());
  OpenView(main_window, window);
}

using DownloadFileCallback =
    std::function<void(const std::filesystem::path& path)>;

void DownloadFile(const NodeRef& file_node,
                  const DownloadFileCallback& callback) {
  const auto& path = GetFilePath(file_node);
  if (path.empty()) {
    callback({});
    return;
  }

  file_node.Read(
      scada::AttributeId::Value, [path, callback](scada::DataValue data_value) {
        if (scada::IsBad(data_value.status_code)) {
          callback({});
          return;
        }

        const auto* contents = data_value.value.get_if<scada::ByteString>();
        if (!contents) {
          callback({});
          return;
        }

        base::WriteFile(GetPublicFilePath(base::FilePath{path.native()}),
                        contents->data(), contents->size());

        callback(path);
      });
}

bool ExecuteFileCommand(MainWindow* main_window,
                        const std::shared_ptr<Executor>& executor,
                        const FileRegistry& file_registry,
                        const NodeRef& file_node,
                        unsigned shift) {
  if (!main_window)
    return false;

  // TODO: Weak ptr for main window.
  DownloadFile(file_node,
               BindExecutor(executor, [&file_registry, main_window, shift](
                                          const std::filesystem::path& path) {
                 if (path.empty()) {
                   main_window->GetDialogService().RunMessageBox(
                       L"Не удалось загрузить файл с сервера.", kOpenFileTitle,
                       MessageBoxMode::Error);
                   return;
                 }

                 OpenFile(path, file_registry, main_window, shift);
               }));

  return true;
}
