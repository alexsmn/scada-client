#pragma once

namespace scada {
class SessionService;
}

template <class T>
class BasicCommandRegistry;

struct MainCommandContext;
struct SelectionCommandContext;

struct DebuggerModuleContext {
  scada::SessionService& session_service_;
  BasicCommandRegistry<MainCommandContext>& main_commands_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
};

class DebuggerModule : private DebuggerModuleContext {
 public:
  explicit DebuggerModule(DebuggerModuleContext&& context);

 private:
  void DumpDebugInfo(const SelectionCommandContext& context);
};