#pragma once

#include "base/any_executor.h"

namespace scada {
class SessionService;
}

template <class T>
class BasicCommandRegistry;

class UiCommandRegistry;
struct GlobalCommandContext;
struct SelectionCommandContext;

struct DebuggerModuleContext {
  // `DumpDebugInfo` confirms the clipboard copy through a message box, which
  // is a lazy awaitable and needs an executor to be spawned on. It cannot come
  // from `SelectionCommandContext`, which carries no executor and is shared by
  // every selection command. See `aui/show_message_box.h`.
  const AnyExecutor executor_;
  scada::SessionService& session_service_;
  BasicCommandRegistry<GlobalCommandContext>& global_commands_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  UiCommandRegistry& ui_command_registry_;
};

class DebuggerModule : private DebuggerModuleContext {
 public:
  explicit DebuggerModule(DebuggerModuleContext&& context);

 private:
  void DumpDebugInfo(const SelectionCommandContext& context);
};
