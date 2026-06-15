#pragma once

#include "base/any_executor.h"
#include "base/cancelation.h"
#include "controller/command_handler.h"

#include <functional>

class DialogService;
class Profile;
class TimeModel;

using TimeModelGetter = std::function<TimeModel*()>;

struct OpenedViewTimeRangeCommandContext {
  AnyExecutor executor_;
  DialogService& dialog_service_;
  Profile& profile_;
  TimeModelGetter time_model_getter_;
};

class OpenedViewTimeRangeCommand final
    : private OpenedViewTimeRangeCommandContext,
      public CommandHandler {
 public:
  explicit OpenedViewTimeRangeCommand(
      OpenedViewTimeRangeCommandContext&& context);
  ~OpenedViewTimeRangeCommand();

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command_id) override;
  virtual bool IsCommandChecked(unsigned command_id) const override;

 private:
  Cancelation cancelation_;
};
