#pragma once

#include "base/any_executor.h"
#include "controller/command_handler.h"

#include <functional>

class DialogService;
class ExportModel;

using ExcelExportModelGetter = std::function<ExportModel*()>;

struct OpenedViewExcelExportCommandContext {
  // The Excel failure is reported through a message box, which is a lazy
  // awaitable and needs an executor to be spawned on. See
  // `aui/show_message_box.h`.
  const AnyExecutor executor_;
  DialogService& dialog_service_;
  ExcelExportModelGetter export_model_getter_;
};

class OpenedViewExcelExportCommand final
    : private OpenedViewExcelExportCommandContext,
      public CommandHandler {
 public:
  explicit OpenedViewExcelExportCommand(
      OpenedViewExcelExportCommandContext&& context);

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command_id) override;
};
