#include "print/service/print_command.h"

#include "base/check.h"
#include "print/service/print_service.h"
#include "resources/common_resources.h"

PrintCommand::PrintCommand(PrintCommandContext&& context)
    : PrintCommandContext{std::move(context)} {}

CommandHandler* PrintCommand::GetCommandHandler(unsigned command_id) {
  return command_id == ID_PRINT && export_model_getter_() ? this : nullptr;
}

void PrintCommand::ExecuteCommand(unsigned command_id) {
  base::Check(command_id == ID_PRINT);
  auto print_view_handler = print_view_handler_;
  print_service_.ShowPrintPreviewDialog(
      dialog_service_,
      [print_view_handler = std::move(print_view_handler),
       &print_service = print_service_] { print_view_handler(print_service); });
}
