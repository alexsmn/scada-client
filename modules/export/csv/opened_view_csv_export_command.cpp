#include "export/csv/opened_view_csv_export_command.h"

#include "base/check.h"
#include "export/csv/csv_export_command.h"
#include "net/net_executor_adapter.h"
#include "resources/common_resources.h"

OpenedViewCsvExportCommand::OpenedViewCsvExportCommand(
    OpenedViewCsvExportCommandContext&& context)
    : OpenedViewCsvExportCommandContext{std::move(context)} {}

OpenedViewCsvExportCommand::~OpenedViewCsvExportCommand() = default;

CommandHandler* OpenedViewCsvExportCommand::GetCommandHandler(
    unsigned command_id) {
  return command_id == ID_EXPORT_CSV && export_model_getter_() ? this : nullptr;
}

void OpenedViewCsvExportCommand::ExecuteCommand(unsigned command_id) {
  scada::base::Check(command_id == ID_EXPORT_CSV);
  if (auto* export_model = export_model_getter_()) {
    CoSpawn(
        executor_, cancelation_,
        [this, export_model,
         window_title = window_title_getter_()]() mutable -> Awaitable<void> {
          co_await RunCsvExport({executor_, dialog_service_, profile_,
                                 *export_model, std::move(window_title)});
          co_return;
        });
  }
}
