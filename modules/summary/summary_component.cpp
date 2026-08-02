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

  const std::pair<unsigned, const char*> interval_actions[] = {
      {ID_INTERVAL_1M, "1-Minute"}, {ID_INTERVAL_5M, "5 min"},
      {ID_INTERVAL_15M, "15 min"},  {ID_INTERVAL_30M, "30 min"},
      {ID_INTERVAL_1H, "1-Hour"},   {ID_INTERVAL_12H, "12 hours"},
      {ID_INTERVAL_1D, "1-Day"}};
  for (const auto& [command_id, title] : interval_actions) {
    ui_command_registry.AddAction(Action{.command_id_ = command_id,
                                         .category_ = CATEGORY_INTERVAL,
                                         .title_ = Translate(title),
                                         .flags_ = Action::CHECKABLE});
  }

  const std::pair<unsigned, const char*> aggregation_actions[] = {
      {ID_AGGREGATION_START, "First"}, {ID_AGGREGATION_END, "Last"},
      {ID_AGGREGATION_COUNT, "Count"}, {ID_AGGREGATION_MIN, "Minimum"},
      {ID_AGGREGATION_MAX, "Maximum"}, {ID_AGGREGATION_SUM, "Sum"},
      {ID_AGGREGATION_AVG, "Average"}};
  for (const auto& [command_id, title] : aggregation_actions) {
    ui_command_registry.AddAction(Action{.command_id_ = command_id,
                                         .category_ = CATEGORY_AGGREGATION,
                                         .title_ = Translate(title),
                                         .flags_ = Action::CHECKABLE});
  }
}
