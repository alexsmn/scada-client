#pragma once

#include "base/any_executor.h"
#include "controller/window_info.h"

template <class T>
class BasicCommandRegistry;

class FileCache;
struct GlobalCommandContext;
struct SelectionCommandContext;
class UiCommandRegistry;

extern const WindowInfo kGraphWindowInfo;

struct GraphModuleContext {
  AnyExecutor executor_;
  FileCache& file_cache_;
  BasicCommandRegistry<GlobalCommandContext>& global_commands_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  UiCommandRegistry& ui_command_registry_;
};

class GraphModule : private GraphModuleContext {
 public:
  explicit GraphModule(GraphModuleContext&& context);
};

void RegisterGraphCommandActions(UiCommandRegistry& ui_command_registry);
