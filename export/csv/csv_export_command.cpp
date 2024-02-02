#include "export/csv/csv_export_command.h"

#include "aui/dialog_service.h"
#include "base/string_piece_util.h"
#include "base/strings/string_util.h"
#include "base/win/win_util2.h"
#include "export/csv/csv_export.h"
#include "export/export_model.h"
#include "main_window/opened_view.h"
#include "profile/profile.h"

namespace {

const char16_t kExportTitle[] = u"Ёкспорт";

std::filesystem::path MakeFileName(std::u16string_view text) {
  std::u16string result;
  base::ReplaceChars(AsStringPiece(text), u":", u"-", &result);
  return result;
}

class CsvExportCommandRun
    : private CsvExportContext,
      public std::enable_shared_from_this<CsvExportCommandRun> {
 public:
  explicit CsvExportCommandRun(CsvExportContext&& context)
      : CsvExportContext{std::move(context)} {}

  void Run() {
    const std::string_view kCsvExt[] = {"*.csv"};
    const DialogService::Filter kFilters[] = {
        {u"‘айлы CSV", kCsvExt},
    };

    auto file_name = MakeFileName(window_title_);
    file_name += ".csv";

    dialog_service_
        .SelectSaveFile({.title = kExportTitle,
                         .default_path = profile_.csv_export_dir / file_name,
                         .filters = kFilters})
        .then([this,
               ref = shared_from_this()](const std::filesystem::path& path) {
          profile_.csv_export_dir = path.parent_path();

          ShowCsvExportDialog(dialog_service_, profile_)
              .then([this, ref, path](const CsvExportParams& params) {
                auto export_data = export_model_.GetExportData();

                try {
                  std::visit(
                      [&](auto& data) { ::ExportToCsv(data, params, path); },
                      export_data);

                } catch (const std::runtime_error&) {
                  dialog_service_.RunMessageBox(u"ќшибка при экспорте.",
                                                kExportTitle,
                                                MessageBoxMode::Error);
                  return;
                }

                dialog_service_
                    .RunMessageBox(u"Ёкспорт завершен. ќткрыть файл сейчас?",
                                   kExportTitle, MessageBoxMode::QuestionYesNo)
                    .then(BindExecutor(
                        executor_, [path = std::move(path)](
                                       MessageBoxResult message_box_result) {
                          if (message_box_result == MessageBoxResult::Yes)
                            win_util::OpenWithAssociatedProgram(path);
                        }));
              });
        });
  }
};

}  // namespace

void RunCsvExport(CsvExportContext&& context) {
  std::make_shared<CsvExportCommandRun>(std::move(context))->Run();
}
