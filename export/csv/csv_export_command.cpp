#include "export/csv/csv_export_command.h"

#include "aui/dialog_service.h"
#include "base/strings/string_util.h"
#include "base/value_util.h"
#include "base/win/win_util2.h"
#include "export/csv/csv_export.h"
#include "export/export_model.h"
#include "main_window/opened_view.h"
#include "profile/profile.h"

namespace {

const char16_t kExportTitle[] = u"Ёкспорт";

std::filesystem::path MakeFileName(std::u16string_view text) {
  std::u16string result;
  base::ReplaceChars(text, u":", u"-", &result);
  return result;
}

class CsvExportCommandRun
    : private CsvExportContext,
      public std::enable_shared_from_this<CsvExportCommandRun> {
 public:
  explicit CsvExportCommandRun(CsvExportContext&& context)
      : CsvExportContext{std::move(context)} {}

  promise<void> Run() {
    const std::string_view kCsvExt[] = {"*.csv"};
    const DialogService::Filter kFilters[] = {
        {u"‘айлы CSV", kCsvExt},
    };

    auto file_name = MakeFileName(window_title_);
    file_name += ".csv";

    auto csv_export_dir = GetString16(profile_.data(), "csvPath");

    auto ref = shared_from_this();

    return dialog_service_
        .SelectSaveFile({.title = kExportTitle,
                         .default_path = csv_export_dir / file_name,
                         .filters = kFilters})
        .then([this, ref](const std::filesystem::path& path) {
          path_ = path;
          SetKey(profile_.data(), "csvPath", path.u16string());
        })
        .then([this, ref] {
          return ShowCsvExportDialog(dialog_service_, profile_);
        })
        .then(std::bind_front(&CsvExportCommandRun::Export, ref))
        .then([this, ref] {
          return dialog_service_.RunMessageBox(
              u"Ёкспорт завершен. ќткрыть файл сейчас?", kExportTitle,
              MessageBoxMode::QuestionYesNo);
        })
        .then([this, ref](MessageBoxResult open_prompt_result) {
          if (open_prompt_result == MessageBoxResult::Yes) {
            win_util::OpenWithAssociatedProgram(path_);
          }
        });
  }

 private:
  promise<void> Export(const CsvExportParams& params) {
    try {
      auto export_data = export_model_.GetExportData();

      std::visit([&](auto& data) { ::ExportToCsv(data, params, path_); },
                 export_data);

    } catch (const std::runtime_error&) {
      return dialog_service_
          .RunMessageBox(u"ќшибка при экспорте.", kExportTitle,
                         MessageBoxMode::Error)
          .then([e = std::current_exception()](MessageBoxResult) {
            return make_rejected_promise(e);
          });
    }

    return make_resolved_promise();
  }

  std::filesystem::path path_;
};

}  // namespace

promise<void> RunCsvExport(CsvExportContext&& context) {
  return std::make_shared<CsvExportCommandRun>(std::move(context))->Run();
}
