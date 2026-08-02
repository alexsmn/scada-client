#pragma once

#include "base/any_executor.h"
#include "base/cancelation.h"
#include "controller/command_handler.h"

#include <functional>

class DialogService;
class ExportModel;
class Profile;

using CsvExportModelGetter = std::function<ExportModel*()>;
using CsvWindowTitleGetter = std::function<std::u16string()>;

struct OpenedViewCsvExportCommandContext {
  AnyExecutor executor_;
  DialogService& dialog_service_;
  Profile& profile_;
  CsvExportModelGetter export_model_getter_;
  CsvWindowTitleGetter window_title_getter_;
};

class OpenedViewCsvExportCommand final
    : private OpenedViewCsvExportCommandContext,
      public CommandHandler {
 public:
  explicit OpenedViewCsvExportCommand(
      OpenedViewCsvExportCommandContext&& context);
  ~OpenedViewCsvExportCommand();

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command_id) override;

 private:
  Cancelation cancelation_;
};
