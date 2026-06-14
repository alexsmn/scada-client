#include "modules/summary/summary_component.h"

#include "aui/translation.h"
#include "controller/action.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_registry.h"
#include "modules/selection_command_helpers.h"
#include "modules/summary/summary_view.h"
#include "resources/common_resources.h"

#include <utility>

const WindowInfo kSummaryWindowInfo = {ID_SUMMARY_VIEW, "Summ", u"Summary",
                                       WIN_INS | WIN_CAN_PRINT};

REGISTER_CONTROLLER(SummaryView, kSummaryWindowInfo);

SummaryModule::SummaryModule(SummaryModuleContext&& context)
    : SummaryModuleContext{std::move(context)} {
  RegisterSummaryCommandActions(ui_command_registry_);
  selection_commands_.AddCommand(MakeOpenViewSelectionCommand(
      ID_OPEN_SUMMARY, kSummaryWindowInfo, executor_));
}

void RegisterSummaryCommandActions(UiCommandRegistry& ui_command_registry) {
  ui_command_registry.AddAction(Action{.command_id_ = ID_OPEN_SUMMARY,
                                       .category_ = CATEGORY_OPEN,
                                       .title_ = Translate("Summary"),
                                       .image_id_ = IDB_SUMMARY,
                                       .flags_ = Action::ALWAYS_VISIBLE});
}
