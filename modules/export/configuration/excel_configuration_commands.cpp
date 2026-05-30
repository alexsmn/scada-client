

#include "export/configuration/excel_configuration_commands.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "aui/resource_error.h"
#include "base/csv_reader.h"
#include "base/csv_writer.h"
#include "base/any_executor.h"
#include "base/u16format.h"
#ifdef _WIN32
#include "base/win/win_util2.h"
#endif
#include "export/configuration/diff_builder.h"
#include "export/configuration/diff_report.h"
#include "export/configuration/export_data_builder.h"
#include "export/configuration/export_data_reader.h"
#include "export/configuration/export_data_writer.h"
#include "export/configuration/importer.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_service.h"

#include <algorithm>
#ifndef _WIN32
#include <cstdlib>
#include <string>
#endif
#include <fstream>

namespace {

const char16_t kImportTitle[] = u"Import";
const char16_t kExportTitle[] = u"Export";
const char kDefaultFileName[] = "configuration.csv";

void OpenWithAssociatedProgram(const std::filesystem::path& path) {
#ifdef _WIN32
  win_util::OpenWithAssociatedProgram(path);
#else
  std::string command = "open '";
  for (char ch : path.string()) {
    if (ch == '\'')
      command += "'\\''";
    else
      command += ch;
  }
  command += "'";
  std::system(command.c_str());
#endif
}

}  // namespace

Awaitable<ExportData> ExportConfigurationCommand::CollectExportData() const {
  co_return co_await ExportDataBuilder{node_service_, executor_}.BuildAsync(
      executor_);
}

void ExportConfigurationCommand::SaveExportData(const ExportData& export_data,
                                                std::ostream& stream) const {
  CsvWriter writer{stream};
  WriteExportData(export_data, writer);
}

Awaitable<void> ExportConfigurationCommand::ExportTo(
    const std::filesystem::path& path,
    DialogService& dialog_service) const {
  std::ofstream stream{path};
  if (!stream) {
    throw ResourceError{Translate("Failed to open file.")};
  }

  auto export_data = co_await CollectExportData();
  SaveExportData(export_data, stream);

  auto open_prompt = co_await dialog_service.RunMessageBox(
      Translate("Export complete. Open the file now?"), kExportTitle,
      MessageBoxMode::QuestionYesNo);
  if (open_prompt == MessageBoxResult::Yes) {
    OpenWithAssociatedProgram(path);
  }
  co_return;
}

Awaitable<void> ExportConfigurationCommand::Execute(
    DialogService& dialog_service) const {
  co_await FetchTypeSystem(node_service_);
  auto path = co_await dialog_service.SelectSaveFile(
      {.title = kExportTitle, .default_path = kDefaultFileName});

  std::exception_ptr resource_error;
  try {
    co_await ExportTo(path, dialog_service);
  } catch (const ResourceError&) {
    resource_error = std::current_exception();
  }
  if (resource_error) {
    co_await ShowResourceError<void>(dialog_service, kExportTitle,
                                     resource_error);
  }
  co_return;
}

Awaitable<void> ImportConfigurationCommand::ImportFrom(
    const std::filesystem::path& path,
    DialogService& dialog_service) const {
  auto old_export_data =
      co_await ExportDataBuilder{node_service_, executor_}.BuildAsync(
          executor_);
  ExportData new_export_data = LoadExportData(path);
  DiffData diff = BuildDiffData(old_export_data, new_export_data);
  if (diff.IsEmpty()) {
    throw ResourceError{Translate("No changes found")};
  }
  ShowDiffReport(diff, node_service_);

  auto apply_prompt = co_await dialog_service.RunMessageBox(
      Translate("Apply changes?"), kImportTitle,
      MessageBoxMode::QuestionYesNoDefaultNo);
  if (apply_prompt == MessageBoxResult::Yes) {
    ApplyDiffData(diff, task_manager_);
  }
  co_return;
}

ExportData ImportConfigurationCommand::LoadExportData(
    const std::filesystem::path& path) const {
  std::ifstream stream{path};
  if (!stream) {
    throw ResourceError{Translate("Failed to open file.")};
  }

  CsvReader csv_reader{stream, kNodeIdTitle};
  try {
    ExportDataReader reader{node_service_, csv_reader};
    return reader.Read();

  } catch (const ResourceError& e) {
    throw ResourceError{u16format(
        L"Error importing row {}, column {}: {}.",
        csv_reader.row_index(), csv_reader.cell_index(), e.message())};
  }
}

Awaitable<void> ImportConfigurationCommand::Execute(
    DialogService& dialog_service) const {
  co_await FetchTypeSystem(node_service_);
  auto path = co_await dialog_service.SelectOpenFile(kImportTitle);

  std::exception_ptr resource_error;
  try {
    co_await ImportFrom(path, dialog_service);
  } catch (const ResourceError&) {
    resource_error = std::current_exception();
  }
  if (resource_error) {
    co_await ShowResourceError<void>(dialog_service, kImportTitle,
                                     resource_error);
  }
  co_return;
}
