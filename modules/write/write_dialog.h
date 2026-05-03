#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"
#include "scada/node_id.h"

class DialogService;
class Profile;
class TimedDataService;

class Profile;
class TimedDataService;

struct WriteContext {
  const AnyExecutor executor_;
  TimedDataService& timed_data_service_;
  const scada::NodeId node_id_;
  Profile& profile_;
  const bool manual_ = false;
};

Awaitable<void> ExecuteWriteDialog(DialogService& dialog_service,
                                   WriteContext&& context);
