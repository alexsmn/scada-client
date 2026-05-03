#pragma once

#include "base/awaitable.h"
#include "node_service/node_ref.h"

class DialogService;
class TaskManager;

struct LimitDialogContext {
  const NodeRef node_;
  TaskManager& task_manager_;
};

Awaitable<void> ShowLimitsDialog(DialogService& dialog_service,
                                 LimitDialogContext&& context);
