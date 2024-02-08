#define _SILENCE_CXX20_CODECVT_FACETS_DEPRECATION_WARNING

#include "export/configuration/excel_configuration_commands.h"

#include "aui/dialog_service.h"
#include "aui/resource_error.h"
#include "base/base_paths.h"
#include "base/csv_reader.h"
#include "base/csv_writer.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/win/win_util2.h"
#include "export/configuration/export_data_builder.h"
#include "export/configuration/export_data_reader.h"
#include "export/configuration/export_data_writer.h"
#include "export/configuration/import_data_builder.h"
#include "export/configuration/import_data_report.h"
#include "export/configuration/importer.h"

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
                     &process_info)) {
    throw ResourceError{u"Не удалось открыть блокнот"};
  }
  ::WaitForSingleObject(process_info.hProcess, INFINITE);
  CloseHandle(process_info.hProcess);
  CloseHandle(process_info.hThread);
}

promise<ExportData> ExportConfigurationCommand::CollectExportData() const {
  return ExportDataBuilder{node_service_}.Build();
}

void ExportConfigurationCommand::SaveExportData(const ExportData& export_data,
                                                std::ostream& stream) const {
  CsvWriter writer{stream};
  WriteExportData(export_data, writer);
}

promise<void> ExportConfigurationCommand::ExportTo(
    const std::filesystem::path& path) const {
  std::ofstream stream{path};
  if (!stream) {
    throw ResourceError{u"Не удалось открыть файл."};
  }

  return CollectExportData()
      .then([this, stream = std::make_shared<std::ofstream>(std::move(stream))](
                const ExportData& export_data) {
        SaveExportData(export_data, *stream);
      })
      .then([this] {
        return dialog_service_.RunMessageBox(
            u"Экспорт завершен. Открыть файл сейчас?", kExportTitle,
            MessageBoxMode::QuestionYesNo);
      })
      .then([path](MessageBoxResult open_prompt) {
        if (open_prompt == MessageBoxResult::Yes) {
          win_util::OpenWithAssociatedProgram(path);
        }
      });
}

promise<> ExportConfigurationCommand::Execute() const {
  auto shared_copy = std::make_shared<ExportConfigurationCommand>(*this);
  return dialog_service_
      .SelectSaveFile({.title = kExportTitle, .default_path = kDefaultFileName})
      .then(CatchResourceError(
          dialog_service_, kExportTitle,
          std::bind_front(&ExportConfigurationCommand::ExportTo, shared_copy)));
}

promise<void> ImportConfigurationCommand::ImportFrom(
    const std::filesystem::path& path) const {
  std::ifstream stream{path};
  if (!stream) {
    throw ResourceError{u"Не удалось открыть файл."};
  }

  CsvReader reader{stream, kNodeIdTitle};
  ImportData import_data;
  try {
    auto export_data = ExportDataReader{node_service_, reader}.Read();

    import_data = BuildImportData(node_service_, export_data);

  } catch (const ResourceError& e) {
    throw ResourceError{base::StringPrintf(
        u"Ошибка при импорте строки %d, столбца %d: %ls.", reader.row_index(),
        reader.cell_index(), e.message().c_str())};
  }

  if (import_data.IsEmpty()) {
    return ToVoidPromise(dialog_service_.RunMessageBox(
        u"Изменений не найдено.", kImportTitle, MessageBoxMode::Info));
  }

  ShowImportReport(import_data, node_service_);

  return dialog_service_
      .RunMessageBox(u"Применить изменения?", kImportTitle,
                     MessageBoxMode::QuestionYesNoDefaultNo)
      .then([import_data = std::move(import_data),
             this](MessageBoxResult apply_prompt) {
        if (apply_prompt == MessageBoxResult::Yes) {
          ApplyImportData(import_data, task_manager_);
        }
      });
}

promise<> ImportConfigurationCommand::Execute() const {
  auto shared_copy = std::make_shared<ImportConfigurationCommand>(*this);
  return dialog_service_.SelectOpenFile(kImportTitle)
      .then(CatchResourceError(
          dialog_service_, kImportTitle,
          std::bind_front(&ImportConfigurationCommand::ImportFrom,
                          shared_copy)));
}
