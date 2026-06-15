#pragma once

#include "base/any_executor.h"
#include "scada/session_service.h"

template <class T>
class BasicCommandRegistry;

class NodeService;
class OpenedViewCommandRegistry;
class TaskManager;
class UiCommandRegistry;
struct SelectionCommandContext;

struct SelectionEditModuleContext {
  AnyExecutor executor_;
  scada::SessionService& session_service_;
  NodeService& node_service_;
  TaskManager& task_manager_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  UiCommandRegistry& ui_command_registry_;
  OpenedViewCommandRegistry& opened_view_commands_;
};

class SelectionEditModule : private SelectionEditModuleContext {
 public:
  explicit SelectionEditModule(SelectionEditModuleContext&& context);
};
