#include "export/csv/csv_export_command.h"

#include "aui/dialog_service.h"

#include <boost/algorithm/string/replace.hpp>
#include "base/awaitable_promise.h"
#include "base/value_util.h"
#include "base/win/win_util2.h"
#include "export/csv/csv_export.h"
#include "export/export_model.h"
#include "main_window/opened_view.h"
#include "net/net_executor_adapter.h"
#include "profile/profile.h"

namespace {

const char16_t kExportTitle[] = u"Export";

std::filesystem::path MakeFileName(std::u16string_view text) {
  auto result = boost::replace_all_copy(std::u16string{text}, u":", u"-");
  return result;
}

class CsvExportCommandRun
    : private CsvExportContext,
      public std::enable_shared_from_this<CsvExportCommandRun> {
 public:
  explicit CsvExportCommandRun(CsvExportContext&& context)
      : CsvExportContext{std::move(context)} {
    if (!show_csv_export_dialog)
      show_csv_export_dialog = ShowCsvExportDialog;
  }

  promise<void> Run() {
    return ToPromise(NetExecutorAdapter{executor_},
                     RunAsync(shared_from_this()));
  }

 private:
  Awaitable<void> RunAsync(std::shared_ptr<CsvExportCommandRun> /*self*/) {
    const std::string_view kCsvExt[] = {"*.csv"};
    const DialogService::Filter kFilters[] = {
        {u"CSV Files", kCsvExt},
    };

    auto file_name = MakeFileName(window_title_);
    file_name += ".csv";

    auto csv_export_dir = GetString16(profile_.data(), "csvPath");

    path_ = co_await AwaitPromise(
        NetExecutorAdapter{executor_},
        dialog_service_.SelectSaveFile({.title = kExportTitle,
                                        .default_path = csv_export_dir /
                                                        file_name,
                                        .filters = kFilters}));
    SetKey(profile_.data(), "csvPath", path_.u16string());

    auto params = co_await AwaitPromise(
        NetExecutorAdapter{executor_},
        show_csv_export_dialog(dialog_service_, profile_));
    co_await ExportAsync(params);

    auto open_prompt_result = co_await AwaitPromise(
        NetExecutorAdapter{executor_},
        dialog_service_.RunMessageBox(
            u"Export completed. Open the file now?", kExportTitle,
            MessageBoxMode::QuestionYesNo));
    if (open_prompt_result == MessageBoxResult::Yes)
      win_util::OpenWithAssociatedProgram(path_);

    co_return;
  }

  Awaitable<void> ExportAsync(const CsvExportParams& params) {
    std::exception_ptr export_exception;
    try {
      auto export_data = export_model_.GetExportData();

      std::visit([&](auto& data) { ::ExportToCsv(data, params, path_); },
                 export_data);

    } catch (const std::runtime_error&) {
      export_exception = std::current_exception();
    }

    if (export_exception) {
      co_await AwaitPromise(NetExecutorAdapter{executor_},
                            dialog_service_.RunMessageBox(
                                u"Export failed.", kExportTitle,
                                MessageBoxMode::Error));
      std::rethrow_exception(export_exception);
    }

    co_return;
  }

  std::filesystem::path path_;
};

}  // namespace

promise<void> RunCsvExport(CsvExportContext&& context) {
  return std::make_shared<CsvExportCommandRun>(std::move(context))->Run();
}
