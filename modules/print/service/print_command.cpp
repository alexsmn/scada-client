#include "print/service/print_command.h"

#include "base/check.h"
#include "print/service/print_service.h"
#include "resources/common_resources.h"

PrintCommand::PrintCommand(PrintCommandContext&& context)
    : PrintCommandContext{std::move(context)} {}

CommandHandler* PrintCommand::GetCommandHandler(unsigned command_id) {
  if (command_id != ID_PRINT)
    return nullptr;
  bool printable = export_model_getter_() ||
                   (image_export_model_getter_ && image_export_model_getter_());
  return printable ? this : nullptr;
}

void PrintCommand::ExecuteCommand(unsigned command_id) {
  scada::base::Check(command_id == ID_PRINT);
  auto print_view_handler = print_view_handler_;
  print_service_.ShowPrintPreviewDialog(
      dialog_service_,
      [print_view_handler = std::move(print_view_handler),
       &print_service = print_service_] { print_view_handler(print_service); });
}
