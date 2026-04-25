

#include "export/configuration/excel_configuration_commands.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "aui/resource_error.h"
#include "base/awaitable_promise.h"
#include "base/csv_reader.h"
#include "base/csv_writer.h"
#include "base/executor_conversions.h"
#include "base/u16format.h"
#include "base/win/win_util2.h"
#include "export/configuration/diff_builder.h"
#include "export/configuration/diff_report.h"
#include "export/configuration/export_data_builder.h"
#include "export/configuration/export_data_reader.h"
#include "export/configuration/export_data_writer.h"
#include "export/configuration/importer.h"
#include "node_service/node_promises.h"
#include "node_service/node_service.h"

#include <algorithm>
#include <fstream>

namespace {

const char16_t kImportTitle[] = u"Import";
const char16_t kExportTitle[] = u"Export";
const char kDefaultFileName[] = "configuration.csv";

}  // namespace

promise<ExportData> ExportConfigurationCommand::CollectExportData() const {
  return ExportDataBuilder{node_service_}.Build();
}

void ExportConfigurationCommand::SaveExportData(const ExportData& export_data,
                                                std::ostream& stream) const {
  CsvWriter writer{stream};
  WriteExportData(export_data, writer);
}

promise<void> ExportConfigurationCommand::ExportTo(
    const std::filesystem::path& path,
    DialogService& dialog_service) const {
  return ToPromise(MakeAnyExecutor(executor_),
                   ExportToAsync(path, dialog_service));
}

Awaitable<void> ExportConfigurationCommand::ExportToAsync(
    const std::filesystem::path& path,
    DialogService& dialog_service) const {
  std::ofstream stream{path};
  if (!stream) {
    throw ResourceError{Translate("Failed to open file.")};
  }

  auto export_data = co_await AwaitPromise(MakeAnyExecutor(executor_),
                                           CollectExportData());
  SaveExportData(export_data, stream);

  auto open_prompt = co_await AwaitPromise(
      MakeAnyExecutor(executor_),
      dialog_service.RunMessageBox(
          Translate("Export complete. Open the file now?"), kExportTitle,
          MessageBoxMode::QuestionYesNo));
  if (open_prompt == MessageBoxResult::Yes) {
    win_util::OpenWithAssociatedProgram(path);
  }
  co_return;
}

promise<void> ExportConfigurationCommand::Execute(
    DialogService& dialog_service) const {
  return ToPromise(MakeAnyExecutor(executor_), ExecuteAsync(dialog_service));
}

Awaitable<void> ExportConfigurationCommand::ExecuteAsync(
    DialogService& dialog_service) const {
  co_await AwaitPromise(MakeAnyExecutor(executor_),
                        FetchTypeSystem(node_service_));
  auto path = co_await AwaitPromise(
      MakeAnyExecutor(executor_),
      dialog_service.SelectSaveFile(
          {.title = kExportTitle, .default_path = kDefaultFileName}));

  std::exception_ptr resource_error;
  try {
    co_await ExportToAsync(path, dialog_service);
  } catch (const ResourceError&) {
    resource_error = std::current_exception();
  }
  if (resource_error) {
    co_await AwaitPromise(
        MakeAnyExecutor(executor_),
        ShowResourceError<void>(dialog_service, kExportTitle, resource_error));
  }
  co_return;
}

promise<void> ImportConfigurationCommand::ImportFrom(
    const std::filesystem::path& path,
    DialogService& dialog_service) const {
  return ToPromise(MakeAnyExecutor(executor_),
                   ImportFromAsync(path, dialog_service));
}

Awaitable<void> ImportConfigurationCommand::ImportFromAsync(
    const std::filesystem::path& path,
    DialogService& dialog_service) const {
  auto old_export_data = co_await AwaitPromise(
      MakeAnyExecutor(executor_), ExportDataBuilder{node_service_}.Build());
  ExportData new_export_data = LoadExportData(path);
  DiffData diff = BuildDiffData(old_export_data, new_export_data);
  if (diff.IsEmpty()) {
    throw ResourceError{Translate("No changes found")};
  }
  ShowDiffReport(diff, node_service_);

  auto apply_prompt = co_await AwaitPromise(
      MakeAnyExecutor(executor_),
      dialog_service.RunMessageBox(Translate("Apply changes?"), kImportTitle,
                                   MessageBoxMode::QuestionYesNoDefaultNo));
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

promise<> ImportConfigurationCommand::Execute(
    DialogService& dialog_service) const {
  return ToPromise(MakeAnyExecutor(executor_), ExecuteAsync(dialog_service));
}

Awaitable<void> ImportConfigurationCommand::ExecuteAsync(
    DialogService& dialog_service) const {
  co_await AwaitPromise(MakeAnyExecutor(executor_),
                        FetchTypeSystem(node_service_));
  auto path = co_await AwaitPromise(MakeAnyExecutor(executor_),
                                    dialog_service.SelectOpenFile(kImportTitle));

  std::exception_ptr resource_error;
  try {
    co_await ImportFromAsync(path, dialog_service);
  } catch (const ResourceError&) {
    resource_error = std::current_exception();
  }
  if (resource_error) {
    co_await AwaitPromise(
        MakeAnyExecutor(executor_),
        ShowResourceError<void>(dialog_service, kImportTitle, resource_error));
  }
  co_return;
}
