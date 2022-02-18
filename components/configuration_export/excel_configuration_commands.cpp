#include "components/configuration_export/excel_configuration_commands.h"

#include "base/base_paths.h"
#include "base/csv_reader.h"
#include "base/csv_writer.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/win/win_util2.h"
#include "components/configuration_export/export_data_builder.h"
#include "components/configuration_export/export_data_reader.h"
#include "components/configuration_export/export_data_writer.h"
#include "components/configuration_export/import_data_builder.h"
#include "components/configuration_export/import_data_report.h"
#include "components/configuration_export/importer.h"
#include "components/configuration_export/resource_error.h"
#include "services/dialog_service.h"

#include <algorithm>
#include <fstream>

namespace {

const char16_t kImportTitle[] = u"Импорт";
const char16_t kExportTitle[] = u"Экспорт";
const char kDefaultFileName[] = "configuration.csv";

}  // namespace

void ShowImportReport(const ImportData& import_data,
                      NodeService& node_service) {
  std::basic_ofstream<char16_t> report("report.txt");
  PrintImportReport(report, import_data, node_service);

  base::FilePath system_path;
  base::PathService::Get(base::DIR_WINDOWS, &system_path);

  auto command_line = L"\"" + system_path.AsEndingWithSeparator().value() +
                      L"notepad.exe\" report.txt";

  STARTUPINFO startup_info = {sizeof(startup_info)};
  PROCESS_INFORMATION process_info = {};
  if (!CreateProcess(nullptr, const_cast<LPTSTR>(command_line.c_str()), nullptr,
                     nullptr, FALSE, 0, nullptr, nullptr, &startup_info,
                     &process_info))
    throw ResourceError{u"Не удалось запустить Блокнот"};
  ::WaitForSingleObject(process_info.hProcess, INFINITE);
  CloseHandle(process_info.hProcess);
  CloseHandle(process_info.hThread);
}

template <class Func>
void CatchExportException(DialogService& dialog_service, Func&& func) {
  try {
    func();
  } catch (const ResourceError& e) {
    dialog_service.RunMessageBox((e.message() + u".").c_str(), kExportTitle,
                                 MessageBoxMode::Error);
  } catch (const std::runtime_error&) {
    dialog_service.RunMessageBox(u"Ошибка при экспорте.", kExportTitle,
                                 MessageBoxMode::Error);
  }
}

void ExportConfigurationToExcel(NodeService& node_service,
                                DialogService& dialog_service,
                                const std::filesystem::path& path) {
  CatchExportException(dialog_service, [&] {
    std::ofstream stream{path};
    if (!stream)
      throw ResourceError{u"Не удалось открыть файл."};

    ExportDataBuilder{node_service}.Build().then(
        [stream = std::make_shared<std::ofstream>(std::move(stream)),
         &dialog_service, path](const ExportData& export_data) {
          CatchExportException(dialog_service, [&] {
            CsvWriter writer{*stream};
            WriteExportData(export_data, writer);

            dialog_service
                .RunMessageBox(u"Экспорт завершен. Открыть файл сейчас?",
                               kExportTitle, MessageBoxMode::QuestionYesNo)
                .then([path](MessageBoxResult message_box_result) {
                  if (message_box_result == MessageBoxResult::Yes)
                    win_util::OpenWithAssociatedProgram(path);
                });
          });
        });
  });
}

void ExportConfigurationToExcel(NodeService& node_service,
                                DialogService& dialog_service) {
  auto path = dialog_service.SelectSaveFile({kExportTitle, kDefaultFileName});
  if (path.empty())
    return;

  ExportConfigurationToExcel(node_service, dialog_service, path);
}

void ImportConfigurationFromExcel(NodeService& node_service,
                                  TaskManager& task_manager,
                                  DialogService& dialog_service,
                                  const std::filesystem::path& path) {
  std::ifstream stream{path};
  if (!stream) {
    dialog_service.RunMessageBox(u"Не удалось открыть файл.", kImportTitle,
                                 MessageBoxMode::Error);
    return;
  }

  CsvReader reader{stream, kNodeIdTitle};
  ImportData import_data;
  try {
    auto export_data = ExportDataReader{node_service, reader}.Read();

    import_data = BuildImportData(node_service, export_data);

  } catch (const ResourceError& e) {
    auto message = base::StringPrintf(
        u"Ошибка при импорте строки %d, столбца %d: %ls.", reader.row_index(),
        reader.cell_index(), e.message().c_str());
    dialog_service.RunMessageBox(message.c_str(), kImportTitle,
                                 MessageBoxMode::Error);
    return;
  }

  if (import_data.IsEmpty()) {
    dialog_service.RunMessageBox(u"Изменений не найдено.", kImportTitle,
                                 MessageBoxMode::Info);
    return;
  }

  ShowImportReport(import_data, node_service);

  dialog_service
      .RunMessageBox(u"Применить изменения?", kImportTitle,
                     MessageBoxMode::QuestionYesNoDefaultNo)
      .then([import_data = std::move(import_data),
             &task_manager](MessageBoxResult message_box_result) {
        if (message_box_result == MessageBoxResult::Yes)
          ApplyImportData(import_data, task_manager);
      });
}

void ImportConfigurationFromExcel(NodeService& node_service,
                                  TaskManager& task_manager,
                                  DialogService& dialog_service) {
  auto path = dialog_service.SelectOpenFile(kImportTitle);
  if (path.empty())
    return;

  ImportConfigurationFromExcel(node_service, task_manager, dialog_service,
                               path);
}
