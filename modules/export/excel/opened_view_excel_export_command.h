#pragma once

#include "base/any_executor.h"
#include "base/excel.h"
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
  // Whether Excel can be automated here. The command is offered only then; it
  // used to need the undocumented `--excel` switch instead (backlog 873).
  std::function<bool()> excel_available_ = &Excel::IsAvailable;
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
