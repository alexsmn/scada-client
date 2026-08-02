#pragma once

#include "base/any_executor.h"

#include "controller/command_handler.h"
#include "core/global_command_context.h"

#include <functional>

namespace scada {
class SessionService;
}

template <class T>
class BasicCommandRegistry;

class DialogService;
class MainWindowInterface;
struct GlobalCommandContext;

struct MainWindowCommandRouterContext {
  AnyExecutor executor_;
  MainWindowInterface& main_window_;
  DialogService& dialog_service_;
  scada::SessionService& session_service_;
  BasicCommandRegistry<GlobalCommandContext>& global_commands_;
};

class MainWindowCommandRouter : private MainWindowCommandRouterContext,
                                public CommandHandler {
 public:
  explicit MainWindowCommandRouter(MainWindowCommandRouterContext&& context);
  ~MainWindowCommandRouter();

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id);
  virtual bool IsCommandEnabled(unsigned command_id) const;
  virtual bool IsCommandChecked(unsigned command_id) const;
  virtual void ExecuteCommand(unsigned command_id);

 private:
  GlobalCommandContext command_context_;
};
