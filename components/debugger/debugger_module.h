#pragma once

namespace scada {
class SessionService;
}

template <class T>
class BasicCommandRegistry;

struct MainCommandContext;

struct DebuggerModuleContext {
  scada::SessionService& session_service_;
  BasicCommandRegistry<MainCommandContext>& main_commands_;
};

class DebuggerModule : private DebuggerModuleContext {
 public:
  explicit DebuggerModule(DebuggerModuleContext&& context);
};
