#include "time_range/time_range_module.h"

#include "aui/translation.h"
#include "controller/action.h"
#include "controller/command_ui_registry.h"
#include "main_window/opened_view/opened_view_command_registry.h"
#include "resources/common_resources.h"
#include "time_range/opened_view_time_range_command.h"

TimeRangeModule::TimeRangeModule(TimeRangeModuleContext&& context)
    : TimeRangeModuleContext{std::move(context)} {
  const std::pair<unsigned, const char*> time_range_actions[] = {
      {ID_TIME_RANGE_15M, "15 min"},
      {ID_TIME_RANGE_HOUR, "Hour"},
      {ID_TIME_RANGE_DAY, "Day"},
      {ID_TIME_RANGE_WEEK, "Week"},
      {ID_TIME_RANGE_MONTH, "Month"}};
  for (const auto& [command_id, title] : time_range_actions) {
    ui_command_registry_.AddAction(Action{.command_id_ = command_id,
                                          .category_ = CATEGORY_PERIOD,
                                          .title_ = Translate(title),
                                          .flags_ = Action::CHECKABLE});
  }
  ui_command_registry_.AddAction(Action{.command_id_ = ID_TIME_RANGE_CUSTOM,
                                        .category_ = CATEGORY_PERIOD,
                                        .title_ = Translate("Custom..."),
                                        .short_title_ = Translate("Custom"),
                                        .flags_ = Action::CHECKABLE});
  opened_view_commands_.AddFactory(
      [](const OpenedViewCommandFactoryContext& context) {
        return std::make_unique<OpenedViewTimeRangeCommand>(
            OpenedViewTimeRangeCommandContext{
                .executor_ = context.executor_,
                .dialog_service_ = context.dialog_service_,
                .profile_ = context.profile_,
                .time_model_getter_ = context.time_model_getter_});
      });
}
