#define _SILENCE_CXX20_CODECVT_FACETS_DEPRECATION_WARNING

#include "export/configuration/excel_configuration_commands.h"

#include "aui/dialog_service.h"
#include "aui/resource_error.h"
#include "base/csv_reader.h"
#include "base/csv_writer.h"
#include "base/strings/stringprintf.h"
#include "base/win/win_util2.h"
#include "export/configuration/diff_data_builder.h"
#include "export/configuration/export_data_builder.h"
#include "export/configuration/export_data_reader.h"
#include "export/configuration/export_data_writer.h"
#include "export/configuration/import_data_builder.h"
#include "export/configuration/import_data_report.h"
#include "export/configuration/importer.h"
#include "node_service/node_promises.h"
#include "node_service/node_service.h"

#include <algorithm>
#include <fstream>

namespace {

const char16_t kImportTitle[] = u"Импорт";
const char16_t kExportTitle[] = u"Экспорт";
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
    throw ResourceError{u"Не удалось открыть файл."};
  }

  return CollectExportData()
      .then([this, stream = std::make_shared<std::ofstream>(std::move(stream))](
                const ExportData& export_data) {
        SaveExportData(export_data, *stream);
      })
      .then([&dialog_service] {
        return dialog_service.RunMessageBox(
            u"Экспорт завершен. Открыть файл сейчас?", kExportTitle,
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
  ExportData export_data = LoadExportData(path);

  ImportData import_data = BuildImportData(node_service_, export_data);
  if (import_data.IsEmpty()) {
    return ToVoidPromise(dialog_service.RunMessageBox(
        u"Изменений не найдено.", kImportTitle, MessageBoxMode::Info));
  }

  ShowImportReport(import_data, node_service_);

  return dialog_service
      .RunMessageBox(u"Применить изменения?", kImportTitle,
                     MessageBoxMode::QuestionYesNoDefaultNo)
      .then([import_data = std::move(import_data),
             this](MessageBoxResult apply_prompt) {
        if (apply_prompt == MessageBoxResult::Yes) {
          ApplyImportData(import_data, task_manager_);
        }
      });
}

promise<void> ImportConfigurationCommand::ImportFrom2(
    const std::filesystem::path& path,
    DialogService& dialog_service) const {
  auto diff_promise = ExportDataBuilder{node_service_}.Build().then(
      [this, path](const ExportData& old_export_data) {
        ExportData new_export_data = LoadExportData(path);
        DiffData diff = BuildDiffData(old_export_data, new_export_data);
        if (diff.IsEmpty()) {
          throw ResourceError{u"Изменений не найдено."};
        }
        ShowDiffReport(diff, node_service_);
        return diff;
      });

  return diff_promise
      .then([&dialog_service](const DiffData& diff) {
        return dialog_service.RunMessageBox(
            u"Применить изменения?", kImportTitle,
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
    throw ResourceError{u"Не удалось открыть файл."};
  }

  CsvReader csv_reader{stream, kNodeIdTitle};
  try {
    ExportDataReader reader{node_service_, csv_reader};
    return reader.Read();

  } catch (const ResourceError& e) {
    throw ResourceError{base::StringPrintf(
        u"Ошибка при импорте строки %d, столбца %d: %ls.",
        csv_reader.row_index(), csv_reader.cell_index(), e.message().c_str())};
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
            return ImportFrom2(path, dialog_service);
          }));
}
