#pragma once

#include "common/node_ref.h"

class DialogService;
class TaskManager;

struct LimitDialogContext {
  const NodeRef node_;
  TaskManager& task_manager_;
};

void ShowLimitsDialog(DialogService& dialog_service,
                      LimitDialogContext&& context);
