#pragma once

#include "base/promise.h"
#include "node_service/node_ref.h"

class DialogService;
class TaskManager;

struct LimitDialogContext {
  const NodeRef node_;
  TaskManager& task_manager_;
};

promise<void> ShowLimitsDialog(DialogService& dialog_service,
                               LimitDialogContext&& context);
