#include "export/csv/csv_export_module.h"

#include "aui/translation.h"
#include "base/value_util.h"
#include "controller/action.h"
#include "controller/command_ui_registry.h"
#include "export/csv/opened_view_csv_export_command.h"
#include "main_window/opened_view/opened_view_command_registry.h"
#include "profile/profile.h"
#include "resources/common_resources.h"

CsvExportModule::CsvExportModule(CsvExportModuleContext&& context)
    : CsvExportModuleContext{std::move(context)} {
  ui_command_registry_.AddAction(Action{.command_id_ = ID_EXPORT_CSV,
                                        .category_ = CATEGORY_EXPORT,
                                        .title_ = Translate("Export to CSV")});
  opened_view_commands_.AddFactory(
      [](const OpenedViewCommandFactoryContext& context) {
        return std::make_unique<OpenedViewCsvExportCommand>(
            OpenedViewCsvExportCommandContext{
                .executor_ = context.executor_,
                .dialog_service_ = context.dialog_service_,
                .profile_ = context.profile_,
                .export_model_getter_ = context.export_model_getter_,
                .window_title_getter_ = context.window_title_getter_});
      });
}

CsvExportModule::~CsvExportModule() {}
