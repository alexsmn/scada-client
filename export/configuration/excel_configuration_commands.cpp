#define _SILENCE_CXX20_CODECVT_FACETS_DEPRECATION_WARNING

#include "export/configuration/excel_configuration_commands.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "aui/resource_error.h"
#include "base/csv_reader.h"
#include "base/csv_writer.h"
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
  std::ofstream stream{path};
  if (!stream) {
    throw ResourceError{Translate("Failed to open file.")};
  }

  return CollectExportData()
      .then([this, stream = std::make_shared<std::ofstream>(std::move(stream))](
                const ExportData& export_data) {
        SaveExportData(export_data, *stream);
      })
      .then([&dialog_service] {
        return dialog_service.RunMessageBox(
            Translate("Export complete. Open the file now?"), kExportTitle,
            MessageBoxMode::QuestionYesNo);
      })
      .then([path](MessageBoxResult open_prompt) {
        if (open_prompt == MessageBoxResult::Yes) {
          win_util::OpenWithAssociatedProgram(path);
        }
      });
}

promise<void> ExportConfigurationCommand::Execute(
    DialogService& dialog_service) const {
  return FetchTypeSystem(node_service_)
      .then([&dialog_service] {
        return dialog_service.SelectSaveFile(
            {.title = kExportTitle, .default_path = kDefaultFileName});
      })
      .then(CatchResourceError(
          dialog_service, kExportTitle,
          [this, &dialog_service](const std::filesystem::path& path) {
            return ExportTo(path, dialog_service);
          }));
}

promise<void> ImportConfigurationCommand::ImportFrom(
    const std::filesystem::path& path,
    DialogService& dialog_service) const {
  auto diff_promise = ExportDataBuilder{node_service_}.Build().then(
      [this, path](const ExportData& old_export_data) {
        ExportData new_export_data = LoadExportData(path);
        DiffData diff = BuildDiffData(old_export_data, new_export_data);
        if (diff.IsEmpty()) {
          throw ResourceError{Translate("No changes found")};
        }
        ShowDiffReport(diff, node_service_);
        return diff;
      });

  return diff_promise
      .then([&dialog_service](const DiffData& diff) {
        return dialog_service.RunMessageBox(
            Translate("Apply changes?"), kImportTitle,
            MessageBoxMode::QuestionYesNoDefaultNo);
      })
      .then([this, diff_promise](MessageBoxResult apply_prompt) {
        if (apply_prompt == MessageBoxResult::Yes) {
          ApplyDiffData(diff_promise.get(), task_manager_);
        }
      });
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
  return FetchTypeSystem(node_service_)
      .then([&dialog_service] {
        return dialog_service.SelectOpenFile(kImportTitle);
      })
      .then(CatchResourceError(
          dialog_service, kImportTitle,
          [this, &dialog_service](const std::filesystem::path& path) {
            return ImportFrom(path, dialog_service);
          }));
}
