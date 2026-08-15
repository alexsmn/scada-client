#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "node_service/node_ref.h"

class DialogService;
class TaskManager;

struct LimitDialogContext {
  // The dialog's writes go through `TaskManager::PostUpdateTask`, which
  // returns a lazy awaitable; the model needs an executor to spawn it on, or
  // the update never runs. See `LimitModel::WriteLimits`.
  AnyExecutor executor_;
  const NodeRef node_;
  TaskManager& task_manager_;
};

// The context is taken **by value**: this returns a lazy awaitable, and a
// coroutine does not copy reference parameters into its frame, so an rvalue
// reference here would dangle the moment the callee became a coroutine.
Awaitable<void> ShowLimitsDialog(DialogService& dialog_service,
                                 LimitDialogContext context);
