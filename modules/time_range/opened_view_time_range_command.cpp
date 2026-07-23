#include "time_range/opened_view_time_range_command.h"

#include "base/check.h"
#include "controller/time_model.h"
#include "net/net_executor_adapter.h"
#include "resources/common_resources.h"
#include "time_range/time_range_dialog.h"

#include <optional>

namespace {

std::optional<TimeRange> GetTimeRangeCommand(unsigned command_id) {
  switch (command_id) {
    case ID_TIME_RANGE_15M:
      return TimeRange{std::chrono::minutes(15)};
    case ID_TIME_RANGE_HOUR:
      return TimeRange{std::chrono::hours(1)};
    case ID_TIME_RANGE_DAY:
      return TimeRange::Type::Day;
    case ID_TIME_RANGE_WEEK:
      return TimeRange::Type::Week;
    case ID_TIME_RANGE_MONTH:
      return TimeRange::Type::Month;
    case ID_TIME_RANGE_CUSTOM:
      return TimeRange{/*start=*/scada::base::Time{},
                       /*end=*/scada::base::Time{}};
    default:
      return std::nullopt;
  }
}

}  // namespace

OpenedViewTimeRangeCommand::OpenedViewTimeRangeCommand(
    OpenedViewTimeRangeCommandContext&& context)
    : OpenedViewTimeRangeCommandContext{std::move(context)} {}

OpenedViewTimeRangeCommand::~OpenedViewTimeRangeCommand() = default;

CommandHandler* OpenedViewTimeRangeCommand::GetCommandHandler(
    unsigned command_id) {
  return GetTimeRangeCommand(command_id) && time_model_getter_() ? this
                                                                 : nullptr;
}

void OpenedViewTimeRangeCommand::ExecuteCommand(unsigned command_id) {
  auto time_range = GetTimeRangeCommand(command_id);
  scada::base::Check(time_range);

  auto* model = time_model_getter_();
  if (!model) {
    return;
  }

  if (time_range->type == TimeRange::Type::Custom) {
    auto range = model->GetTimeRange();
    bool time_required = model->IsTimeRequired();
    CoSpawn(executor_, cancelation_,
            [model, &dialog_service = dialog_service_, &profile = profile_,
             range, time_required]() mutable -> Awaitable<void> {
              auto picked = co_await ShowTimeRangeDialog(
                  dialog_service, {profile, range, time_required});
              model->SetTimeRange(picked);
              co_return;
            });
  } else {
    model->SetTimeRange(*time_range);
  }
}

bool OpenedViewTimeRangeCommand::IsCommandChecked(unsigned command_id) const {
  auto time_range = GetTimeRangeCommand(command_id);
  if (!time_range) {
    return false;
  }

  if (auto* model = time_model_getter_()) {
    auto current_time_range = model->GetTimeRange();
    return time_range->is_custom() ? current_time_range.type == time_range->type
                                   : current_time_range == *time_range;
  }
  return false;
}
