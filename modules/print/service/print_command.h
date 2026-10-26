#pragma once

#include "controller/command_handler.h"

#include <functional>

class DialogService;
class ExportModel;
class ImageExportModel;
class PrintService;

using ExportModelGetter = std::function<ExportModel*()>;
using ImageExportModelGetter = std::function<ImageExportModel*()>;
using PrintViewHandler = std::function<void(PrintService&)>;

struct PrintCommandContext {
  PrintService& print_service_;
  DialogService& dialog_service_;
  ExportModelGetter export_model_getter_;
  // A view that prints as a picture (the graph) rather than as rows.
  ImageExportModelGetter image_export_model_getter_;
  PrintViewHandler print_view_handler_;
};

class PrintCommand final : private PrintCommandContext, public CommandHandler {
 public:
  explicit PrintCommand(PrintCommandContext&& context);

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command_id) override;
};
