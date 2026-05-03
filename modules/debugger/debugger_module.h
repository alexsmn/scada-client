#pragma once

namespace scada {
class SessionService;
}

class ActionManager;
struct GlobalCommandContext;
struct SelectionCommandContext;

struct DebuggerModuleContext {
  scada::SessionService& session_service_;
  ActionManager& action_manager_;
};

class DebuggerModule : private DebuggerModuleContext {
 public:
  explicit DebuggerModule(DebuggerModuleContext&& context);

 private:
  void DumpDebugInfo(const SelectionCommandContext& context);
};
