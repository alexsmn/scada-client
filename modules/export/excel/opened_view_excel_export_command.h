#pragma once

#include "controller/command_handler.h"

#include <functional>

class DialogService;
class ExportModel;

using ExcelExportModelGetter = std::function<ExportModel*()>;

struct OpenedViewExcelExportCommandContext {
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
