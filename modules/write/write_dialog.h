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

// The context is taken **by value**: this returns a lazy awaitable, and a
// coroutine does not copy reference parameters into its frame, so an rvalue
// reference here would dangle the moment the callee became a coroutine.
Awaitable<void> ExecuteWriteDialog(DialogService& dialog_service,
                                   WriteContext context);
