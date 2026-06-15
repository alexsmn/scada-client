#include "export/csv/csv_export_module.h"

#include "aui/translation.h"
#include "base/value_util.h"
#include "controller/action.h"
#include "controller/command_ui_registry.h"
#include "profile/profile.h"
#include "resources/common_resources.h"

CsvExportModule::CsvExportModule(CsvExportModuleContext&& context)
    : CsvExportModuleContext{std::move(context)} {
  ui_command_registry_.AddAction(Action{.command_id_ = ID_EXPORT_CSV,
                                        .category_ = CATEGORY_EXPORT,
                                        .title_ = Translate("Export to CSV")});
}

CsvExportModule::~CsvExportModule() {}
