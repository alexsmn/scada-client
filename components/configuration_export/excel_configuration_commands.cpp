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

const wchar_t kImportTitle[] = L"Импорт";
const wchar_t kExportTitle[] = L"Экспорт";
const char kDefaultFileName[] = "configuration.csv";

}  // namespace

void ShowImportReport(const ImportData& import_data,
                      NodeService& node_service) {
  std::wofstream report("report.txt");
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
    throw ResourceError{L"Не удалось запустить Блокнот"};
  ::WaitForSingleObject(process_info.hProcess, INFINITE);
  CloseHandle(process_info.hProcess);
  CloseHandle(process_info.hThread);
}

void ExportConfigurationToExcel(NodeService& node_service,
                                DialogService& dialog_service,
                                const std::filesystem::path& path) {
  try {
    std::ofstream stream{path};
    if (!stream)
      throw ResourceError{L"Не удалось открыть файл."};

    auto data = BuildExportData(node_service);

    CsvWriter writer{stream};
    WriteExportData(data, writer);

  } catch (const ResourceError& e) {
    dialog_service.RunMessageBox((e.message() + L".").c_str(), kExportTitle,
                                 MessageBoxMode::Error);
    return;

  } catch (const std::runtime_error&) {
    dialog_service.RunMessageBox(L"Ошибка при экспорте.", kExportTitle,
                                 MessageBoxMode::Error);
    return;
  }

  if (dialog_service.RunMessageBox(
          L"Экспорт завершен. Открыть файл сейчас?", kExportTitle,
          MessageBoxMode::QuestionYesNo) == MessageBoxResult::Yes)
    win_util::OpenWithAssociatedProgram(path);
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
    dialog_service.RunMessageBox(L"Не удалось открыть файл.", kImportTitle,
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
        L"Ошибка при импорте строки %d, столбца %d: %ls.", reader.row_index(),
        reader.cell_index(), e.message().c_str());
    dialog_service.RunMessageBox(message.c_str(), kImportTitle,
                                 MessageBoxMode::Error);
    return;
  }

  if (import_data.IsEmpty()) {
    dialog_service.RunMessageBox(L"Изменений не найдено.", kImportTitle,
                                 MessageBoxMode::Info);
    return;
  }

  ShowImportReport(import_data, node_service);

  bool confirmed =
      dialog_service.RunMessageBox(L"Применить изменения?", kImportTitle,
                                   MessageBoxMode::QuestionYesNoDefaultNo) ==
      MessageBoxResult::Yes;

  if (confirmed)
    ApplyImportData(import_data, task_manager);
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
